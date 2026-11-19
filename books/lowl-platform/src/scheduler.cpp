// MIT License - (c) Anthony Matarazzo 2026
#include "scheduler.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>

namespace lowl {
namespace {

struct Run {
    std::vector<Task> tasks;
    std::vector<int> remaining;
    std::vector<double> wait;
    double now = 0.0;
    int switches = 0;
    int last = -1;
};

// Charge every task that is still runnable for a slice given to someone else.
void charge_wait(Run& r, std::size_t runner, int slice) {
    for (std::size_t j = 0; j < r.tasks.size(); ++j) {
        if (j != runner && r.remaining[j] > 0) {
            r.wait[j] += slice;
        }
    }
}

void settle(Stats& s, const Run& r) {
    const std::size_t n = r.tasks.size();
    if (n == 0) {
        return;
    }
    double wait_sum = 0.0;
    int interactive_count = 0;
    double interactive_sum = 0.0;
    std::vector<double> turnaround;
    turnaround.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        wait_sum += r.wait[i];
        s.max_wait = std::max(s.max_wait, r.wait[i]);
        turnaround.push_back(r.wait[i] + static_cast<double>(r.tasks[i].cost_units));
        if (r.tasks[i].interactive) {
            ++interactive_count;
            interactive_sum += r.wait[i];
            s.max_interactive_wait = std::max(s.max_interactive_wait, r.wait[i]);
        }
    }
    s.mean_wait = wait_sum / static_cast<double>(n);
    if (interactive_count > 0) {
        s.mean_interactive_wait = interactive_sum / static_cast<double>(interactive_count);
    }
    s.fairness = jain_index(turnaround);
    if (s.makespan_units > 0.0) {
        s.throughput_per_1000 = static_cast<double>(n) * 1000.0 / s.makespan_units;
    }
}

}  // namespace

double jain_index(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    double sumsq = 0.0;
    for (double v : values) {
        sum += v;
        sumsq += v * v;
    }
    if (sumsq == 0.0) {
        return 0.0;
    }
    return (sum * sum) / (static_cast<double>(values.size()) * sumsq);
}

Stats run_round_robin(const std::vector<Task>& tasks, int quantum) {
    Stats s;
    s.policy = "round robin";
    if (tasks.empty() || quantum <= 0) {
        return s;
    }
    Run r;
    r.tasks = tasks;
    r.remaining.resize(tasks.size());
    r.wait.assign(tasks.size(), 0.0);
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        r.remaining[i] = tasks[i].cost_units;
    }

    bool any_left = true;
    while (any_left) {
        any_left = false;
        for (std::size_t i = 0; i < r.tasks.size(); ++i) {
            if (r.remaining[i] <= 0) {
                continue;
            }
            any_left = true;
            if (r.last != static_cast<int>(i)) {
                ++r.switches;
                r.last = static_cast<int>(i);
            }
            const int slice = std::min(quantum, r.remaining[i]);
            charge_wait(r, i, slice);
            r.now += slice;
            r.remaining[i] -= slice;
        }
    }
    s.makespan_units = r.now;
    s.context_switches = r.switches;
    settle(s, r);
    return s;
}

Stats run_priority(const std::vector<Task>& tasks) {
    Stats s;
    s.policy = "strict priority";
    if (tasks.empty()) {
        return s;
    }
    Run r;
    r.tasks = tasks;
    r.remaining.resize(tasks.size());
    r.wait.assign(tasks.size(), 0.0);
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        r.remaining[i] = tasks[i].cost_units;
    }

    // Highest priority first, ties keep arrival order. Each task runs to
    // completion without preemption, which is what makes a long batch task
    // block a keystroke handler.
    std::vector<std::size_t> order(tasks.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return tasks[a].priority > tasks[b].priority;
    });

    for (std::size_t i : order) {
        ++r.switches;
        r.last = static_cast<int>(i);
        const int cost = r.remaining[i];
        charge_wait(r, i, cost);
        r.now += cost;
        r.remaining[i] = 0;
    }
    s.makespan_units = r.now;
    s.context_switches = r.switches;
    settle(s, r);
    return s;
}

Stats run_weighted_fair(const std::vector<Task>& tasks, int quantum) {
    Stats s;
    s.policy = "weighted fair share";
    if (tasks.empty() || quantum <= 0) {
        return s;
    }
    Run r;
    r.tasks = tasks;
    r.remaining.resize(tasks.size());
    r.wait.assign(tasks.size(), 0.0);
    std::vector<double> served(tasks.size(), 0.0);
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        r.remaining[i] = tasks[i].cost_units;
    }

    for (;;) {
        int pick = -1;
        double best = std::numeric_limits<double>::max();
        for (std::size_t i = 0; i < tasks.size(); ++i) {
            if (r.remaining[i] <= 0) {
                continue;
            }
            const double weight = static_cast<double>(std::max(1, r.tasks[i].priority));
            const double key = served[i] / weight;
            if (key < best) {
                best = key;
                pick = static_cast<int>(i);
            }
        }
        if (pick < 0) {
            break;
        }
        if (r.last != pick) {
            ++r.switches;
            r.last = pick;
        }
        const std::size_t i = static_cast<std::size_t>(pick);
        const int slice = std::min(quantum, r.remaining[i]);
        charge_wait(r, i, slice);
        r.now += slice;
        r.remaining[i] -= slice;
        served[i] += slice;
    }
    s.makespan_units = r.now;
    s.context_switches = r.switches;
    settle(s, r);
    return s;
}

std::vector<Task> mixed_workload(int batch_tasks, int interactive_tasks) {
    // Deterministic: the same call always produces the same task set.
    std::vector<Task> tasks;
    for (int i = 0; i < batch_tasks; ++i) {
        Task t;
        t.name = "batch" + std::to_string(i);
        t.cost_units = 60 + (i * 17) % 40;
        t.priority = 1;
        t.interactive = false;
        tasks.push_back(t);
    }
    for (int i = 0; i < interactive_tasks; ++i) {
        Task t;
        t.name = "ui" + std::to_string(i);
        t.cost_units = 2 + (i % 3);
        t.priority = 8;
        t.interactive = true;
        tasks.push_back(t);
    }
    return tasks;
}

WeightCheck check_weights(int quantum) {
    // Two equal-size tasks, weights 4 and 1. Over the first 500 units of CPU time
    // weighted fair share must give the heavy task about 4 times the service.
    WeightCheck c;
    c.expected_ratio = 4.0;
    if (quantum <= 0) {
        return c;
    }
    const int weights[2] = {4, 1};
    std::vector<double> served(2, 0.0);
    int remaining[2] = {400, 400};
    double clock = 0.0;
    while (clock < 500.0) {
        int pick = -1;
        double best = std::numeric_limits<double>::max();
        for (int i = 0; i < 2; ++i) {
            if (remaining[i] <= 0) {
                continue;
            }
            const double key = served[static_cast<std::size_t>(i)] /
                               static_cast<double>(weights[i]);
            if (key < best) {
                best = key;
                pick = i;
            }
        }
        if (pick < 0) {
            break;
        }
        const int slice = std::min(quantum, remaining[pick]);
        remaining[pick] -= slice;
        served[static_cast<std::size_t>(pick)] += slice;
        clock += slice;
    }
    c.observed_ratio = served[1] > 0 ? served[0] / served[1] : 0.0;
    return c;
}

}  // namespace lowl

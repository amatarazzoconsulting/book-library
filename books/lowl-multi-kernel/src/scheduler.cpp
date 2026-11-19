// MIT License - (c) Anthony Matarazzo 2026
#include "scheduler.hpp"

#include <algorithm>
#include <cstdio>
#include <limits>

namespace lowl {
namespace kal {

const char* policy_name(Policy p) {
    switch (p) {
        case Policy::RoundRobin: return "round_robin";
        case Policy::Priority: return "priority";
        case Policy::FairShare: return "fair_share";
        case Policy::Gang: return "gang";
        case Policy::Count: break;
    }
    return "unknown";
}

Scheduler::Scheduler(KernelAdapter* kernel, const SchedOptions& options)
    : kernel_(kernel), options_(options) {}

void Scheduler::set_options(const SchedOptions& o) { options_ = o; }

unsigned Scheduler::add_task(int priority, double weight, unsigned gang) {
    Task t;
    t.id = static_cast<unsigned>(tasks_.size());
    t.priority = priority;
    t.weight = weight > 0.0 ? weight : 1.0;
    t.gang = gang;
    tasks_.push_back(t);
    return t.id;
}

void Scheduler::set_runnable(unsigned id, bool run) {
    if (id >= tasks_.size()) return;
    // A task that is parked keeps its vruntime so it resumes where it left off;
    // a negative sentinel marks it not runnable.
    if (run) {
        if (tasks_[id].vruntime < 0.0) tasks_[id].vruntime = 0.0;
    } else {
        tasks_[id].vruntime = -1.0;
    }
}

bool Scheduler::runnable(unsigned id) const {
    if (id >= tasks_.size()) return false;
    return tasks_[id].vruntime >= 0.0;
}

void Scheduler::reset() {
    tasks_.clear();
    stats_ = SchedStats{};
    cursor_ = 0;
    gang_cursor_ = 0;
    gang_dispatched_ = 0;
}

unsigned long Scheduler::runs(unsigned id) const {
    if (id >= tasks_.size()) return 0;
    return tasks_[id].runs;
}

long Scheduler::pick_round_robin() {
    const std::size_t n = tasks_.size();
    if (n == 0) return -1;
    for (std::size_t i = 0; i < n; ++i) {
        const std::size_t idx = (cursor_ + i) % n;
        if (runnable(static_cast<unsigned>(idx))) {
            cursor_ = (idx + 1) % n;
            return static_cast<long>(idx);
        }
    }
    return -1;
}

long Scheduler::pick_priority() {
    long best = -1;
    for (std::size_t i = 0; i < tasks_.size(); ++i) {
        if (!runnable(static_cast<unsigned>(i))) continue;
        if (best < 0) {
            best = static_cast<long>(i);
            continue;
        }
        const Task& a = tasks_[static_cast<std::size_t>(i)];
        const Task& b = tasks_[static_cast<std::size_t>(best)];
        if (a.priority > b.priority) {
            best = static_cast<long>(i);
        } else if (a.priority == b.priority && a.vruntime < b.vruntime) {
            best = static_cast<long>(i);
        }
    }
    return best;
}

long Scheduler::pick_fair_share() {
    long best = -1;
    double best_key = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i < tasks_.size(); ++i) {
        if (!runnable(static_cast<unsigned>(i))) continue;
        const Task& t = tasks_[i];
        // vruntime is already normalised by weight in charge(), so the plain
        // minimum is the fair share winner. Dividing again would double-count.
        const double key = t.vruntime;
        if (key < best_key) {
            best_key = key;
            best = static_cast<long>(i);
        }
    }
    return best;
}

long Scheduler::pick_gang() {
    // Gang scheduling: a gang runs as a unit. All runnable members of the
    // current gang are dispatched (round robin inside the gang) before the
    // scheduler moves to the next gang, so co-scheduled tasks make progress
    // together and cannot deadlock on a barrier.
    std::vector<unsigned> gangs;
    for (const Task& t : tasks_) {
        if (t.vruntime < 0.0) continue;
        if (std::find(gangs.begin(), gangs.end(), t.gang) == gangs.end()) {
            gangs.push_back(t.gang);
        }
    }
    if (gangs.empty()) return -1;
    std::sort(gangs.begin(), gangs.end());

    std::size_t pos = 0;
    bool in_list = false;
    for (std::size_t i = 0; i < gangs.size(); ++i) {
        if (gangs[i] == gang_cursor_) {
            pos = i;
            in_list = true;
            break;
        }
    }
    if (!in_list) {
        pos = 0;
        gang_cursor_ = gangs[0];
        gang_dispatched_ = 0;
    }

    std::size_t members = 0;
    for (const Task& t : tasks_) {
        if (t.vruntime >= 0.0 && t.gang == gang_cursor_) ++members;
    }

    const std::size_t n = tasks_.size();
    for (std::size_t i = 0; i < n; ++i) {
        const std::size_t idx = (cursor_ + i) % n;
        if (runnable(static_cast<unsigned>(idx)) && tasks_[idx].gang == gang_cursor_) {
            cursor_ = (idx + 1) % n;
            ++gang_dispatched_;
            if (gang_dispatched_ >= members) {
                gang_dispatched_ = 0;
                pos = (pos + 1) % gangs.size();
                const unsigned next = gangs[pos];
                if (next != gang_cursor_) ++stats_.gang_switches;
                gang_cursor_ = next;
            }
            return static_cast<long>(idx);
        }
    }
    return -1;
}

long Scheduler::pick_starved() {
    // Starvation guard: any runnable task whose run count is `starve_limit`
    // behind the leader is promoted ahead of the policy choice.
    if (options_.starve_limit == 0) return -1;
    unsigned long max_runs = 0;
    for (const Task& t : tasks_) {
        if (t.vruntime >= 0.0) max_runs = std::max(max_runs, t.runs);
    }
    for (std::size_t i = 0; i < tasks_.size(); ++i) {
        const Task& t = tasks_[i];
        if (t.vruntime < 0.0) continue;
        if (max_runs >= t.runs && max_runs - t.runs >= options_.starve_limit) {
            return static_cast<long>(i);
        }
    }
    return -1;
}

void Scheduler::charge(long id) {
    if (id < 0) return;
    Task& t = tasks_[static_cast<std::size_t>(id)];
    ++t.runs;
    const double step = static_cast<double>(options_.quantum) / t.weight;
    t.vruntime += step;
    if (options_.charge_kernel && kernel_ != nullptr) {
        const CallResult r = kernel_->call(Op::SemAcquire);
        stats_.kernel_cost_ns += r.cost_ns;
    }
}

long Scheduler::dispatch() {
    long id = -1;
    if (options_.policy == Policy::RoundRobin) {
        id = pick_round_robin();
    } else if (options_.policy == Policy::Priority) {
        id = pick_priority();
        const long forced = pick_starved();
        if (forced >= 0 && forced != id) {
            id = forced;
            ++stats_.starved;
        }
    } else if (options_.policy == Policy::FairShare) {
        id = pick_fair_share();
        const long forced = pick_starved();
        if (forced >= 0 && forced != id) {
            id = forced;
            ++stats_.starved;
        }
    } else {
        id = pick_gang();
    }

    ++stats_.dispatched;
    if (id < 0) {
        ++stats_.idle;
        return -1;
    }
    charge(id);
    return id;
}

unsigned long Scheduler::run(unsigned long count) {
    unsigned long live = 0;
    for (unsigned long i = 0; i < count; ++i) {
        if (dispatch() >= 0) ++live;
    }
    return live;
}

unsigned long Scheduler::spread() const {
    unsigned long lo = std::numeric_limits<unsigned long>::max();
    unsigned long hi = 0;
    bool any = false;
    for (const Task& t : tasks_) {
        if (t.runs == 0) continue;
        any = true;
        lo = std::min(lo, t.runs);
        hi = std::max(hi, t.runs);
    }
    if (!any) return 0;
    return hi - lo;
}

std::string Scheduler::report() const {
    char line[160];
    std::string out = std::string("policy=") + policy_name(options_.policy) +
                      " quantum=" + std::to_string(options_.quantum) +
                      " tasks=" + std::to_string(tasks_.size()) + "\n";
    for (const Task& t : tasks_) {
        std::snprintf(line, sizeof line,
                      "  task %2u prio %3d weight %5.2f gang %u runs %6lu vruntime %9.2f%s\n",
                      t.id, t.priority, t.weight, t.gang, t.runs, t.vruntime,
                      t.vruntime < 0.0 ? " (parked)" : "");
        out += line;
    }
    std::snprintf(line, sizeof line,
                  "  dispatched=%lu idle=%lu starved=%lu gang_switches=%lu "
                  "kernel_cost=%.0fns\n",
                  stats_.dispatched, stats_.idle, stats_.starved, stats_.gang_switches,
                  stats_.kernel_cost_ns);
    out += line;
    return out;
}

}  // namespace kal
}  // namespace lowl

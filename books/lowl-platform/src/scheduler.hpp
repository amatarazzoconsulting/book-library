// MIT License - (c) Anthony Matarazzo 2026
#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <cstddef>
#include <string>
#include <vector>

// Scheduling model for the multitasking and throughput claims in "LOWL Platform"
// part 5 and the scheduler notes in part 4.3. The book asserts that LOWL keeps
// interactive latency low under 100 percent CPU load because the foreground
// application gets priority. That claim is testable with a discrete event
// simulation: run the same task set under round robin, priority and weighted fair
// share, then compare mean and worst case response time for interactive tasks.
namespace lowl {

struct Task {
    std::string name;
    int cost_units = 0;     // total CPU time the task needs
    int priority = 0;       // higher runs first under the priority policy
    bool interactive = false;
};

struct Stats {
    std::string policy;
    double makespan_units = 0.0;
    double throughput_per_1000 = 0.0;  // tasks completed per 1000 time units
    double mean_wait = 0.0;
    double max_wait = 0.0;
    double mean_interactive_wait = 0.0;
    double max_interactive_wait = 0.0;
    int context_switches = 0;
    double fairness = 0.0;  // Jain index over completed work, 1.0 is perfectly even
};

// Round robin with a fixed quantum. Every task gets the quantum until it runs out
// of work, in arrival order.
Stats run_round_robin(const std::vector<Task>& tasks, int quantum);

// Strict priority, no preemption inside a task. Highest priority runs to
// completion. This is the pathological case for interactive latency.
Stats run_priority(const std::vector<Task>& tasks);

// Weighted fair share: at each step the runnable task with the smallest
// work_done / weight ratio runs for one quantum. Tasks that have not run are
// charged zero work so they start first, which bounds starvation.
Stats run_weighted_fair(const std::vector<Task>& tasks, int quantum);

// A reproducible task set: a mix of interactive and batch work, like the mixed
// workload the book describes for the 80 to 100 application claim.
std::vector<Task> mixed_workload(int batch_tasks, int interactive_tasks);

// Jain fairness index over the completed work vector.
double jain_index(const std::vector<double>& values);

// Weighted fair share is only useful if the weight ratio is bounded. Report the
// worst observed service ratio between two tasks of different weight.
struct WeightCheck {
    double expected_ratio = 0.0;
    double observed_ratio = 0.0;
};

WeightCheck check_weights(int quantum);

}  // namespace lowl

#endif  // SCHEDULER_HPP

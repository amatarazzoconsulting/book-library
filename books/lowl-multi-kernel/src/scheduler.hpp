// MIT License - (c) Anthony Matarazzo 2026
//
// Scheduler policy adapter: one dispatch interface, four policies
// (round robin, strict priority, weighted fair share, gang). Each dispatch is
// gated through a KernelAdapter so the cost of the underlying kernel's
// semaphore primitive is charged to the policy that caused it.
#ifndef LOWL_KAL_SCHEDULER_HPP
#define LOWL_KAL_SCHEDULER_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "kernel_adapter.hpp"

namespace lowl {
namespace kal {

enum class Policy { RoundRobin, Priority, FairShare, Gang, Count };
const char* policy_name(Policy p);

struct Task {
    unsigned id{0};
    int priority{0};       // higher wins under Policy::Priority
    double weight{1.0};    // share weight under Policy::FairShare
    unsigned gang{0};      // gang index under Policy::Gang
    double vruntime{0.0};  // accumulated virtual runtime
    unsigned long runs{0};
};

// Every knob is exercised by teaching.cpp and the benchmarks.
struct SchedOptions {
    Policy policy{Policy::RoundRobin};
    unsigned quantum{4};        // vruntime charged per dispatch
    bool charge_kernel{true};   // gate each dispatch through the kernel adapter
    unsigned starve_limit{0};   // 0 disables the starvation guard
};

struct SchedStats {
    unsigned long dispatched{0};
    unsigned long idle{0};       // dispatch calls that found nothing runnable
    unsigned long starved{0};    // times the starvation guard overrode the policy
    double kernel_cost_ns{0.0};
    unsigned long gang_switches{0};
};

class Scheduler {
public:
    Scheduler(KernelAdapter* kernel, const SchedOptions& options);
    explicit Scheduler(const SchedOptions& options) : Scheduler(nullptr, options) {}

    void set_options(const SchedOptions& o);
    const SchedOptions& options() const { return options_; }

    unsigned add_task(int priority = 0, double weight = 1.0, unsigned gang = 0);
    void set_runnable(unsigned id, bool runnable);
    bool runnable(unsigned id) const;
    std::size_t size() const { return tasks_.size(); }
    const std::vector<Task>& tasks() const { return tasks_; }

    // Returns the chosen task id, or -1 when nothing is runnable.
    long dispatch();

    // Run `count` dispatches, returning how many were non-idle.
    unsigned long run(unsigned long count);

    void reset();  // clears tasks and stats

    const SchedStats& stats() const { return stats_; }
    unsigned long runs(unsigned id) const;

    // Distribution quality of the last run: max-min run count over tasks that
    // were runnable at least once. 0 means perfectly even.
    unsigned long spread() const;

    std::string report() const;

private:
    long pick_round_robin();
    long pick_priority();
    long pick_fair_share();
    long pick_gang();
    long pick_starved();
    void charge(long id);

    KernelAdapter* kernel_;
    SchedOptions options_;
    std::vector<Task> tasks_;
    SchedStats stats_{};
    std::size_t cursor_{0};
    unsigned gang_cursor_{0};
    std::size_t gang_dispatched_{0};
};

}  // namespace kal
}  // namespace lowl

#endif  // LOWL_KAL_SCHEDULER_HPP

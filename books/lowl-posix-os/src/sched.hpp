// MIT License - (c) Anthony Matarazzo 2026
//
// lowl-posix-os : scheduler core.
// Implements posix/scheduler.lp: 41 run queues (priority -20..20), an active
// and an expired array that are swapped when the active one drains, timeslice
// sizing from priority, and per-process accounting. Adds the fairness metrics
// the book only hints at (Jain index, wait time spread, throughput).
#ifndef LOWL_SCHED_HPP
#define LOWL_SCHED_HPP

#include <array>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include "pcb.hpp"

namespace lowl {

constexpr int NR_PRIO = 41;  // -20 .. 20

struct SchedParams {
    std::uint32_t min_timeslice = 10;      // ms, priority -20
    std::uint32_t max_timeslice = 100;     // ms, priority 20
    std::uint32_t default_timeslice = 50;  // ms, used before the first sizing
    std::uint64_t sched_latency = 60;      // CFS: target period in ticks
    std::uint64_t min_granularity = 6;     // CFS: floor for one slice
};

struct SchedStats {
    std::uint64_t ticks = 0;
    std::uint64_t dispatches = 0;        // times a process was put on the cpu
    std::uint64_t context_switches = 0;  // times the running process changed
    std::uint64_t preemptions = 0;       // switches caused by timeslice expiry
    std::uint64_t voluntary = 0;         // switches caused by yield/block
    std::uint64_t idle_ticks = 0;
    std::uint64_t completed = 0;
    std::uint64_t expired_swaps = 0;
    double fairness_jain = 0.0;  // 1/N (worst) .. 1.0 (perfectly equal)
    double mean_wait = 0.0;      // ticks from first ready to first dispatch
    double max_wait = 0.0;
    std::uint64_t wait_samples = 0;

    double avg_wait() const {
        return wait_samples == 0 ? 0.0 : mean_wait / static_cast<double>(wait_samples);
    }
};

// One FIFO of pids for a single priority level.
class RunQueue {
public:
    void push_back(std::int32_t pid) { q_.push_back(pid); }
    // Insert keeping ascending vruntime. Used by CFS levels.
    void insert_by_vruntime(std::int32_t pid, std::uint64_t vruntime);
    std::int32_t pop_front();
    bool remove(std::int32_t pid);
    bool empty() const { return q_.empty(); }
    std::size_t size() const { return q_.size(); }
    const std::deque<std::int32_t>& items() const { return q_; }
    void clear() { q_.clear(); }

private:
    std::deque<std::int32_t> q_;
    std::vector<std::uint64_t> vr_;  // parallel to q_, only used when ordered
};

class Scheduler {
public:
    explicit Scheduler(ProcessTable& table);

    void init();
    void set_params(const SchedParams& p) { params_ = p; }
    const SchedParams& params() const { return params_; }

    // NEW -> READY and place on the correct level. Returns false if unknown.
    bool admit(std::int32_t pid);
    bool enqueue(std::int32_t pid);
    bool dequeue(std::int32_t pid);

    // Pick the next runnable pid, or -1 when nothing is ready.
    std::int32_t pick_next();
    // One scheduling decision: save the old process, pick, switch.
    std::int32_t schedule(bool preempted);
    // One 1 kHz tick of the simulated clock.
    void tick();
    void run(std::uint64_t ticks);
    void yield();
    void block(std::int32_t pid);
    void wake(std::int32_t pid);

    std::int32_t current() const { return current_; }
    std::uint64_t clock() const { return clock_; }
    std::uint32_t timeslice_for(std::int8_t priority) const;
    std::uint32_t timeslice_for(const PCB& p) const;
    std::uint32_t cfs_slice(const PCB& p) const;

    std::size_t runnable_count() const;
    bool has_runnable() const { return runnable_count() > 0; }
    std::size_t expired_count() const;
    void swap_queues();

    const SchedStats& stats() const { return stats_; }
    void reset_stats();
    // Fairness of accumulated cpu time across the given pids.
    double jain_index(const std::vector<std::int32_t>& pids) const;
    std::uint64_t cpu_ticks(std::int32_t pid) const;

    const RunQueue& queue(int level) const { return active_[static_cast<std::size_t>(level)]; }
    const RunQueue& expired_queue(int level) const {
        return expired_[static_cast<std::size_t>(level)];
    }
    bool using_expired() const { return active_is_expired_; }

private:
    int level_of(std::int8_t priority) const;
    void place(PCB& p, bool expired);
    void account_running();

    ProcessTable& table_;
    SchedParams params_;
    std::array<RunQueue, NR_PRIO> active_;
    std::array<RunQueue, NR_PRIO> expired_;
    bool active_is_expired_ = false;

    std::int32_t current_ = -1;
    std::uint64_t clock_ = 0;
    std::uint64_t slice_start_ = 0;
    std::uint64_t min_vruntime_ = 0;
    SchedStats stats_;
    std::map<std::int32_t, std::uint64_t> cpu_;
    std::map<std::int32_t, std::uint64_t> ready_since_;
};

}  // namespace lowl

#endif  // LOWL_SCHED_HPP

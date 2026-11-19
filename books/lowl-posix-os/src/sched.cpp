// MIT License - (c) Anthony Matarazzo 2026
#include "sched.hpp"

#include <algorithm>
#include <cmath>

namespace lowl {
namespace {

// CFS weight for a priority level. Range 100 (idle) .. 1125 (max), which is
// the same shape as Linux's nice-to-weight table, only linear and small enough
// to explain in one line: w = 100 + (prio + 20) * 25.
std::uint64_t weight_of(std::int8_t priority) {
    int p = priority < prio::MIN ? prio::MIN : (priority > prio::MAX ? prio::MAX : priority);
    return static_cast<std::uint64_t>(100 + (p + 20) * 25);
}

constexpr std::uint64_t NICE0_WEIGHT = 1024;

}  // namespace

void RunQueue::insert_by_vruntime(std::int32_t pid, std::uint64_t vruntime) {
    if (vr_.size() != q_.size()) vr_.assign(q_.size(), 0);
    std::size_t i = 0;
    while (i < vr_.size() && vr_[i] <= vruntime) ++i;
    q_.insert(q_.begin() + static_cast<std::ptrdiff_t>(i), pid);
    vr_.insert(vr_.begin() + static_cast<std::ptrdiff_t>(i), vruntime);
}

std::int32_t RunQueue::pop_front() {
    if (q_.empty()) return -1;
    std::int32_t pid = q_.front();
    q_.pop_front();
    if (!vr_.empty()) vr_.erase(vr_.begin());
    return pid;
}

bool RunQueue::remove(std::int32_t pid) {
    for (std::size_t i = 0; i < q_.size(); ++i) {
        if (q_[i] == pid) {
            q_.erase(q_.begin() + static_cast<std::ptrdiff_t>(i));
            if (i < vr_.size()) vr_.erase(vr_.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

Scheduler::Scheduler(ProcessTable& table) : table_(table) {}

void Scheduler::init() {
    for (auto& q : active_) q.clear();
    for (auto& q : expired_) q.clear();
    active_is_expired_ = false;
    current_ = -1;
    clock_ = 0;
    slice_start_ = 0;
    min_vruntime_ = 0;
    cpu_.clear();
    ready_since_.clear();
    reset_stats();
}

void Scheduler::reset_stats() {
    stats_ = SchedStats();
    stats_.fairness_jain = 1.0;
}

int Scheduler::level_of(std::int8_t priority) const {
    return static_cast<int>(prio::clamp(priority)) + 20;
}

std::uint32_t Scheduler::timeslice_for(std::int8_t priority) const {
    // calculate_timeslice() from the book: base is the clamped priority mapped
    // to 0..40, slice interpolates min_timeslice..max_timeslice.
    int base = level_of(priority);
    std::uint32_t span = params_.max_timeslice - params_.min_timeslice;
    return params_.min_timeslice + static_cast<std::uint32_t>(base) * span / 40u;
}

std::uint32_t Scheduler::timeslice_for(const PCB& p) const {
    if (p.policy == Policy::Fifo) return 0;  // runs until it yields
    if (p.policy == Policy::Cfs) return cfs_slice(p);
    return timeslice_for(p.priority);
}

std::uint32_t Scheduler::cfs_slice(const PCB& p) const {
    // slice = latency * w_i / sum(w). Sum over the runnable CFS set at the
    // moment of dispatch would need a scan; the cheap approximation the book
    // implies is to use the queue depth at this level.
    std::uint64_t w = weight_of(p.priority);
    std::uint64_t total = 0;
    int level = level_of(p.priority);
    for (const auto& pid : active_[static_cast<std::size_t>(level)].items()) {
        const PCB* q = table_.get(pid);
        if (q && q->policy == Policy::Cfs) total += weight_of(q->priority);
    }
    if (total < w) total = w;
    std::uint64_t slice = params_.sched_latency * w / total;
    if (slice < params_.min_granularity) slice = params_.min_granularity;
    return static_cast<std::uint32_t>(slice);
}

bool Scheduler::admit(std::int32_t pid) {
    PCB* p = table_.get(pid);
    if (!p) return false;
    if (p->state != ProcState::New && p->state != ProcState::Ready) return false;
    p->state = ProcState::Ready;
    p->time_slice = timeslice_for(*p);
    if (p->policy == Policy::Cfs && p->vruntime < min_vruntime_) p->vruntime = min_vruntime_;
    return enqueue(pid);
}

void Scheduler::place(PCB& p, bool expired) {
    int level = level_of(p.priority);
    auto& q = expired ? expired_[static_cast<std::size_t>(level)]
                      : active_[static_cast<std::size_t>(level)];
    if (p.policy == Policy::Cfs) {
        q.insert_by_vruntime(p.pid, p.vruntime);
    } else {
        q.push_back(p.pid);
    }
}

bool Scheduler::enqueue(std::int32_t pid) {
    PCB* p = table_.get(pid);
    if (!p) return false;
    // Refuse to queue the same process twice: a run queue is a set of pids.
    for (auto& q : active_) {
        for (auto id : q.items()) if (id == pid) return true;
    }
    for (auto& q : expired_) {
        for (auto id : q.items()) if (id == pid) return true;
    }
    if (p->time_slice == 0) {
        p->time_slice = timeslice_for(*p);
        place(*p, true);  // no slice left: refill in the expired array
    } else {
        place(*p, false);
    }
    if (ready_since_.find(pid) == ready_since_.end()) ready_since_[pid] = clock_;
    return true;
}

bool Scheduler::dequeue(std::int32_t pid) {
    const PCB* p = table_.get(pid);
    if (p) {
        int level = level_of(p->priority);
        if (active_[static_cast<std::size_t>(level)].remove(pid)) return true;
        if (expired_[static_cast<std::size_t>(level)].remove(pid)) return true;
    }
    for (auto& q : active_) if (q.remove(pid)) return true;
    for (auto& q : expired_) if (q.remove(pid)) return true;
    return false;
}

std::size_t Scheduler::runnable_count() const {
    std::size_t n = 0;
    for (const auto& q : active_) n += q.size();
    return n;
}

std::size_t Scheduler::expired_count() const {
    std::size_t n = 0;
    for (const auto& q : expired_) n += q.size();
    return n;
}

void Scheduler::swap_queues() {
    std::swap(active_, expired_);
    active_is_expired_ = !active_is_expired_;
    ++stats_.expired_swaps;
}

std::int32_t Scheduler::pick_next() {
    for (int pass = 0; pass < 2; ++pass) {
        for (int level = NR_PRIO - 1; level >= 0; --level) {
            auto& q = active_[static_cast<std::size_t>(level)];
            if (q.empty()) continue;

            // Within a level, CFS members are ordered by vruntime and FIFO/RR
            // members by arrival. Mixed levels pick the smallest vruntime
            // among CFS members first, because that is what CFS promises.
            std::int32_t chosen = -1;
            std::uint64_t best = 0;
            for (auto pid : q.items()) {
                const PCB* p = table_.get(pid);
                if (!p) continue;
                if (p->policy != Policy::Cfs) continue;
                if (chosen < 0 || p->vruntime < best) { chosen = pid; best = p->vruntime; }
            }
            if (chosen < 0) chosen = q.items().front();
            q.remove(chosen);
            return chosen;
        }
        if (expired_count() > 0) {
            swap_queues();
        } else {
            break;
        }
    }
    return -1;
}

void Scheduler::account_running() {
    if (current_ < 0) return;
    PCB* p = table_.get(current_);
    if (!p || p->state != ProcState::Running) return;
    cpu_[current_] += 1;
    if (p->flags & pflags::KTHREAD) {
        p->system_time += 1;
    } else {
        p->user_time += 1;
    }
    if (p->policy == Policy::Cfs) {
        p->vruntime += (NICE0_WEIGHT * 1) / weight_of(p->priority);
        if (p->vruntime > min_vruntime_) min_vruntime_ = p->vruntime;
    }
}

std::int32_t Scheduler::schedule(bool preempted) {
    std::int32_t old = current_;

    if (old >= 0) {
        PCB* p = table_.get(old);
        if (p && p->state == ProcState::Running) {
            p->state = ProcState::Ready;
            p->last_ran = clock_;
            p->time_slice = 0;  // consumed; refilled on enqueue
            enqueue(old);
        }
    }

    std::int32_t next = pick_next();
    if (next < 0) {
        current_ = -1;
        return -1;
    }

    PCB* np = table_.get(next);
    if (!np) { current_ = -1; return -1; }

    np->state = ProcState::Running;
    np->last_ran = clock_;
    if (np->time_slice == 0) np->time_slice = timeslice_for(*np);
    slice_start_ = clock_;

    // Wait accounting: how long did this process sit ready before dispatch.
    auto it = ready_since_.find(next);
    if (it != ready_since_.end()) {
        double waited = static_cast<double>(clock_ - it->second);
        stats_.mean_wait += waited;
        stats_.max_wait = std::max(stats_.max_wait, waited);
        ++stats_.wait_samples;
        ready_since_.erase(it);
    }

    if (next != old) {
        ++stats_.context_switches;
        if (preempted) ++stats_.preemptions;
        else if (old >= 0) ++stats_.voluntary;
    }
    ++stats_.dispatches;
    current_ = next;
    return next;
}

void Scheduler::tick() {
    ++clock_;
    ++stats_.ticks;

    if (current_ < 0) {
        ++stats_.idle_ticks;
    } else {
        PCB* p = table_.get(current_);
        if (p && p->state == ProcState::Running) {
            account_running();
            bool expired = false;
            if (p->policy != Policy::Fifo && p->time_slice > 0) {
                if (clock_ - slice_start_ >= p->time_slice) expired = true;
            }
            if (expired) schedule(true);
        }
    }
}

void Scheduler::run(std::uint64_t ticks) {
    for (std::uint64_t i = 0; i < ticks; ++i) tick();
}

void Scheduler::yield() {
    if (current_ >= 0) schedule(false);
}

void Scheduler::block(std::int32_t pid) {
    PCB* p = table_.get(pid);
    if (!p) return;
    if (pid == current_) {
        p->state = ProcState::Waiting;
        current_ = -1;
        schedule(false);
    } else {
        dequeue(pid);
        p->state = ProcState::Waiting;
    }
}

void Scheduler::wake(std::int32_t pid) {
    PCB* p = table_.get(pid);
    if (!p) return;
    if (p->state != ProcState::Waiting) return;
    p->state = ProcState::Ready;
    p->time_slice = timeslice_for(*p);
    enqueue(pid);
}

std::uint64_t Scheduler::cpu_ticks(std::int32_t pid) const {
    auto it = cpu_.find(pid);
    return it == cpu_.end() ? 0 : it->second;
}

double Scheduler::jain_index(const std::vector<std::int32_t>& pids) const {
    if (pids.empty()) return 0.0;
    double sum = 0.0, sq = 0.0;
    for (auto pid : pids) {
        double x = static_cast<double>(cpu_ticks(pid));
        sum += x;
        sq += x * x;
    }
    if (sq == 0.0) return 1.0;
    return (sum * sum) / (static_cast<double>(pids.size()) * sq);
}

}  // namespace lowl

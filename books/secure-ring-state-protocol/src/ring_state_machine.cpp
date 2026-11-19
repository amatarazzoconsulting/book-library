// MIT License - (c) Anthony Matarazzo 2026
//
// ring_state_machine.cpp - guarded ring lifecycle transitions.

#include "ring_state_machine.hpp"

#include <algorithm>
#include <deque>
#include <set>

namespace srsp {
namespace ring {

namespace {

struct named_state {
    ring_state s;
    const char* name;
};

const named_state kStates[] = {
    {ring_state::boot, "boot"},
    {ring_state::probing, "probing"},
    {ring_state::joining, "joining"},
    {ring_state::syncing, "syncing"},
    {ring_state::active, "active"},
    {ring_state::degraded, "degraded"},
    {ring_state::draining, "draining"},
    {ring_state::sealed, "sealed"},
    {ring_state::failed, "failed"},
};

}  // namespace

const char* to_string(ring_state s) {
    for (const auto& e : kStates) {
        if (e.s == s) return e.name;
    }
    return "unknown";
}

bool parse_state(const std::string& name, ring_state& out) {
    for (const auto& e : kStates) {
        if (name == e.name) {
            out = e.s;
            return true;
        }
    }
    return false;
}

std::size_t state_count() { return sizeof(kStates) / sizeof(kStates[0]); }

std::vector<std::string> ring_machine::guard_names() {
    return {"always",
            "quorum_met",
            "quorum_lost",
            "peers_seen",
            "integrity_ok",
            "lease_ok",
            "drain_ready",
            "drain_done",
            "rekey_ready",
            "retry_budget",
            "epoch_advanced"};
}

ring_machine::ring_machine() : ring_machine(config{}) {}

ring_machine::ring_machine(config cfg) : cfg_(cfg) {
    if (cfg_.quorum_required == 0) cfg_.quorum_required = 1;
    if (cfg_.peer_count < cfg_.quorum_required) cfg_.peer_count = cfg_.quorum_required;
    install_default_table();
}

void ring_machine::install_default_table() {
    edges_.clear();
    // Boot and discovery.
    add_transition(ring_state::boot, ring_state::probing, "always");
    add_transition(ring_state::boot, ring_state::failed, "integrity_ok");
    add_transition(ring_state::probing, ring_state::joining, "peers_seen");
    add_transition(ring_state::probing, ring_state::failed, "retry_budget");
    // Join and sync. Syncing is mandatory before active.
    add_transition(ring_state::joining, ring_state::syncing, "quorum_met");
    add_transition(ring_state::joining, ring_state::probing, "retry_budget");
    add_transition(ring_state::joining, ring_state::failed, "retry_budget");
    add_transition(ring_state::syncing, ring_state::active, "integrity_ok");
    add_transition(ring_state::syncing, ring_state::probing, "retry_budget");
    // Steady state.
    add_transition(ring_state::active, ring_state::degraded, "quorum_lost");
    add_transition(ring_state::active, ring_state::draining, "drain_ready");
    add_transition(ring_state::active, ring_state::sealed, "rekey_ready");
    add_transition(ring_state::degraded, ring_state::active, "quorum_met");
    add_transition(ring_state::degraded, ring_state::draining, "drain_ready");
    add_transition(ring_state::degraded, ring_state::failed, "retry_budget");
    // Drain and seal.
    add_transition(ring_state::draining, ring_state::sealed, "drain_done");
    add_transition(ring_state::draining, ring_state::failed, "retry_budget");
    add_transition(ring_state::sealed, ring_state::probing, "epoch_advanced");
    add_transition(ring_state::failed, ring_state::probing, "retry_budget");
}

bool ring_machine::add_transition(ring_state from, ring_state to, const std::string& guard) {
    if (from == to) return false;
    if (find(from, to) != nullptr) return false;
    const auto names = guard_names();
    if (std::find(names.begin(), names.end(), guard) == names.end()) return false;
    edges_.push_back(edge{from, to, guard});
    return true;
}

bool ring_machine::remove_transition(ring_state from, ring_state to) {
    auto it = std::find_if(edges_.begin(), edges_.end(), [&](const edge& e) {
        return e.from == from && e.to == to;
    });
    if (it == edges_.end()) return false;
    edges_.erase(it);
    return true;
}

std::vector<std::string> ring_machine::transitions_from(ring_state from) const {
    std::vector<std::string> out;
    for (const auto& e : edges_) {
        if (e.from == from) out.push_back(std::string(to_string(e.to)) + ":" + e.guard);
    }
    return out;
}

std::size_t ring_machine::transition_count() const { return edges_.size(); }

const ring_machine::edge* ring_machine::find(ring_state from, ring_state to) const {
    for (const auto& e : edges_) {
        if (e.from == from && e.to == to) return &e;
    }
    return nullptr;
}

bool ring_machine::guard_ok(const std::string& guard, const guard_context& ctx) const {
    const std::size_t need = ctx.quorum_required > 0 ? ctx.quorum_required : cfg_.quorum_required;
    if (guard == "always") return true;
    if (guard == "quorum_met") return ctx.quorum_votes >= need;
    if (guard == "quorum_lost") return cfg_.allow_degraded && ctx.quorum_votes < need;
    if (guard == "peers_seen") return ctx.peers_reachable > 0;
    if (guard == "integrity_ok") return ctx.integrity_ok;
    if (guard == "lease_ok") return ctx.lease_valid;
    if (guard == "drain_ready") return ctx.drain_requested;
    if (guard == "drain_done") return ctx.drain_complete;
    if (guard == "rekey_ready") return ctx.rekey_ready && ctx.integrity_ok;
    if (guard == "retry_budget") return ctx.retries <= cfg_.max_retries;
    if (guard == "epoch_advanced") return ctx.epoch > epoch_;
    return !cfg_.strict_guards;
}

transition_result ring_machine::step(ring_state target, const guard_context& ctx) {
    transition_result r;
    r.from = state_;
    r.to = target;
    r.seq = seq_;

    if (target == state_) {
        r.reason = "already in requested state";
        ++rejected_;
        return r;
    }

    const edge* e = find(state_, target);
    if (e == nullptr) {
        r.reason = "no transition from " + std::string(to_string(state_)) + " to " +
                   std::string(to_string(target));
        ++rejected_;
        return r;
    }

    r.guard = e->guard;
    if (!guard_ok(e->guard, ctx)) {
        r.reason = "guard '" + e->guard + "' failed";
        ++rejected_;
        return r;
    }

    // epoch_advanced is the only guard that also mutates the epoch counter.
    if (e->guard == "epoch_advanced") epoch_ = ctx.epoch;
    if (state_ == ring_state::failed || state_ == ring_state::probing) {
        if (target == ring_state::probing) ++failures_;
    }

    ++seq_;
    r.ok = true;
    r.seq = seq_;
    r.reason = "guard '" + e->guard + "' passed";
    state_ = target;
    visits_[static_cast<int>(target)] += 1;

    history_.push_back(history_entry{seq_, r.from, r.to, r.guard, epoch_});
    if (history_.size() > cfg_.history_limit) {
        history_.erase(history_.begin());
    }
    return r;
}

std::size_t ring_machine::advance(const guard_context& ctx) {
    static const ring_state order[] = {ring_state::probing, ring_state::joining,
                                       ring_state::syncing, ring_state::active};
    std::size_t moved = 0;
    for (ring_state target : order) {
        if (state_ == target) continue;
        if (state_ > target) continue;
        const auto r = step(target, ctx);
        if (!r.ok) break;
        ++moved;
        if (state_ == ring_state::active) break;
    }
    return moved;
}

bool ring_machine::can_reach(ring_state target) const {
    if (target == state_) return true;
    std::set<int> seen;
    std::deque<ring_state> q;
    q.push_back(state_);
    seen.insert(static_cast<int>(state_));
    while (!q.empty()) {
        const ring_state cur = q.front();
        q.pop_front();
        for (const auto& e : edges_) {
            if (e.from != cur) continue;
            if (e.to == target) return true;
            if (seen.insert(static_cast<int>(e.to)).second) q.push_back(e.to);
        }
    }
    return false;
}

std::vector<ring_state> ring_machine::path_to(ring_state target) const {
    std::map<int, int> parent;
    std::deque<ring_state> q;
    q.push_back(state_);
    parent[static_cast<int>(state_)] = -1;
    while (!q.empty()) {
        const ring_state cur = q.front();
        q.pop_front();
        if (cur == target) break;
        for (const auto& e : edges_) {
            if (e.from != cur) continue;
            const int key = static_cast<int>(e.to);
            if (parent.find(key) != parent.end()) continue;
            parent[key] = static_cast<int>(cur);
            q.push_back(e.to);
        }
    }
    std::vector<ring_state> path;
    int cur = static_cast<int>(target);
    if (parent.find(cur) == parent.end()) return path;
    while (cur != -1) {
        path.push_back(static_cast<ring_state>(cur));
        cur = parent[cur];
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<std::pair<ring_state, std::size_t>> ring_machine::visit_counts() const {
    std::vector<std::pair<ring_state, std::size_t>> out;
    for (const auto& e : kStates) {
        auto it = visits_.find(static_cast<int>(e.s));
        out.push_back({e.s, it == visits_.end() ? 0 : it->second});
    }
    return out;
}

void ring_machine::reset() {
    state_ = ring_state::boot;
    epoch_ = 0;
    seq_ = 0;
    failures_ = 0;
    rejected_ = 0;
    history_.clear();
    visits_.clear();
}

}  // namespace ring
}  // namespace srsp

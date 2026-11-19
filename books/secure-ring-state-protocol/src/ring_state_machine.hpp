// MIT License - (c) Anthony Matarazzo 2026
//
// ring_state_machine.hpp - guarded ring lifecycle state machine.
//
// Models the node lifecycle described in chapter "SYSTEM DESCRIPTION": a node
// moves boot -> probing -> joining -> syncing -> active, may degrade when the
// ring loses peers, drains on shutdown, and reseals after a rekey. Every edge
// carries a named guard; a transition only happens when its guard evaluates
// true against a caller supplied guard_context. The machine is pure: no
// threads, no clock, no randomness, so a given event stream always produces
// the same history.

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace srsp {
namespace ring {

enum class ring_state {
    boot = 0,
    probing,
    joining,
    syncing,
    active,
    degraded,
    draining,
    sealed,
    failed
};

const char* to_string(ring_state s);
bool parse_state(const std::string& name, ring_state& out);
std::size_t state_count();

// Facts a guard may inspect. The caller fills this in; the machine never
// consults the network or the wall clock on its own.
struct guard_context {
    std::size_t quorum_votes = 0;      // votes observed for the current proposal
    std::size_t quorum_required = 2;   // votes needed for quorum
    std::size_t peers_reachable = 0;   // peers answering gossip right now
    std::size_t peers_expected = 3;    // ring size this node believes in
    bool integrity_ok = true;          // hash and signature check on the last packet
    bool lease_valid = true;           // session lease has not expired
    bool drain_requested = false;      // operator asked for a graceful stop
    bool drain_complete = false;       // all locks handed off
    bool rekey_ready = false;          // new epoch key material is installed
    std::uint64_t epoch = 0;           // epoch the caller is proposing
    std::size_t retries = 0;           // failed join attempts so far
};

struct transition_result {
    bool ok = false;
    ring_state from = ring_state::boot;
    ring_state to = ring_state::boot;
    std::string guard;
    std::string reason;
    std::uint64_t seq = 0;
};

struct history_entry {
    std::uint64_t seq = 0;
    ring_state from = ring_state::boot;
    ring_state to = ring_state::boot;
    std::string guard;
    std::uint64_t epoch = 0;
};

class ring_machine {
public:
    struct config {
        std::size_t quorum_required = 2;
        std::size_t peer_count = 3;
        std::size_t max_retries = 3;   // failed -> probing attempts before giving up
        bool allow_degraded = true;    // permit active -> degraded when peers drop
        bool strict_guards = true;     // false records transitions that fail a guard
        std::size_t history_limit = 256;
    };

    ring_machine();
    explicit ring_machine(config cfg);

    // Table editing. A guard name that is not registered is treated as failing
    // when strict_guards is true, and as passing when strict_guards is false.
    void install_default_table();
    bool add_transition(ring_state from, ring_state to, const std::string& guard);
    bool remove_transition(ring_state from, ring_state to);
    std::vector<std::string> transitions_from(ring_state from) const;
    std::size_t transition_count() const;

    // Guard evaluation, exposed so teaching programs can probe it directly.
    bool guard_ok(const std::string& guard, const guard_context& ctx) const;
    static std::vector<std::string> guard_names();

    // Attempt one edge. The edge must exist in the table and its guard must
    // pass. On success the state, epoch and sequence advance.
    transition_result step(ring_state target, const guard_context& ctx);

    // Convenience: drive the happy path as far as the guards allow.
    std::size_t advance(const guard_context& ctx);

    ring_state state() const { return state_; }
    std::uint64_t epoch() const { return epoch_; }
    std::uint64_t seq() const { return seq_; }
    std::size_t failures() const { return failures_; }
    std::size_t rejected() const { return rejected_; }

    // Reachability over the transition table, ignoring guards. Useful to prove
    // that a lifecycle cannot skip syncing before active.
    bool can_reach(ring_state target) const;
    std::vector<ring_state> path_to(ring_state target) const;

    const std::vector<history_entry>& history() const { return history_; }
    std::vector<std::pair<ring_state, std::size_t>> visit_counts() const;

    void reset();
    const config& settings() const { return cfg_; }

private:
    struct edge {
        ring_state from;
        ring_state to;
        std::string guard;
    };

    const edge* find(ring_state from, ring_state to) const;

    config cfg_;
    std::vector<edge> edges_;
    ring_state state_ = ring_state::boot;
    std::uint64_t epoch_ = 0;
    std::uint64_t seq_ = 0;
    std::size_t failures_ = 0;
    std::size_t rejected_ = 0;
    std::vector<history_entry> history_;
    std::map<int, std::size_t> visits_;
};

}  // namespace ring
}  // namespace srsp

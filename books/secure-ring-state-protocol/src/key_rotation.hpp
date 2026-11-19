// MIT License - (c) Anthony Matarazzo 2026
//
// key_rotation.hpp - epoch based key rotation schedule.
//
// A rotating key ring: every epoch owns a key derived from a master secret and
// the epoch number, so any node holding the master can recompute the whole
// chain while an observer who learns one epoch key learns nothing about its
// neighbours. Three policies are offered:
//
//   fixed_epoch    rotate after N operations
//   time_based     rotate after N seconds of logical clock
//   sliding_window accept the current epoch plus the previous overlap epochs
//
// The clock is injected (a tick counter, not the wall clock) so the schedule is
// deterministic and reproducible in tests and benchmarks.

#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace srsp {
namespace keys {

using bytes = std::vector<std::uint8_t>;

enum class policy {
    fixed_epoch = 0,
    time_based,
    sliding_window
};

const char* to_string(policy p);
bool parse_policy(const std::string& name, policy& out);

enum class event_kind {
    install = 0,
    rotate,
    accept,
    reject_stale,
    reject_future,
    revoke
};

const char* to_string(event_kind k);

struct event {
    std::uint64_t epoch = 0;
    event_kind kind = event_kind::install;
    std::string detail;
};

struct config {
    policy mode = policy::fixed_epoch;
    std::size_t rotate_after_ops = 1000;   // fixed_epoch threshold
    std::uint64_t rotate_after_ticks = 60; // time_based threshold
    std::size_t overlap_epochs = 1;        // sliding_window accepted history
    std::size_t key_len = 32;
    std::uint64_t start_epoch = 1;
    bool allow_backfill = false;           // precompute keys ahead of time
};

struct key_epoch {
    std::uint64_t epoch = 0;
    bytes key;
    std::uint64_t created_tick = 0;
    std::size_t ops_served = 0;
    bool revoked = false;
    std::string fingerprint() const;
};

struct verdict {
    bool ok = false;
    event_kind kind = event_kind::reject_stale;
    std::uint64_t epoch = 0;
    std::string reason;
};

class rotation_schedule {
public:
    explicit rotation_schedule(config cfg, const bytes& master_secret);

    // Logical clock. Nothing rotates on its own; the caller ticks.
    void tick(std::uint64_t amount = 1);
    std::uint64_t now() const { return tick_; }

    // Record one operation against the current epoch. May trigger rotation
    // when the policy threshold is reached.
    verdict record_operation();

    // Force a rotation now, optionally skipping ahead by a number of epochs.
    verdict force_rotate(std::uint64_t skip = 1);

    // Check an epoch number presented by a peer. Applies the policy's
    // acceptance window.
    verdict check(std::uint64_t epoch) const;

    // Recompute a key from the master for any epoch, without storing it.
    bytes derive(std::uint64_t epoch) const;
    bytes derive_labeled(const std::string& label, std::uint64_t epoch) const;

    key_epoch current() const { return current_; }
    std::uint64_t current_epoch() const { return current_.epoch; }
    std::size_t ops_in_epoch() const { return current_.ops_served; }

    std::optional<key_epoch> get(std::uint64_t epoch) const;
    std::vector<std::uint64_t> retained_epochs() const;

    // Destroy a specific epoch key so it can never be handed out again.
    bool revoke(std::uint64_t epoch);

    std::size_t rotations() const { return rotations_; }
    std::size_t rejects() const { return rejects_; }
    std::size_t accepts() const { return accepts_; }
    const std::vector<event>& journal() const { return journal_; }
    const config& settings() const { return cfg_; }

    // Number of epochs that should still be accepted given the policy.
    std::size_t acceptance_window() const;

    void reset();

private:
    void prune();
    void log(event_kind kind, const std::string& detail);
    bool should_rotate() const;

    config cfg_;
    bytes master_;
    key_epoch current_;
    std::map<std::uint64_t, key_epoch> ring_;
    std::deque<std::uint64_t> order_;
    std::uint64_t tick_ = 0;
    std::uint64_t epoch_started_tick_ = 0;
    std::size_t rotations_ = 0;
    // check() is const but still accounts for what it decided, so an operator
    // can see how much stale traffic the ring is being offered.
    mutable std::size_t rejects_ = 0;
    mutable std::size_t accepts_ = 0;
    std::vector<event> journal_;
};

}  // namespace keys
}  // namespace srsp

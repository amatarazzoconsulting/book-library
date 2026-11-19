// MIT License - (c) Anthony Matarazzo 2026
//
// key_rotation.cpp - epoch key ring and rotation policy.

#include "key_rotation.hpp"

#include "ring_handshake.hpp"
#include "sha256.hpp"

#include <algorithm>

namespace srsp {
namespace keys {

const char* to_string(policy p) {
    switch (p) {
        case policy::fixed_epoch: return "fixed_epoch";
        case policy::time_based: return "time_based";
        case policy::sliding_window: return "sliding_window";
    }
    return "unknown";
}

bool parse_policy(const std::string& name, policy& out) {
    if (name == "fixed_epoch") { out = policy::fixed_epoch; return true; }
    if (name == "time_based") { out = policy::time_based; return true; }
    if (name == "sliding_window") { out = policy::sliding_window; return true; }
    return false;
}

const char* to_string(event_kind k) {
    switch (k) {
        case event_kind::install: return "install";
        case event_kind::rotate: return "rotate";
        case event_kind::accept: return "accept";
        case event_kind::reject_stale: return "reject_stale";
        case event_kind::reject_future: return "reject_future";
        case event_kind::revoke: return "revoke";
    }
    return "unknown";
}

std::string key_epoch::fingerprint() const {
    if (key.empty()) return "none";
    const auto d = hash::sha256::digest(key.data(), key.size());
    return hash::to_hex(d).substr(0, 16);
}

rotation_schedule::rotation_schedule(config cfg, const bytes& master_secret)
    : cfg_(cfg), master_(master_secret) {
    if (cfg_.key_len < 16) cfg_.key_len = 16;
    if (cfg_.rotate_after_ops == 0) cfg_.rotate_after_ops = 1;
    if (cfg_.rotate_after_ticks == 0) cfg_.rotate_after_ticks = 1;
    if (cfg_.start_epoch == 0) cfg_.start_epoch = 1;

    current_.epoch = cfg_.start_epoch;
    current_.key = derive(current_.epoch);
    current_.created_tick = 0;
    ring_[current_.epoch] = current_;
    order_.push_back(current_.epoch);
    log(event_kind::install, "epoch " + std::to_string(current_.epoch) + " installed");
}

bytes rotation_schedule::derive_labeled(const std::string& label, std::uint64_t epoch) const {
    bytes seed;
    seed.insert(seed.end(), master_.begin(), master_.end());
    for (int i = 0; i < 8; ++i) {
        seed.push_back(static_cast<std::uint8_t>((epoch >> (8 * i)) & 0xffu));
    }
    return handshake::derive_key_material(label, seed, cfg_.key_len);
}

bytes rotation_schedule::derive(std::uint64_t epoch) const {
    return derive_labeled("srsp-epoch", epoch);
}

void rotation_schedule::log(event_kind kind, const std::string& detail) {
    journal_.push_back(event{current_.epoch, kind, detail});
}

void rotation_schedule::tick(std::uint64_t amount) {
    tick_ += amount;
}

bool rotation_schedule::should_rotate() const {
    if (cfg_.mode == policy::fixed_epoch) {
        return current_.ops_served >= cfg_.rotate_after_ops;
    }
    if (cfg_.mode == policy::time_based) {
        return tick_ - epoch_started_tick_ >= cfg_.rotate_after_ticks;
    }
    // sliding_window rotates on either signal, whichever comes first.
    return current_.ops_served >= cfg_.rotate_after_ops ||
           tick_ - epoch_started_tick_ >= cfg_.rotate_after_ticks;
}

verdict rotation_schedule::force_rotate(std::uint64_t skip) {
    if (skip == 0) skip = 1;
    verdict v;
    const std::uint64_t next_epoch = current_.epoch + skip;
    key_epoch next;
    next.epoch = next_epoch;
    next.key = derive(next_epoch);
    next.created_tick = tick_;
    next.ops_served = 0;
    next.revoked = false;

    current_ = next;
    epoch_started_tick_ = tick_;
    ring_[next_epoch] = next;
    order_.push_back(next_epoch);
    ++rotations_;
    prune();

    v.ok = true;
    v.kind = event_kind::rotate;
    v.epoch = next_epoch;
    v.reason = "rotated to epoch " + std::to_string(next_epoch);
    log(event_kind::rotate, v.reason);
    return v;
}

verdict rotation_schedule::record_operation() {
    ++current_.ops_served;
    ring_[current_.epoch] = current_;
    if (should_rotate()) {
        return force_rotate(1);
    }
    verdict v;
    v.ok = true;
    v.kind = event_kind::accept;
    v.epoch = current_.epoch;
    v.reason = "operation served in epoch " + std::to_string(current_.epoch);
    return v;
}

std::size_t rotation_schedule::acceptance_window() const {
    switch (cfg_.mode) {
        case policy::fixed_epoch: return 1;
        case policy::time_based: return 1;
        case policy::sliding_window: return 1 + cfg_.overlap_epochs;
    }
    return 1;
}

verdict rotation_schedule::check(std::uint64_t epoch) const {
    verdict v;
    v.epoch = epoch;
    if (epoch == current_.epoch) {
        v.ok = true;
        v.kind = event_kind::accept;
        ++accepts_;
        v.reason = "epoch is current";
        return v;
    }
    if (epoch > current_.epoch) {
        v.kind = event_kind::reject_future;
        ++rejects_;
        v.reason = "epoch " + std::to_string(epoch) + " is ahead of current " +
                   std::to_string(current_.epoch);
        return v;
    }
    const std::uint64_t behind = current_.epoch - epoch;
    if (behind >= acceptance_window()) {
        v.kind = event_kind::reject_stale;
        ++rejects_;
        v.reason = "epoch " + std::to_string(epoch) + " is outside the acceptance window";
        return v;
    }
    auto it = ring_.find(epoch);
    if (it == ring_.end()) {
        v.kind = event_kind::reject_stale;
        ++rejects_;
        v.reason = "epoch key was pruned";
        return v;
    }
    if (it->second.revoked) {
        v.kind = event_kind::reject_stale;
        ++rejects_;
        v.reason = "epoch key was revoked";
        return v;
    }
    v.ok = true;
    v.kind = event_kind::accept;
    ++accepts_;
    v.reason = "epoch accepted inside overlap window";
    return v;
}

std::optional<key_epoch> rotation_schedule::get(std::uint64_t epoch) const {
    auto it = ring_.find(epoch);
    if (it == ring_.end()) return std::nullopt;
    if (it->second.revoked) return std::nullopt;
    return it->second;
}

std::vector<std::uint64_t> rotation_schedule::retained_epochs() const {
    std::vector<std::uint64_t> out;
    for (const auto& e : order_) {
        auto it = ring_.find(e);
        if (it != ring_.end() && !it->second.revoked) out.push_back(e);
    }
    std::sort(out.begin(), out.end());
    return out;
}

bool rotation_schedule::revoke(std::uint64_t epoch) {
    auto it = ring_.find(epoch);
    if (it == ring_.end()) return false;
    if (it->second.revoked) return false;
    it->second.revoked = true;
    if (epoch == current_.epoch) current_.revoked = true;
    log(event_kind::revoke, "epoch " + std::to_string(epoch) + " revoked");
    return true;
}

void rotation_schedule::prune() {
    const std::size_t keep = acceptance_window() + cfg_.overlap_epochs + 2;
    while (order_.size() > keep) {
        const std::uint64_t old = order_.front();
        order_.pop_front();
        if (old != current_.epoch) ring_.erase(old);
    }
}

void rotation_schedule::reset() {
    ring_.clear();
    order_.clear();
    journal_.clear();
    rotations_ = 0;
    rejects_ = 0;
    accepts_ = 0;
    tick_ = 0;
    epoch_started_tick_ = 0;
    current_ = key_epoch{};
    current_.epoch = cfg_.start_epoch;
    current_.key = derive(current_.epoch);
    ring_[current_.epoch] = current_;
    order_.push_back(current_.epoch);
    log(event_kind::install, "epoch " + std::to_string(current_.epoch) + " reinstalled");
}

}  // namespace keys
}  // namespace srsp

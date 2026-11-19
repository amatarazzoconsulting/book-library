// MIT License - (c) Anthony Matarazzo 2026
#include "spinlock.hpp"

namespace srsp {
namespace locks {

const char* to_string(acquire_result r) {
    switch (r) {
        case acquire_result::acquired: return "acquired";
        case acquire_result::busy: return "busy";
        case acquire_result::expired_reclaimed: return "expired_reclaimed";
        case acquire_result::invalid: return "invalid";
        case acquire_result::lease_too_long: return "lease_too_long";
    }
    return "unknown";
}

table::table(config cfg) : cfg_(cfg) {
    if (cfg_.default_lease == 0) cfg_.default_lease = 1;
    if (cfg_.max_lease < cfg_.default_lease) cfg_.max_lease = cfg_.default_lease;
}

bool table::is_expired(const lock_info& info, std::uint64_t now) const {
    return now >= info.lease_expiry;
}

acquire_result table::acquire(const std::string& resource, const std::string& session,
                              const std::string& node, std::uint64_t now,
                              std::uint64_t lease_ticks) {
    if (resource.empty() || session.empty() || node.empty()) {
        ++stats_.refused;
        return acquire_result::invalid;
    }
    const std::uint64_t lease = lease_ticks == 0 ? cfg_.default_lease : lease_ticks;
    if (lease > cfg_.max_lease) {
        ++stats_.refused;
        return acquire_result::lease_too_long;
    }
    acquire_result result = acquire_result::acquired;
    auto it = locks_.find(resource);
    if (it != locks_.end()) {
        if (!is_expired(it->second, now)) {
            ++stats_.refused;
            return acquire_result::busy;
        }
        ++stats_.expired;
        result = acquire_result::expired_reclaimed;
    }
    lock_info info;
    info.resource = resource;
    info.session = session;
    info.node = node;
    info.acquired_at = now;
    info.lease_expiry = now + lease;
    info.version = ++versions_[resource];
    locks_[resource] = info;
    ++stats_.acquired;
    return result;
}

bool table::release(const std::string& resource, const std::string& session, std::uint64_t now) {
    auto it = locks_.find(resource);
    if (it == locks_.end()) return false;
    if (is_expired(it->second, now)) {
        locks_.erase(it);
        ++stats_.expired;
        return false;
    }
    if (it->second.session != session) return false;  // only the owner may release
    locks_.erase(it);
    ++stats_.released;
    return true;
}

bool table::refresh(const std::string& resource, const std::string& session, std::uint64_t now,
                    std::uint64_t lease_ticks) {
    auto it = locks_.find(resource);
    if (it == locks_.end()) return false;
    if (is_expired(it->second, now)) {
        locks_.erase(it);
        ++stats_.expired;
        return false;
    }
    if (it->second.session != session) return false;
    const std::uint64_t lease = lease_ticks == 0 ? cfg_.default_lease : lease_ticks;
    if (lease > cfg_.max_lease) return false;
    it->second.lease_expiry = now + lease;
    ++stats_.refreshed;
    return true;
}

const lock_info* table::inspect(const std::string& resource, std::uint64_t now) {
    auto it = locks_.find(resource);
    if (it == locks_.end()) return nullptr;
    if (is_expired(it->second, now)) {
        if (cfg_.reclaim_on_inspect) {
            locks_.erase(it);
            ++stats_.expired;
        }
        return nullptr;
    }
    return &it->second;
}

std::size_t table::expire(std::uint64_t now) {
    std::size_t dropped = 0;
    for (auto it = locks_.begin(); it != locks_.end();) {
        if (is_expired(it->second, now)) {
            it = locks_.erase(it);
            ++dropped;
            ++stats_.expired;
        } else {
            ++it;
        }
    }
    return dropped;
}

bool table::held(const std::string& resource, std::uint64_t now) {
    return inspect(resource, now) != nullptr;
}

std::size_t table::held_count(std::uint64_t now) {
    expire(now);
    return locks_.size();
}

std::uint64_t table::version_of(const std::string& resource) const {
    auto it = versions_.find(resource);
    return it == versions_.end() ? 0 : it->second;
}

std::vector<std::string> table::resources() const {
    std::vector<std::string> out;
    out.reserve(locks_.size());
    for (const auto& kv : locks_) out.push_back(kv.first);
    return out;
}

}  // namespace locks
}  // namespace srsp

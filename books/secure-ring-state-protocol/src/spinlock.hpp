// MIT License - (c) Anthony Matarazzo 2026
// Lease based distributed locks. A holder must refresh before the lease expires;
// an expired lease is reclaimed automatically, so a crashed client cannot wedge a
// resource. Every acquisition bumps a monotonic version so stale holders can tell
// that they lost the lock.
#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace srsp {
namespace locks {

enum class acquire_result { acquired, busy, expired_reclaimed, invalid, lease_too_long };
const char* to_string(acquire_result r);

struct config {
    std::uint64_t default_lease = 30000;  // logical ticks
    std::uint64_t max_lease = 120000;     // ceiling for a requested lease
    bool reclaim_on_inspect = true;       // expire lazily when a lock is looked at
};

struct lock_info {
    std::string resource;
    std::string session;
    std::string node;
    std::uint64_t version = 0;
    std::uint64_t acquired_at = 0;
    std::uint64_t lease_expiry = 0;
};

struct stats {
    std::size_t acquired = 0;
    std::size_t refused = 0;
    std::size_t refreshed = 0;
    std::size_t released = 0;
    std::size_t expired = 0;
};

class table {
public:
    explicit table(config cfg = {});

    const config& settings() const { return cfg_; }

    // lease_ticks == 0 uses the default lease.
    acquire_result acquire(const std::string& resource, const std::string& session,
                           const std::string& node, std::uint64_t now,
                           std::uint64_t lease_ticks = 0);

    bool release(const std::string& resource, const std::string& session, std::uint64_t now);
    bool refresh(const std::string& resource, const std::string& session, std::uint64_t now,
                 std::uint64_t lease_ticks = 0);

    // Returns the current holder, or nullptr when free or expired.
    const lock_info* inspect(const std::string& resource, std::uint64_t now);

    // Reclaims every expired lease. Returns how many were dropped.
    std::size_t expire(std::uint64_t now);

    bool held(const std::string& resource, std::uint64_t now);
    std::size_t held_count(std::uint64_t now);
    std::uint64_t version_of(const std::string& resource) const;
    std::vector<std::string> resources() const;
    const stats& counters() const { return stats_; }

private:
    bool is_expired(const lock_info& info, std::uint64_t now) const;

    config cfg_;
    std::map<std::string, lock_info> locks_;
    std::map<std::string, std::uint64_t> versions_;
    stats stats_;
};

}  // namespace locks
}  // namespace srsp

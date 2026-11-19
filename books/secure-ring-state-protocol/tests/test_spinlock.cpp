// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>

#include "spinlock.hpp"

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}
using namespace srsp::locks;
}  // namespace

int main() {
    config cfg;
    cfg.default_lease = 100;
    cfg.max_lease = 200;
    table t(cfg);

    check(t.acquire("res", "s1", "n1", 0) == acquire_result::acquired, "first acquire wins");
    check(t.acquire("res", "s2", "n2", 0) == acquire_result::busy, "second acquire is refused");
    check(t.acquire("", "s1", "n1", 0) == acquire_result::invalid, "empty resource is invalid");
    check(t.acquire("res3", "s1", "n1", 0, 500) == acquire_result::lease_too_long,
          "a lease above max is refused");

    const auto* info = t.inspect("res", 10);
    check(info != nullptr && info->session == "s1", "inspect reports the holder");
    check(info->version == 1, "the first acquisition is version 1");
    check(info->lease_expiry == 100, "expiry is now plus the lease");

    check(t.refresh("res", "s2", 50) == false, "a non owner cannot refresh");
    check(t.refresh("res", "s1", 50), "the owner refreshes");
    check(t.inspect("res", 50)->lease_expiry == 150, "refresh extends the lease");

    check(!t.release("res", "s2", 60), "a non owner cannot release");
    check(t.release("res", "s1", 60), "the owner releases");
    check(t.inspect("res", 60) == nullptr, "the resource is free after release");

    check(t.acquire("res", "s1", "n1", 70) == acquire_result::acquired, "reacquire after release");
    check(t.version_of("res") == 2, "the version advanced");
    check(t.acquire("res", "s2", "n2", 500) == acquire_result::expired_reclaimed,
          "an expired lease is reclaimed");
    check(t.version_of("res") == 3, "reclaiming also bumps the version");

    check(t.held("res", 500), "the reclaimed lock is now held by the new owner");
    check(t.expire(1000) == 1, "expire drops the now stale lock");
    check(!t.held("res", 1000), "the lock is gone after expiry");

    const auto& c = t.counters();
    check(c.acquired == 3, "three acquisitions counted");
    check(c.refused == 3, "three refusals counted");
    check(c.expired >= 2, "expiries counted");

    std::cout << (failures == 0 ? "all spinlock tests passed\n" : "spinlock failures\n");
    return failures == 0 ? 0 : 1;
}

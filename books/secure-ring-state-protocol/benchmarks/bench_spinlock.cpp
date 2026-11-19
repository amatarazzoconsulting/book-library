// MIT License - (c) Anthony Matarazzo 2026
// Lock operations per second and expiry sweep throughput.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

#include "spinlock.hpp"

namespace {
using Clock = std::chrono::steady_clock;
double seconds(Clock::duration d) { return std::chrono::duration<double>(d).count(); }
using namespace srsp::locks;
}  // namespace

int main() {
    config cfg;
    cfg.default_lease = 1000;
    cfg.max_lease = 5000;
    table t(cfg);
    const int rounds = 200000;
    std::uint64_t now = 0;
    std::size_t acquired = 0;

    const auto t0 = Clock::now();
    for (int i = 0; i < rounds; ++i) {
        const std::string res = "res-" + std::to_string(i % 1000);
        if (t.acquire(res, "s1", "n1", now) == acquire_result::acquired) ++acquired;
        t.release(res, "s1", now);
        ++now;
    }
    const auto t1 = Clock::now();

    for (int i = 0; i < 1000; ++i) {
        t.acquire("held-" + std::to_string(i), "s1", "n1", now);
    }
    const int refresh_rounds = 200000;
    const auto t2 = Clock::now();
    for (int i = 0; i < refresh_rounds; ++i) {
        t.refresh("held-" + std::to_string(i % 1000), "s1", now);
    }
    const auto t3 = Clock::now();

    const auto t4 = Clock::now();
    const std::size_t dropped = t.expire(now + 5000);
    const auto t5 = Clock::now();

    std::cout << std::left << std::setw(28) << "metric" << std::setw(16) << "value" << "\n"
              << std::string(44, '-') << "\n"
              << std::setw(28) << "acquire+release pairs" << std::setw(16) << rounds << "\n"
              << std::setw(28) << "acquire+release per second" << std::setw(16) << std::fixed
              << std::setprecision(0) << (rounds / seconds(t1 - t0)) << "\n"
              << std::setw(28) << "refresh per second" << std::setw(16)
              << (refresh_rounds / seconds(t3 - t2)) << "\n"
              << std::setw(28) << "expire sweep per second" << std::setw(16)
              << (dropped / seconds(t5 - t4)) << "\n"
              << std::setw(28) << "locks reclaimed" << std::setw(16) << dropped << "\n"
              << std::setw(28) << "acquisitions counted" << std::setw(16) << acquired << "\n";
    return 0;
}

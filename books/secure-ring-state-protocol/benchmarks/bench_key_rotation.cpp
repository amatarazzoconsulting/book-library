// MIT License - (c) Anthony Matarazzo 2026
// Key derivation and epoch check throughput for each policy.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "key_rotation.hpp"

namespace {
using Clock = std::chrono::steady_clock;
using bytes = std::vector<std::uint8_t>;
double seconds(Clock::duration d) { return std::chrono::duration<double>(d).count(); }
using namespace srsp::keys;
}  // namespace

int main() {
    const bytes master(32, 0x5A);
    const int rounds = 50000;

    std::cout << std::left << std::setw(24) << "policy" << std::setw(14) << "derive/s"
              << std::setw(14) << "check/s" << std::setw(14) << "rotations" << "\n"
              << std::string(66, '-') << "\n";

    for (const auto mode : {policy::fixed_epoch, policy::time_based, policy::sliding_window}) {
        config cfg;
        cfg.mode = mode;
        cfg.rotate_after_ops = 1000;
        cfg.rotate_after_ticks = 100;
        cfg.overlap_epochs = 2;
        rotation_schedule sched(cfg, master);

        const auto t0 = Clock::now();
        std::size_t sink = 0;
        for (int i = 0; i < rounds; ++i) {
            sink += sched.derive(static_cast<std::uint64_t>(i % 64)).size();
        }
        const auto t1 = Clock::now();
        for (int i = 0; i < rounds; ++i) {
            sched.check(sched.current_epoch());
        }
        const auto t2 = Clock::now();
        for (int i = 0; i < 1000; ++i) {
            sched.record_operation();
            sched.tick(10);
        }
        const auto t3 = Clock::now();

        std::cout << std::left << std::setw(24) << to_string(mode) << std::setw(14)
                  << std::fixed << std::setprecision(0) << (rounds / seconds(t1 - t0))
                  << std::setw(14) << (rounds / seconds(t2 - t1))
                  << std::setw(14) << sched.rotations() << "\n";
        if (sink == 0) std::cout << "  WARNING: derivations returned nothing\n";
        (void)t3;
    }
    return 0;
}

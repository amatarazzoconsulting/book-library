// MIT License - (c) Anthony Matarazzo 2026
// Commit throughput for several ring sizes and the cost of early rejection.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

#include "consensus.hpp"

namespace {
using Clock = std::chrono::steady_clock;
double seconds(Clock::duration d) { return std::chrono::duration<double>(d).count(); }
using namespace srsp::consensus;
}  // namespace

int main() {
    const int proposals = 20000;
    std::cout << std::left << std::setw(12) << "ring" << std::setw(10) << "quorum"
              << std::setw(16) << "votes/s" << std::setw(18) << "commits/s"
              << std::setw(16) << "commit_ms" << "\n" << std::string(72, '-') << "\n";

    for (const std::size_t ring : {3u, 5u, 7u}) {
        config cfg;
        cfg.ring_size = ring;
        cfg.quorum = ring / 2 + 1;
        cfg.deadline = 1000000;
        engine e(cfg);
        std::string error;
        std::uint64_t clock = 0;

        const auto t0 = Clock::now();
        for (int i = 0; i < proposals; ++i) {
            const std::string id = "tx-" + std::to_string(i);
            e.propose(id, "payload-" + std::to_string(i), clock++, &error);
            for (std::size_t v = 0; v < cfg.quorum; ++v) {
                e.cast_vote(id, "node-" + std::to_string(v), vote_kind::accept, clock++, &error);
            }
        }
        const auto t1 = Clock::now();
        const double elapsed = seconds(t1 - t0);
        std::cout << std::left << std::setw(12) << ring << std::setw(10) << cfg.quorum
                  << std::setw(16) << std::fixed << std::setprecision(0)
                  << (e.votes_cast() / elapsed)
                  << std::setw(18) << (e.committed() / elapsed)
                  << std::setw(16) << std::setprecision(3)
                  << (elapsed * 1000.0 / static_cast<double>(proposals)) << "\n";
        if (e.committed() != static_cast<std::size_t>(proposals)) {
            std::cout << "  WARNING: not every proposal committed\n";
        }
    }

    config cfg;
    cfg.ring_size = 5;
    cfg.quorum = 3;
    engine e(cfg);
    std::string error;
    const auto t2 = Clock::now();
    for (int i = 0; i < proposals; ++i) {
        const std::string id = "bad-" + std::to_string(i);
        e.propose(id, "payload", 0, &error);
        for (std::size_t v = 0; v < 3; ++v) {
            e.cast_vote(id, "node-" + std::to_string(v), vote_kind::reject, 0, &error);
        }
    }
    const auto t3 = Clock::now();
    std::cout << "\nearly rejection/s       " << std::fixed << std::setprecision(0)
              << (proposals / seconds(t3 - t2)) << " (rejected " << e.rejected() << ")\n";
    return 0;
}

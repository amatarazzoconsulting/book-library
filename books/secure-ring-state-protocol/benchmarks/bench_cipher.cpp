// MIT License - (c) Anthony Matarazzo 2026
// Envelope throughput by payload size and seeding cost.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "cipher.hpp"
#include "sha256.hpp"

namespace {
using Clock = std::chrono::steady_clock;
using bytes = std::vector<std::uint8_t>;
double seconds(Clock::duration d) { return std::chrono::duration<double>(d).count(); }
}  // namespace

int main() {
    const bytes key(32, 0x5A);
    const bytes aad{'s', 'r', 's', 'p'};

    std::cout << std::left << std::setw(14) << "payload" << std::setw(14) << "rounds"
              << std::setw(16) << "seal MB/s" << std::setw(16) << "open MB/s"
              << std::setw(14) << "overhead" << "\n" << std::string(74, '-') << "\n";

    for (const std::size_t size : {static_cast<std::size_t>(64), static_cast<std::size_t>(1024),
                                   static_cast<std::size_t>(16384), static_cast<std::size_t>(262144)}) {
        for (const std::size_t rounds : {static_cast<std::size_t>(1000), static_cast<std::size_t>(10000)}) {
            srsp::cipher::config cfg;
            cfg.key_derivation_rounds = rounds;
            const bytes plain(size, 0x41);
            const int iterations = size > 100000 ? 6 : 40;

            const auto t0 = Clock::now();
            bytes sealed;
            for (int i = 0; i < iterations; ++i) sealed = srsp::cipher::seal(key, plain, aad, cfg);
            const auto t1 = Clock::now();
            bytes out;
            bool ok = true;
            for (int i = 0; i < iterations; ++i) ok = srsp::cipher::open(key, sealed, aad, out, cfg) && ok;
            const auto t2 = Clock::now();

            const double mb = static_cast<double>(size) * iterations / (1024.0 * 1024.0);
            std::cout << std::left << std::setw(14) << size << std::setw(14) << rounds
                      << std::setw(16) << std::fixed << std::setprecision(3)
                      << (mb / seconds(t1 - t0))
                      << std::setw(16) << (mb / seconds(t2 - t1))
                      << std::setw(14) << (sealed.size() - size) << (ok ? "" : "  ROUNDTRIP FAILED")
                      << "\n";
        }
    }

    // Baseline: the same payload with a plain HMAC, which is what the previous
    // XOR keystream path cost. Shows what the permutation pipeline buys and costs.
    const bytes plain(262144, 0x41);
    const int iterations = 200;
    const auto t3 = Clock::now();
    std::size_t sink = 0;
    for (int i = 0; i < iterations; ++i) {
        const auto tag = srsp::hash::hmac(key, plain);
        sink += tag[0];
    }
    const auto t4 = Clock::now();
    std::cout << "\nhmac-sha256 baseline   " << std::fixed << std::setprecision(1)
              << (static_cast<double>(plain.size()) * iterations / (1024.0 * 1024.0) /
                  seconds(t4 - t3)) << " MB/s (checksum " << sink << ")\n";
    return 0;
}

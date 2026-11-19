// MIT License - (c) Anthony Matarazzo 2026
// Append and verify throughput for the keyed and unkeyed chains.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "audit_log.hpp"

namespace {
using Clock = std::chrono::steady_clock;
using bytes = std::vector<std::uint8_t>;
double seconds(Clock::duration d) { return std::chrono::duration<double>(d).count(); }
}  // namespace

int main() {
    const int records = 20000;
    std::cout << std::left << std::setw(24) << "mode" << std::setw(12) << "records"
              << std::setw(16) << "append/s" << std::setw(16) << "verify/s"
              << std::setw(14) << "serialized" << "\n" << std::string(82, '-') << "\n";

    for (const bool keyed : {false, true}) {
        srsp::audit::options opt;
        opt.keyed = keyed;
        srsp::audit::audit_log log(opt);
        if (keyed) log.set_key(bytes(32, 0x42));

        const auto t0 = Clock::now();
        for (int i = 0; i < records; ++i) {
            log.append(srsp::audit::severity::info, "node-a", "put", "key-" + std::to_string(i));
        }
        const auto t1 = Clock::now();
        const auto report = log.verify();
        const auto t2 = Clock::now();
        const std::string text = log.serialize();

        std::cout << std::left << std::setw(24) << (keyed ? "keyed hmac" : "unkeyed hash")
                  << std::setw(12) << log.size() << std::setw(16) << std::fixed
                  << std::setprecision(0) << (records / seconds(t1 - t0))
                  << std::setw(16) << (report.checked / seconds(t2 - t1))
                  << std::setw(14) << text.size() << "\n";
        if (!report.ok) std::cout << "  WARNING: chain failed to verify\n";
    }

    srsp::audit::audit_log log;
    for (int i = 0; i < 1000; ++i) log.append(srsp::audit::severity::info, "a", "x", std::to_string(i));
    const int proof_rounds = 20000;
    const auto t3 = Clock::now();
    std::size_t sink = 0;
    for (int i = 0; i < proof_rounds; ++i) {
        const auto proof = log.inclusion_proof(static_cast<std::size_t>(i) % log.size());
        sink += proof.size();
    }
    const auto t4 = Clock::now();
    std::cout << "\ninclusion proofs/s      " << std::fixed << std::setprecision(0)
              << (proof_rounds / seconds(t4 - t3)) << " (checksum " << sink << ")\n";
    return 0;
}

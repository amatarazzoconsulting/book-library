// MIT License - (c) Anthony Matarazzo 2026
// Measures compression ratio and throughput for each codec on several payload shapes.
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "compression.hpp"

namespace {

using Clock = std::chrono::steady_clock;

std::vector<std::uint8_t> make(const std::string& kind, std::size_t target) {
    std::vector<std::uint8_t> out;
    out.reserve(target);
    std::uint32_t state = 12345;
    while (out.size() < target) {
        if (kind == "text") {
            const std::string text = "the container stores rows and the index keeps them sorted. ";
            out.insert(out.end(), text.begin(), text.end());
        } else if (kind == "runs") {
            for (int i = 0; i < 200; ++i) out.push_back(static_cast<std::uint8_t>('A' + (out.size() / 200) % 5));
        } else {
            state = state * 1664525u + 1013904223u;
            out.push_back(static_cast<std::uint8_t>(state >> 24));
        }
    }
    out.resize(target);
    return out;
}

double mb_per_sec(std::size_t bytes, double seconds) {
    if (seconds <= 0) return 0.0;
    return (static_cast<double>(bytes) / (1024.0 * 1024.0)) / seconds;
}

}  // namespace

int main() {
    const std::size_t target = 1024 * 1024;
    std::cout << std::left << std::setw(10) << "payload" << std::setw(8) << "codec"
              << std::setw(12) << "raw_bytes" << std::setw(12) << "packed" << std::setw(9)
              << "ratio" << std::setw(14) << "enc_MB/s" << std::setw(14) << "dec_MB/s" << "\n";
    std::cout << std::string(79, '-') << "\n";
    for (const char* kind : {"text", "runs", "random"}) {
        const auto data = make(kind, target);
        for (const auto& pair : std::vector<std::pair<const char*, libcontainer::Codec>>{
                 {"none", libcontainer::Codec::kNone},
                 {"rle", libcontainer::Codec::kRle},
                 {"lz77", libcontainer::Codec::kLz77},
                 {"auto", libcontainer::Codec::kAuto}}) {
            const auto t0 = Clock::now();
            const auto packed = libcontainer::compress(data, pair.second);
            const auto t1 = Clock::now();
            const auto back = libcontainer::decompress(packed, pair.second);
            const auto t2 = Clock::now();
            const double enc = std::chrono::duration<double>(t1 - t0).count();
            const double dec = std::chrono::duration<double>(t2 - t1).count();
            const bool ok = back == data;
            std::cout << std::left << std::setw(10) << kind << std::setw(8) << pair.first
                      << std::setw(12) << data.size() << std::setw(12) << packed.size()
                      << std::setw(9) << std::fixed << std::setprecision(3)
                      << (data.empty() ? 0.0 : static_cast<double>(packed.size()) / data.size())
                      << std::setw(14) << std::setprecision(1) << mb_per_sec(data.size(), enc)
                      << std::setw(14) << mb_per_sec(data.size(), dec)
                      << (ok ? "" : "  MISMATCH") << "\n";
        }
    }
    return 0;
}

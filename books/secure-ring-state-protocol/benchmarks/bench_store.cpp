// MIT License - (c) Anthony Matarazzo 2026
// Key-value operation throughput, scan cost and watch overhead.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "store.hpp"

namespace {
using Clock = std::chrono::steady_clock;
double seconds(Clock::duration d) { return std::chrono::duration<double>(d).count(); }
using namespace srsp::store;
}  // namespace

int main() {
    kv_store db;
    const int rows = 200000;
    const auto t0 = Clock::now();
    for (int i = 0; i < rows; ++i) {
        db.put("user:" + std::to_string(100000 + i), "value-" + std::to_string(i),
               static_cast<std::uint64_t>(i));
    }
    const auto t1 = Clock::now();

    std::string value;
    const auto t2 = Clock::now();
    std::size_t sink = 0;
    for (int i = 0; i < rows; ++i) {
        if (db.get("user:" + std::to_string(100000 + i), &value)) sink += value.size();
    }
    const auto t3 = Clock::now();

    const int scans = 2000;
    const auto t4 = Clock::now();
    for (int i = 0; i < scans; ++i) sink += db.scan("user:1001", 100).size();
    const auto t5 = Clock::now();

    std::size_t notified = 0;
    db.watch("user:100000", [&notified](const std::string&, const std::string&, bool) { ++notified; });
    const int watched_writes = 20000;
    const auto t6 = Clock::now();
    for (int i = 0; i < watched_writes; ++i) db.put("user:100000", "v" + std::to_string(i), 1);
    const auto t7 = Clock::now();

    const int batches = 2000;
    const auto t8 = Clock::now();
    for (int i = 0; i < batches; ++i) {
        std::string error;
        db.batch({{op_kind::put, "b:" + std::to_string(i), "x"},
                  {op_kind::put, "b2:" + std::to_string(i), "y"}}, 2, &error);
    }
    const auto t9 = Clock::now();

    std::cout << std::left << std::setw(30) << "operation" << std::setw(12) << "calls"
              << std::setw(16) << "per second" << std::setw(14) << "per_op_us" << "\n"
              << std::string(72, '-') << "\n";
    auto row = [](const char* name, std::size_t calls, double sec) {
        std::cout << std::left << std::setw(30) << name << std::setw(12) << calls
                  << std::setw(16) << std::fixed << std::setprecision(0)
                  << (sec > 0 ? calls / sec : 0.0) << std::setw(14) << std::setprecision(3)
                  << (calls ? sec * 1e6 / static_cast<double>(calls) : 0.0) << "\n";
    };
    row("put", rows, seconds(t1 - t0));
    row("get", rows, seconds(t3 - t2));
    row("scan prefix limit 100", scans, seconds(t5 - t4));
    row("put with one watcher", watched_writes, seconds(t7 - t6));
    row("batch of two ops", batches, seconds(t9 - t8));

    std::cout << "\nentries=" << db.size() << " notifications=" << notified
              << " checksum=" << sink << "\n";
    return 0;
}

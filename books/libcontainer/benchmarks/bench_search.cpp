// MIT License - (c) Anthony Matarazzo 2026
// Measures container throughput: insert, point lookup, prefix search and fuzzy search.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "container.hpp"
#include "search_index.hpp"

namespace {

using Clock = std::chrono::steady_clock;

std::string key_for(int i) {
    std::string k = "key-";
    k += std::to_string(100000 + i);
    return k;
}

}  // namespace

int main() {
    const int count = 200000;
    libcontainer::Container db;
    const auto t0 = Clock::now();
    for (int i = 0; i < count; ++i) db.insert(key_for(i), "payload-" + std::to_string(i));
    const auto t1 = Clock::now();
    const double insert_s = std::chrono::duration<double>(t1 - t0).count();

    std::size_t sink = 0;
    const auto t2 = Clock::now();
    for (int i = 0; i < count; ++i) {
        const auto* e = db.find(key_for(i));
        if (e) sink += e->payload.size();
    }
    const auto t3 = Clock::now();
    const double find_s = std::chrono::duration<double>(t3 - t2).count();

    const auto t4 = Clock::now();
    std::size_t prefix_hits = 0;
    for (int i = 0; i < 2000; ++i) prefix_hits += db.search().prefix("key-1").size();
    const auto t5 = Clock::now();
    const double prefix_s = std::chrono::duration<double>(t5 - t4).count();

    const auto t6 = Clock::now();
    std::size_t fuzzy_hits = 0;
    for (int i = 0; i < 200; ++i) fuzzy_hits += db.search().fuzzy("key-100007", 1).size();
    const auto t7 = Clock::now();
    const double fuzzy_s = std::chrono::duration<double>(t7 - t6).count();

    std::cout << std::left << std::setw(26) << "operation" << std::setw(12) << "count"
              << std::setw(14) << "total_ms" << std::setw(16) << "per_op_us" << "\n";
    std::cout << std::string(68, '-') << "\n";
    auto row = [](const char* name, std::size_t n, double seconds) {
        std::cout << std::left << std::setw(26) << name << std::setw(12) << n
                  << std::setw(14) << std::fixed << std::setprecision(2) << seconds * 1000.0
                  << std::setw(16) << std::setprecision(3)
                  << (n ? seconds * 1e6 / static_cast<double>(n) : 0.0) << "\n";
    };
    row("insert", static_cast<std::size_t>(count), insert_s);
    row("point lookup", static_cast<std::size_t>(count), find_s);
    row("prefix search (key-1)", 2000, prefix_s);
    row("fuzzy search (d=1)", 200, fuzzy_s);
    std::cout << "\nentries=" << db.size() << " checksum=" << sink
              << " prefix_hits=" << prefix_hits << " fuzzy_hits=" << fuzzy_hits << "\n";
    return 0;
}

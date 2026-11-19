// MIT License - (c) Anthony Matarazzo 2026
// Measures snapshot size and round-trip time with and without payload compression.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

#include "container.hpp"
#include "serialization.hpp"

namespace {

using Clock = std::chrono::steady_clock;

}  // namespace

int main() {
    libcontainer::Container db;
    for (int i = 0; i < 50000; ++i) {
        std::string payload;
        for (int r = 0; r < 4; ++r) payload += "row " + std::to_string(i) + " value repeated ";
        db.insert("key-" + std::to_string(i), payload);
    }
    std::size_t raw = 0;
    db.visit_all([&](const std::string& k, const libcontainer::Entry& e) {
        raw += k.size() + e.payload.size();
    });

    std::cout << std::left << std::setw(22) << "mode" << std::setw(14) << "raw_bytes"
              << std::setw(14) << "snapshot" << std::setw(9) << "ratio" << std::setw(12)
              << "ser_ms" << std::setw(12) << "de_ms" << "\n";
    std::cout << std::string(83, '-') << "\n";
    for (const bool pack : {false, true}) {
        const auto t0 = Clock::now();
        const auto snap = libcontainer::serialize(db, pack);
        const auto t1 = Clock::now();
        libcontainer::Container restored;
        libcontainer::deserialize(snap, &restored);
        const auto t2 = Clock::now();
        std::cout << std::left << std::setw(22) << (pack ? "compressed payloads" : "verbatim")
                  << std::setw(14) << raw << std::setw(14) << snap.size()
                  << std::setw(9) << std::fixed << std::setprecision(3)
                  << (raw ? static_cast<double>(snap.size()) / raw : 0.0)
                  << std::setw(12) << std::setprecision(1)
                  << std::chrono::duration<double>(t1 - t0).count() * 1000.0
                  << std::setw(12) << std::chrono::duration<double>(t2 - t1).count() * 1000.0
                  << " restored=" << restored.size() << "\n";
    }
    return 0;
}

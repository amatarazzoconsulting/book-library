// MIT License - (c) Anthony Matarazzo 2026
// Measures load throughput and the cost of indexed versus scanned access.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

#include "query.hpp"
#include "schema.hpp"
#include "table.hpp"

namespace {
using Clock = std::chrono::steady_clock;
double ms(Clock::duration d) { return std::chrono::duration<double>(d).count() * 1000.0; }
}  // namespace

int main() {
    const char* schema_text =
        "Table Bench\nColumns\nID Integer Key\nName Text\nCategory Text\nScore Decimal\n***\n";
    webtable::Schema schema;
    std::string err;
    schema.parse(schema_text, &err);

    const int rows = 200000;
    std::string data;
    data.reserve(static_cast<std::size_t>(rows) * 40);
    for (int i = 0; i < rows; ++i) {
        data += std::to_string(i) + ",item-" + std::to_string(i) + ",cat-" + std::to_string(i % 20) +
                "," + std::to_string((i % 1000) / 10.0) + "\n";
    }

    webtable::Table table(schema);
    const auto t0 = Clock::now();
    const std::size_t loaded = table.load(data, &err);
    const auto t1 = Clock::now();
    const double load_ms = ms(t1 - t0);

    std::size_t sink = 0;
    const auto t2 = Clock::now();
    for (int i = 0; i < 100000; ++i) {
        const auto* row = table.find_by_key(std::to_string(i));
        if (row) sink += row->values[1].text.size();
    }
    const auto t3 = Clock::now();
    const double key_ms = ms(t3 - t2);

    const auto t4 = Clock::now();
    for (int i = 0; i < 20; ++i) {
        table.visit_text_contains("Name", "item-99", [&](const webtable::Row&) { ++sink; });
    }
    const auto t5 = Clock::now();
    const double scan_ms = ms(t5 - t4);

    table.build_index("Category");
    const auto t6 = Clock::now();
    for (int i = 0; i < 2000; ++i) {
        table.visit_index("Category", "cat-" + std::to_string(i % 20), [&](const webtable::Row&) { ++sink; });
    }
    const auto t7 = Clock::now();
    const double index_ms = ms(t7 - t6);

    const auto t8 = Clock::now();
    for (int i = 0; i < 20; ++i) webtable::fuzzy_rows(table, "Name", "item-9999", 1);
    const auto t9 = Clock::now();
    const double fuzzy_ms = ms(t9 - t8);

    std::cout << std::left << std::setw(34) << "operation" << std::setw(10) << "calls"
              << std::setw(12) << "total_ms" << std::setw(14) << "per_call_us" << "\n";
    std::cout << std::string(70, '-') << "\n";
    auto row = [](const char* name, std::size_t calls, double total) {
        std::cout << std::left << std::setw(34) << name << std::setw(10) << calls << std::setw(12)
                  << std::fixed << std::setprecision(2) << total << std::setw(14)
                  << std::setprecision(3) << (calls ? total * 1000.0 / static_cast<double>(calls) : 0.0)
                  << "\n";
    };
    row("load rows from text", loaded, load_ms);
    row("find_by_key", 100000, key_ms);
    row("visit_text_contains (full scan)", 20, scan_ms);
    row("visit_index on Category", 2000, index_ms);
    row("fuzzy_rows distance 1", 20, fuzzy_ms);
    std::cout << "\nrows=" << table.size() << " bytes_loaded=" << data.size()
              << " load_rows_per_sec=" << std::fixed << std::setprecision(0)
              << (loaded / (load_ms / 1000.0)) << " checksum=" << sink << "\n";
    return 0;
}

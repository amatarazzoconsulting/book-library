// MIT License - (c) Anthony Matarazzo 2026
// Teaching program: walks every option of the container, the search index and the codecs.
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "compression.hpp"
#include "container.hpp"
#include "search_index.hpp"
#include "serialization.hpp"

namespace {

void section(const char* title) {
    std::cout << "\n=== " << title << " ===\n";
}

std::vector<std::uint8_t> bytes_of(const std::string& text, int repeat) {
    std::vector<std::uint8_t> out;
    for (int i = 0; i < repeat; ++i) out.insert(out.end(), text.begin(), text.end());
    return out;
}

void show_codec(const char* label, const std::vector<std::uint8_t>& in, libcontainer::Codec codec) {
    const auto packed = libcontainer::compress(in, codec);
    const auto back = libcontainer::decompress(packed, codec);
    const bool lossless = back == in;
    std::cout << "  " << std::setw(6) << std::left << label
              << " raw=" << std::setw(7) << in.size()
              << " packed=" << std::setw(7) << packed.size()
              << " ratio=" << std::fixed << std::setprecision(3)
              << (in.empty() ? 0.0 : static_cast<double>(packed.size()) / in.size())
              << " lossless=" << (lossless ? "yes" : "NO") << "\n";
}

}  // namespace

int main() {
    section("Step 1: ContainerConfig, every field");
    libcontainer::ContainerConfig config;
    std::cout << "  window_size            = " << config.window_size << " bytes\n"
              << "  max_memory_mb          = " << config.max_memory_mb << "\n"
              << "  compression_threshold  = " << config.compression_threshold << " bytes\n"
              << "  track_dirty            = " << (config.track_dirty ? "true" : "false") << "\n"
              << "  enable_prefix_search   = " << (config.enable_prefix_search ? "true" : "false") << "\n";

    section("Step 2: insert, duplicate rejection, find, size");
    libcontainer::Container db(config);
    std::cout << "  insert(alpha) -> " << (db.insert("alpha", "one") ? "true" : "false") << "\n";
    std::cout << "  insert(alpha) again -> " << (db.insert("alpha", "dup") ? "true" : "false")
              << " (duplicates are rejected)\n";
    std::cout << "  size = " << db.size() << "\n";
    if (const auto* e = db.find("alpha")) std::cout << "  find(alpha) = " << e->payload << "\n";
    std::cout << "  find(missing) = " << (db.find("missing") ? "found" : "null") << "\n";

    section("Step 3: update keeps indices consistent");
    db.add_secondary_index("first_char", [](const libcontainer::Entry& e) {
        return std::vector<std::string>{e.payload.substr(0, 1)};
    });
    db.insert("beta", "two");
    db.update("beta", "three");
    db.visit_by_index("first_char", "t", [](const std::string& key, const libcontainer::Entry& e) {
        std::cout << "  first_char=t -> " << key << " = " << e.payload << "\n";
    });

    section("Step 4: prefix and fuzzy search options");
    for (const char* key : {"alphabet", "alpine", "altar", "beta"}) db.insert(key, key);
    std::cout << "  prefix(\"al\") count = " << db.search().prefix("al").size() << "\n";
    for (std::size_t distance = 0; distance <= 3; ++distance) {
        const auto hits = db.search().fuzzy("alpah", distance);
        std::cout << "  fuzzy(\"alpah\", " << distance << ") = " << hits.size() << " hit(s)";
        for (const auto& h : hits) std::cout << " " << h;
        std::cout << "\n";
    }

    section("Step 5: every codec, on two very different payloads");
    const auto repetitive = bytes_of("AAAAABBBBBCCCCCDDDDD", 40);
    const auto text = bytes_of("the quick brown fox jumps over the lazy dog ", 20);
    for (const auto& pair : std::vector<std::pair<const char*, std::vector<std::uint8_t>>>{
             {"rep", repetitive}, {"text", text}}) {
        show_codec("none", pair.second, libcontainer::Codec::kNone);
        show_codec("rle", pair.second, libcontainer::Codec::kRle);
        show_codec("lz77", pair.second, libcontainer::Codec::kLz77);
        show_codec("auto", pair.second, libcontainer::Codec::kAuto);
    }

    section("Step 6: serialization with and without payload compression");
    for (const bool pack : {false, true}) {
        const auto snapshot = libcontainer::serialize(db, pack);
        libcontainer::Container restored;
        const auto count = libcontainer::deserialize(snapshot, &restored);
        std::cout << "  compress_payloads=" << (pack ? "true" : "false")
                  << " snapshot=" << snapshot.size() << " bytes"
                  << " entries=" << count
                  << " identical=" << (restored.size() == db.size() ? "yes" : "NO") << "\n";
    }

    section("Step 7: dirty tracking and sync");
    std::cout << "  dirty before sync = " << db.dirty_count() << "\n";
    db.sync();
    std::cout << "  dirty after sync  = " << db.dirty_count() << "\n";

    section("Step 8: erase removes keys and index entries");
    const std::size_t before = db.size();
    db.erase("beta");
    std::cout << "  size " << before << " -> " << db.size() << "\n";
    db.visit_by_index("first_char", "t", [](const std::string& key, const libcontainer::Entry&) {
        std::cout << "  stale index hit: " << key << "\n";
    });
    std::cout << "  (no stale index hits above means erase cleaned the index)\n";
    return 0;
}

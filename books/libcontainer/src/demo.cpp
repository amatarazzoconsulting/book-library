// MIT License - (c) Anthony Matarazzo 2026
// Demo: a small people database built from the container, its indices and its codecs.
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "compression.hpp"
#include "container.hpp"
#include "search_index.hpp"
#include "serialization.hpp"

namespace {

std::string record(const std::string& name, const std::string& city, int age) {
    return name + "|" + city + "|" + std::to_string(age);
}

std::string field(const std::string& payload, std::size_t index) {
    std::size_t start = 0;
    for (std::size_t n = 0; n < index; ++n) {
        const std::size_t bar = payload.find('|', start);
        if (bar == std::string::npos) return "";
        start = bar + 1;
    }
    const std::size_t end = payload.find('|', start);
    return payload.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

}  // namespace

int main() {
    libcontainer::ContainerConfig config;
    config.compression_threshold = 32;
    libcontainer::Container db(config);

    db.insert("alice", record("Alice Johnson", "New York", 30));
    db.insert("bob", record("Bob Smith", "Los Angeles", 25));
    db.insert("charlie", record("Charlie Brown", "Chicago", 35));
    db.insert("dana", record("Dana White", "New York", 41));

    std::cout << "== primary keys ==\n";
    db.visit_all([](const std::string& key, const libcontainer::Entry& e) {
        std::cout << "  " << std::setw(8) << std::left << key << e.payload << "\n";
    });

    std::cout << "\n== secondary index on city ==\n";
    db.add_secondary_index("city", [](const libcontainer::Entry& e) {
        return std::vector<std::string>{field(e.payload, 1)};
    });
    db.visit_by_index("city", "New York", [](const std::string& key, const libcontainer::Entry&) {
        std::cout << "  New York -> " << key << "\n";
    });

    std::cout << "\n== prefix search ==\n";
    for (const auto& key : db.search().prefix("ch")) std::cout << "  ch* -> " << key << "\n";

    std::cout << "\n== fuzzy search, distance 2 of 'alise' ==\n";
    db.visit_fuzzy("alise", 2, [](const std::string& key, const libcontainer::Entry&) {
        std::cout << "  match -> " << key << "\n";
    });

    std::cout << "\n== compression of a repetitive payload ==\n";
    std::vector<std::uint8_t> sample;
    for (int i = 0; i < 400; ++i) {
        const std::string text = "the container stores this line repeatedly\n";
        sample.insert(sample.end(), text.begin(), text.end());
    }
    libcontainer::Codec chosen = libcontainer::Codec::kNone;
    const auto stats = libcontainer::choose_codec(sample, &chosen);
    std::cout << "  raw " << stats.input_bytes << " bytes, packed " << stats.output_bytes
              << " bytes, ratio " << std::fixed << std::setprecision(3) << stats.ratio()
              << ", codec " << static_cast<int>(chosen) << "\n";

    std::cout << "\n== snapshot round trip ==\n";
    const auto snapshot = libcontainer::serialize(db, true);
    libcontainer::Container restored;
    const std::size_t count = libcontainer::deserialize(snapshot, &restored);
    std::cout << "  snapshot " << snapshot.size() << " bytes, restored " << count << " entries\n";
    if (const auto* e = restored.find("charlie")) {
        std::cout << "  charlie -> " << e->payload << "\n";
    }
    std::cout << "  dirty before sync: " << db.dirty_count();
    db.sync();
    std::cout << ", after sync: " << db.dirty_count() << "\n";
    return 0;
}

// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>
#include <vector>

#include "container.hpp"
#include "serialization.hpp"

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}

}  // namespace

int main() {
    libcontainer::Container db;
    check(db.insert("a", "alpha"), "insert accepts a new key");
    check(!db.insert("a", "again"), "insert rejects a duplicate key");
    check(db.size() == 1, "size reflects one entry");
    check(db.find("a") != nullptr && db.find("a")->payload == "alpha", "find returns the payload");
    check(db.find("nope") == nullptr, "find returns null for a missing key");

    check(db.update("a", "beta"), "update replaces an existing payload");
    check(!db.update("nope", "x"), "update rejects a missing key");
    check(db.find("a")->payload == "beta", "payload changed after update");

    db.insert("b", "bravo");
    db.insert("c", "charlie");
    std::size_t visited = 0;
    db.visit_all([&](const std::string&, const libcontainer::Entry&) { ++visited; });
    check(visited == 3, "visit_all visits every entry");

    db.add_secondary_index("initial", [](const libcontainer::Entry& e) {
        return std::vector<std::string>{e.payload.substr(0, 1)};
    });
    std::size_t hits = 0;
    db.visit_by_index("initial", "b", [&](const std::string& key, const libcontainer::Entry&) {
        ++hits;
        check(key == "a" || key == "b", "index hit points at a real key");
    });
    check(hits == 2, "secondary index finds both beta and bravo");

    db.update("a", "zulu");
    hits = 0;
    db.visit_by_index("initial", "b", [&](const std::string&, const libcontainer::Entry&) { ++hits; });
    check(hits == 1, "update moved the entry out of the old index bucket");

    check(db.search().prefix("b").size() == 1, "prefix search finds bravo");
    db.erase("b");
    check(db.size() == 2 && db.search().prefix("b").empty(), "erase clears store and index");

    const auto snapshot = libcontainer::serialize(db, true);
    libcontainer::Container restored;
    const auto count = libcontainer::deserialize(snapshot, &restored);
    check(count == 2 && restored.size() == 2, "snapshot restores every entry");
    check(restored.find("a") != nullptr && restored.find("a")->payload == "zulu",
          "restored payload matches");

    bool threw = false;
    try {
        std::vector<std::uint8_t> broken(snapshot);
        broken[0] = 'X';
        libcontainer::Container sink;
        libcontainer::deserialize(broken, &sink);
    } catch (const std::exception&) {
        threw = true;
    }
    check(threw, "deserialize rejects a bad magic header");

    std::cout << (failures == 0 ? "all container tests passed\n" : "container failures\n");
    return failures == 0 ? 0 : 1;
}

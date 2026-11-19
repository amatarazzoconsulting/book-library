// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>
#include <vector>

#include "store.hpp"

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}
using namespace srsp::store;
}  // namespace

int main() {
    kv_store db;
    std::string value;
    std::uint64_t version = 0;

    check(!db.get("missing", &value), "get on a missing key fails");
    check(!db.remove("missing", 0), "remove on a missing key fails");
    check(!db.put("", "x", 0), "an empty key is refused");

    check(db.put("k1", "one", 1), "put stores a value");
    check(db.get("k1", &value, &version) && value == "one", "get returns the value");
    check(version == 1, "the first write is version 1");
    check(db.exists("k1"), "exists reports the key");
    check(!db.exists("k2"), "exists is false for an absent key");

    db.put("k1", "two", 2);
    check(db.get("k1", &value, &version) && value == "two" && version == 2,
          "overwrite bumps the version");

    db.put("k2", "b", 3);
    db.put("k3", "c", 4);
    db.put("other", "d", 5);
    const auto k = db.scan("k", 10);
    check(k.size() == 3, "scan returns the prefixed keys only");
    check(k.front().first == "k1" && k.back().first == "k3", "scan is lexicographic");
    check(db.scan("k", 2).size() == 2, "scan honours the limit");
    check(db.scan("zz", 10).empty(), "scan of an unknown prefix is empty");

    std::size_t fired = 0;
    std::string last_key;
    const auto token = db.watch("k1", [&](const std::string& key, const std::string&, bool) {
        ++fired;
        last_key = key;
    });
    db.put("k1", "three", 6);
    check(fired == 1 && last_key == "k1", "a watch fires for its key");
    db.put("k2", "changed", 7);
    check(fired == 1, "a watch ignores other keys");
    check(db.unwatch(token), "unwatch removes the callback");
    db.put("k1", "four", 8);
    check(fired == 1, "no callback after unwatch");
    check(!db.unwatch(token), "unwatching twice fails");

    std::string error;
    check(!db.batch({}, 9, &error), "an empty batch is refused");
    const std::size_t before = db.size();
    check(!db.batch({{op_kind::put, "k9", "nine"}, {op_kind::remove, "absent", ""}}, 10, &error),
          "a batch with a bad remove is rejected");
    check(db.size() == before && !db.exists("k9"), "the rejected batch changed nothing");

    check(db.batch({{op_kind::put, "k9", "nine"}, {op_kind::remove, "k2", ""}}, 11, &error),
          "a valid batch applies");
    check(db.exists("k9") && !db.exists("k2"), "both operations of the batch took effect");
    check(db.counters().rejected_batches == 2, "rejected batches are counted");

    std::cout << (failures == 0 ? "all store tests passed\n" : "store failures\n");
    return failures == 0 ? 0 : 1;
}

// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>
#include <vector>

#include "search_index.hpp"

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}

}  // namespace

int main() {
    libcontainer::SearchIndex index;
    for (const char* key : {"alice", "alicia", "bob", "charlie", "dana"}) index.insert(key);
    check(index.size() == 5, "size counts inserted keys");
    check(index.contains("alice"), "contains finds an inserted key");
    check(!index.contains("alic"), "contains rejects a prefix that is not a key");

    index.insert("alice");
    check(index.size() == 5, "duplicate insert does not change size");

    const auto a = index.prefix("ali");
    check(a.size() == 2 && a[0] == "alice" && a[1] == "alicia", "prefix returns sorted matches");
    check(index.prefix("zz").empty(), "prefix returns empty for unknown prefix");
    check(index.prefix("").size() == 5, "empty prefix returns every key");

    check(libcontainer::SearchIndex::distance("kitten", "sitting", 5) == 3, "levenshtein kitten/sitting");
    check(libcontainer::SearchIndex::distance("same", "same", 0) == 0, "levenshtein identical");
    check(libcontainer::SearchIndex::distance("abc", "xyz", 1) == 2,
          "levenshtein returns bound + 1 when exceeded");

    const auto fuzzy = index.fuzzy("alise", 2);
    check(!fuzzy.empty() && fuzzy[0] == "alice", "fuzzy finds the closest key first");

    index.erase("bob");
    check(!index.contains("bob") && index.size() == 4, "erase removes a key");

    std::cout << (failures == 0 ? "all search index tests passed\n" : "search index failures\n");
    return failures == 0 ? 0 : 1;
}

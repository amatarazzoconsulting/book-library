// MIT License - (c) Anthony Matarazzo 2026
// Prefix trie and bounded edit-distance search used by the container query layer.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace libcontainer {

class SearchIndex {
public:
    void insert(const std::string& key);
    void erase(const std::string& key);
    bool contains(const std::string& key) const;

    // All keys that start with the given prefix, in lexicographic order.
    std::vector<std::string> prefix(const std::string& prefix) const;

    // Keys within max_distance edits of the query, closest first.
    std::vector<std::string> fuzzy(const std::string& query, std::size_t max_distance) const;

    std::size_t size() const { return count_; }

    // Bounded Levenshtein distance. Returns max_distance + 1 when the distance
    // exceeds the bound, which keeps the fuzzy scan cheap.
    static std::size_t distance(const std::string& a, const std::string& b, std::size_t bound);

private:
    struct Node {
        bool terminal = false;
        std::vector<std::pair<char, int>> children;
    };
    std::vector<Node> nodes_{{}};  // node 0 is the root
    std::size_t count_ = 0;

    int child_of(int node, char c) const;
    int ensure_child(int node, char c);
    void collect(int node, std::string& path, std::vector<std::string>& out) const;
};

}  // namespace libcontainer

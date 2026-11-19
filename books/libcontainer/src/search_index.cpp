// MIT License - (c) Anthony Matarazzo 2026
#include "search_index.hpp"

#include <algorithm>
#include <limits>

namespace libcontainer {

int SearchIndex::child_of(int node, char c) const {
    for (const auto& kv : nodes_[static_cast<std::size_t>(node)].children) {
        if (kv.first == c) return kv.second;
    }
    return -1;
}

int SearchIndex::ensure_child(int node, char c) {
    const int existing = child_of(node, c);
    if (existing >= 0) return existing;
    nodes_.push_back(Node{});
    const int idx = static_cast<int>(nodes_.size()) - 1;
    nodes_[static_cast<std::size_t>(node)].children.emplace_back(c, idx);
    return idx;
}

void SearchIndex::insert(const std::string& key) {
    int node = 0;
    for (const char c : key) node = ensure_child(node, c);
    if (!nodes_[static_cast<std::size_t>(node)].terminal) {
        nodes_[static_cast<std::size_t>(node)].terminal = true;
        ++count_;
    }
}

bool SearchIndex::contains(const std::string& key) const {
    int node = 0;
    for (const char c : key) {
        node = child_of(node, c);
        if (node < 0) return false;
    }
    return nodes_[static_cast<std::size_t>(node)].terminal;
}

void SearchIndex::erase(const std::string& key) {
    int node = 0;
    for (const char c : key) {
        node = child_of(node, c);
        if (node < 0) return;
    }
    if (nodes_[static_cast<std::size_t>(node)].terminal) {
        nodes_[static_cast<std::size_t>(node)].terminal = false;
        --count_;
    }
}

void SearchIndex::collect(int node, std::string& path, std::vector<std::string>& out) const {
    const Node& n = nodes_[static_cast<std::size_t>(node)];
    if (n.terminal) out.push_back(path);
    for (const auto& kv : n.children) {
        path.push_back(kv.first);
        collect(kv.second, path, out);
        path.pop_back();
    }
}

std::vector<std::string> SearchIndex::prefix(const std::string& prefix) const {
    std::vector<std::string> out;
    int node = 0;
    for (const char c : prefix) {
        node = child_of(node, c);
        if (node < 0) return out;
    }
    std::string path = prefix;
    collect(node, path, out);
    std::sort(out.begin(), out.end());
    return out;
}

std::size_t SearchIndex::distance(const std::string& a, const std::string& b, std::size_t bound) {
    const std::size_t inf = bound + 1;
    if (a.size() > b.size() + bound || b.size() > a.size() + bound) return inf;
    std::vector<std::size_t> prev(b.size() + 1), cur(b.size() + 1);
    for (std::size_t j = 0; j <= b.size(); ++j) prev[j] = j;
    for (std::size_t i = 1; i <= a.size(); ++i) {
        cur[0] = i;
        std::size_t row_min = cur[0];
        for (std::size_t j = 1; j <= b.size(); ++j) {
            const std::size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
            row_min = std::min(row_min, cur[j]);
        }
        if (row_min > bound) return inf;
        prev.swap(cur);
    }
    return prev[b.size()] > bound ? inf : prev[b.size()];
}

std::vector<std::string> SearchIndex::fuzzy(const std::string& query, std::size_t max_distance) const {
    std::vector<std::pair<std::size_t, std::string>> scored;
    std::vector<std::string> all;
    std::string path;
    collect(0, path, all);
    for (const auto& key : all) {
        const std::size_t d = distance(query, key, max_distance);
        if (d <= max_distance) scored.emplace_back(d, key);
    }
    std::sort(scored.begin(), scored.end(), [](const auto& x, const auto& y) {
        if (x.first != y.first) return x.first < y.first;
        return x.second < y.second;
    });
    std::vector<std::string> out;
    out.reserve(scored.size());
    for (const auto& s : scored) out.push_back(s.second);
    return out;
}

}  // namespace libcontainer

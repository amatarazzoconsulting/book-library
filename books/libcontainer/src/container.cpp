// MIT License - (c) Anthony Matarazzo 2026
#include "container.hpp"

#include <algorithm>

namespace libcontainer {

Container::Container(ContainerConfig config) : config_(config) {}

bool Container::insert(const std::string& key, std::string payload) {
    auto it = entries_.find(key);
    if (it != entries_.end()) return false;
    Entry e;
    e.key = key;
    e.payload = std::move(payload);
    e.dirty = config_.track_dirty;
    entries_.emplace(key, std::move(e));
    if (config_.enable_prefix_search) search_.insert(key);
    for (const auto& kv : index_extractors_) {
        for (const auto& value : kv.second(entries_.at(key))) {
            index_tables_[kv.first][value].push_back(key);
        }
    }
    return true;
}

bool Container::update(const std::string& key, std::string payload) {
    auto it = entries_.find(key);
    if (it == entries_.end()) return false;
    // Remove the old index entries before changing the value.
    for (const auto& kv : index_extractors_) {
        for (const auto& value : kv.second(it->second)) {
            auto& bucket = index_tables_[kv.first][value];
            bucket.erase(std::remove(bucket.begin(), bucket.end(), key), bucket.end());
        }
    }
    it->second.payload = std::move(payload);
    it->second.dirty = config_.track_dirty;
    for (const auto& kv : index_extractors_) {
        for (const auto& value : kv.second(it->second)) {
            index_tables_[kv.first][value].push_back(key);
        }
    }
    return true;
}

bool Container::erase(const std::string& key) {
    auto it = entries_.find(key);
    if (it == entries_.end()) return false;
    for (const auto& kv : index_extractors_) {
        for (const auto& value : kv.second(it->second)) {
            auto& bucket = index_tables_[kv.first][value];
            bucket.erase(std::remove(bucket.begin(), bucket.end(), key), bucket.end());
        }
    }
    search_.erase(key);
    entries_.erase(it);
    return true;
}

const Entry* Container::find(const std::string& key) const {
    auto it = entries_.find(key);
    return it == entries_.end() ? nullptr : &it->second;
}

bool Container::add_secondary_index(const std::string& name, Extractor extractor) {
    if (name.empty() || !extractor) return false;
    index_extractors_[name] = extractor;
    auto& table = index_tables_[name];
    table.clear();
    for (const auto& kv : entries_) {
        for (const auto& value : extractor(kv.second)) table[value].push_back(kv.first);
    }
    return true;
}

void Container::visit_all(const Visitor& visitor) const {
    for (const auto& kv : entries_) visitor(kv.first, kv.second);
}

void Container::visit_prefix(const std::string& prefix, const Visitor& visitor) const {
    for (const auto& key : search_.prefix(prefix)) {
        auto it = entries_.find(key);
        if (it != entries_.end()) visitor(it->first, it->second);
    }
}

void Container::visit_by_index(const std::string& index, const std::string& value,
                               const Visitor& visitor) const {
    auto idx = index_tables_.find(index);
    if (idx == index_tables_.end()) return;
    auto bucket = idx->second.find(value);
    if (bucket == idx->second.end()) return;
    for (const auto& key : bucket->second) {
        auto it = entries_.find(key);
        if (it != entries_.end()) visitor(it->first, it->second);
    }
}

void Container::visit_fuzzy(const std::string& query, std::size_t max_distance,
                            const Visitor& visitor) const {
    for (const auto& key : search_.fuzzy(query, max_distance)) {
        auto it = entries_.find(key);
        if (it != entries_.end()) visitor(it->first, it->second);
    }
}

std::size_t Container::dirty_count() const {
    std::size_t n = 0;
    for (const auto& kv : entries_) {
        if (kv.second.dirty) ++n;
    }
    return n;
}

void Container::sync() {
    for (auto& kv : entries_) kv.second.dirty = false;
}

}  // namespace libcontainer

// MIT License - (c) Anthony Matarazzo 2026
// The unified container: primary store, secondary indices and the visitor pattern.
#pragma once
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "search_index.hpp"

namespace libcontainer {

struct ContainerConfig {
    std::size_t window_size = 4096;        // memory window in bytes
    std::size_t max_memory_mb = 256;       // soft memory ceiling
    std::size_t compression_threshold = 4096;  // compress values at or above this size
    bool track_dirty = true;
    bool enable_prefix_search = true;
};

// A value stored in the container. Values are kept as bytes so the container can
// hold anything; typed access goes through the templated helpers in the demo.
struct Entry {
    std::string key;
    std::string payload;
    bool dirty = false;
};

class Container {
public:
    using Visitor = std::function<void(const std::string&, const Entry&)>;

    explicit Container(ContainerConfig config = {});

    bool insert(const std::string& key, std::string payload);
    bool update(const std::string& key, std::string payload);
    bool erase(const std::string& key);
    const Entry* find(const std::string& key) const;

    // Secondary index on an arbitrary extraction function.
    using Extractor = std::function<std::vector<std::string>(const Entry&)>;
    bool add_secondary_index(const std::string& name, Extractor extractor);

    // Visitor pattern entry points.
    void visit_all(const Visitor& visitor) const;
    void visit_prefix(const std::string& prefix, const Visitor& visitor) const;
    void visit_by_index(const std::string& index, const std::string& value,
                        const Visitor& visitor) const;
    void visit_fuzzy(const std::string& query, std::size_t max_distance,
                     const Visitor& visitor) const;

    std::size_t size() const { return entries_.size(); }
    std::size_t dirty_count() const;
    void sync();
    const ContainerConfig& config() const { return config_; }
    const SearchIndex& search() const { return search_; }

private:
    ContainerConfig config_;
    std::map<std::string, Entry> entries_;
    SearchIndex search_;
    std::map<std::string, Extractor> index_extractors_;
    std::map<std::string, std::map<std::string, std::vector<std::string>>> index_tables_;
};

}  // namespace libcontainer

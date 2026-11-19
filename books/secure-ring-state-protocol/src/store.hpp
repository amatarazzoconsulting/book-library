// MIT License - (c) Anthony Matarazzo 2026
// Versioned key-value store with prefix scan and watch callbacks, matching the
// database_t surface the book describes: put, get, remove, exists, batch, scan,
// watch and unwatch. Writes are committed atomically per batch.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace srsp {
namespace store {

struct entry {
    std::string value;
    std::uint64_t version = 0;
    std::uint64_t updated_at = 0;
};

enum class op_kind { put, remove };
const char* to_string(op_kind k);

struct op {
    op_kind kind = op_kind::put;
    std::string key;
    std::string value;
};

// Called with (key, value, deleted) after a commit.
using watch_fn = std::function<void(const std::string&, const std::string&, bool)>;

struct stats {
    std::size_t puts = 0;
    std::size_t removes = 0;
    std::size_t batches = 0;
    std::size_t notifications = 0;
    std::size_t rejected_batches = 0;
};

class kv_store {
public:
    kv_store() = default;

    bool put(const std::string& key, const std::string& value, std::uint64_t now);
    bool get(const std::string& key, std::string* value, std::uint64_t* version = nullptr) const;
    bool remove(const std::string& key, std::uint64_t now);
    bool exists(const std::string& key) const;

    // All operations apply or none do: an empty key or a remove of a missing key
    // rejects the whole batch and leaves the store untouched.
    bool batch(const std::vector<op>& ops, std::uint64_t now, std::string* error);

    // Lexicographic prefix scan, capped at limit entries.
    std::vector<std::pair<std::string, entry>> scan(const std::string& prefix,
                                                    std::size_t limit) const;

    std::uint64_t watch(const std::string& key, watch_fn fn);
    bool unwatch(std::uint64_t token);
    std::size_t watchers(const std::string& key) const;

    std::size_t size() const { return data_.size(); }
    std::uint64_t version_of(const std::string& key) const;
    const stats& counters() const { return stats_; }

private:
    void notify(const std::string& key, const std::string& value, bool deleted);

    std::map<std::string, entry> data_;
    std::uint64_t version_ = 0;
    std::uint64_t next_token_ = 1;
    std::map<std::string, std::map<std::uint64_t, watch_fn>> watches_;
    stats stats_;
};

}  // namespace store
}  // namespace srsp

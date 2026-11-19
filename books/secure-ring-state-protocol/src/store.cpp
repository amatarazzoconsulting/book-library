// MIT License - (c) Anthony Matarazzo 2026
#include "store.hpp"

namespace srsp {
namespace store {

const char* to_string(op_kind k) {
    return k == op_kind::put ? "put" : "remove";
}

bool kv_store::put(const std::string& key, const std::string& value, std::uint64_t now) {
    if (key.empty()) return false;
    entry e;
    e.value = value;
    e.version = ++version_;
    e.updated_at = now;
    data_[key] = e;
    ++stats_.puts;
    notify(key, value, false);
    return true;
}

bool kv_store::get(const std::string& key, std::string* value, std::uint64_t* version) const {
    auto it = data_.find(key);
    if (it == data_.end()) return false;
    if (value) *value = it->second.value;
    if (version) *version = it->second.version;
    return true;
}

bool kv_store::remove(const std::string& key, std::uint64_t now) {
    auto it = data_.find(key);
    if (it == data_.end()) return false;
    data_.erase(it);
    ++version_;
    ++stats_.removes;
    notify(key, "", true);
    (void)now;
    return true;
}

bool kv_store::exists(const std::string& key) const { return data_.count(key) != 0; }

bool kv_store::batch(const std::vector<op>& ops, std::uint64_t now, std::string* error) {
    if (ops.empty()) {
        if (error) *error = "empty batch";
        ++stats_.rejected_batches;
        return false;
    }
    // Validate the batch against the keys it touches, not against the whole store:
    // copying the key set would make every batch O(n) in the store size.
    std::map<std::string, bool> touched;
    for (const auto& o : ops) {
        if (o.key.empty()) {
            if (error) *error = "batch contains an empty key";
            ++stats_.rejected_batches;
            return false;
        }
        bool present;
        auto seen = touched.find(o.key);
        if (seen != touched.end()) {
            present = seen->second;
        } else {
            present = data_.count(o.key) != 0;
        }
        if (o.kind == op_kind::put) {
            touched[o.key] = true;
        } else {
            if (!present) {
                if (error) *error = "remove of missing key: " + o.key;
                ++stats_.rejected_batches;
                return false;
            }
            touched[o.key] = false;
        }
    }
    for (const auto& o : ops) {
        if (o.kind == op_kind::put) {
            entry e;
            e.value = o.value;
            e.version = ++version_;
            e.updated_at = now;
            data_[o.key] = e;
            ++stats_.puts;
        } else {
            data_.erase(o.key);
            ++version_;
            ++stats_.removes;
        }
    }
    ++stats_.batches;
    for (const auto& o : ops) notify(o.key, o.value, o.kind == op_kind::remove);
    return true;
}

std::vector<std::pair<std::string, entry>> kv_store::scan(const std::string& prefix,
                                                          std::size_t limit) const {
    std::vector<std::pair<std::string, entry>> out;
    auto it = prefix.empty() ? data_.begin() : data_.lower_bound(prefix);
    for (; it != data_.end(); ++it) {
        if (it->first.compare(0, prefix.size(), prefix) != 0) break;
        out.emplace_back(it->first, it->second);
        if (out.size() >= limit) break;
    }
    return out;
}

std::uint64_t kv_store::watch(const std::string& key, watch_fn fn) {
    const std::uint64_t token = next_token_++;
    watches_[key][token] = std::move(fn);
    return token;
}

bool kv_store::unwatch(std::uint64_t token) {
    for (auto& kv : watches_) {
        auto it = kv.second.find(token);
        if (it != kv.second.end()) {
            kv.second.erase(it);
            if (kv.second.empty()) watches_.erase(kv.first);
            return true;
        }
    }
    return false;
}

std::size_t kv_store::watchers(const std::string& key) const {
    auto it = watches_.find(key);
    return it == watches_.end() ? 0 : it->second.size();
}

std::uint64_t kv_store::version_of(const std::string& key) const {
    auto it = data_.find(key);
    return it == data_.end() ? 0 : it->second.version;
}

void kv_store::notify(const std::string& key, const std::string& value, bool deleted) {
    auto it = watches_.find(key);
    if (it == watches_.end()) return;
    // Copy the callbacks first: a callback may register or drop a watch.
    std::vector<watch_fn> callbacks;
    callbacks.reserve(it->second.size());
    for (const auto& kv : it->second) callbacks.push_back(kv.second);
    for (const auto& fn : callbacks) {
        fn(key, value, deleted);
        ++stats_.notifications;
    }
}

}  // namespace store
}  // namespace srsp

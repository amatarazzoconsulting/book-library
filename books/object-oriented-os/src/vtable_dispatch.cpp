// MIT License - (c) Anthony Matarazzo 2026
#include "vtable_dispatch.hpp"

#include <algorithm>
#include <stdexcept>

namespace nebula {

VClass::VClass(std::string name, const VClass* base)
    : name_(std::move(name)), base_(base) {
    if (base_ != nullptr) {
        slots_ = base_->slots_;
        by_name_ = base_->by_name_;
        // Base slots keep their index; owner is re-stamped on override only.
    }
}

int VClass::addMethod(const MethodDef& def) {
    auto it = by_name_.find(def.name);
    if (it != by_name_.end()) {
        const int idx = it->second;
        if (slots_[idx].arity != def.arity && slots_[idx].arity >= 0 && def.arity >= 0) {
            return -1;  // incompatible override signature
        }
        slots_[idx].fn = def.fn;
        slots_[idx].arity = def.arity;
        slots_[idx].owner = this;
        return idx;
    }
    const int idx = static_cast<int>(slots_.size());
    slots_.push_back(VSlot{def.name, def.fn, def.arity, this});
    by_name_.emplace(def.name, idx);
    return idx;
}

int VClass::findSlot(const std::string& method) const {
    auto it = by_name_.find(method);
    return it == by_name_.end() ? -1 : it->second;
}

const VSlot* VClass::slot(int index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= slots_.size()) return nullptr;
    return &slots_[static_cast<std::size_t>(index)];
}

bool VClass::isA(const VClass* other) const {
    for (const VClass* c = this; c != nullptr; c = c->base_)
        if (c == other) return true;
    return false;
}

std::vector<std::string> VClass::methodNames() const {
    std::vector<std::string> out;
    out.reserve(slots_.size());
    for (const VSlot& s : slots_) out.push_back(s.name);
    return out;
}

Dispatcher::Dispatcher(DispatchOptions opt) : opt_(opt) {
    if (opt_.inline_cache) {
        std::size_t n = 1;
        while (n < opt_.cache_entries) n <<= 1;
        cache_.assign(n, CacheEntry{});
    }
}

std::uint64_t Dispatcher::hashName(const std::string& s) {
    std::uint64_t h = 1469598103934665603ull;  // FNV-1a
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ull;
    }
    return h;
}

std::size_t Dispatcher::cacheIndex(const VClass* cls, std::uint64_t h) const {
    const std::uint64_t mix =
        h ^ (static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(cls)) >> 4);
    return static_cast<std::size_t>(mix) & (cache_.size() - 1);
}

const VSlot* Dispatcher::resolveUncached(const VClass* cls, const std::string& method) {
    if (cls == nullptr) return nullptr;
    int idx = -1;
    if (opt_.count_stats) ++stats_.slot_scans;
    idx = cls->findSlot(method);
    if (idx < 0 && opt_.allow_base_fallback) {
        for (const VClass* c = cls->base(); c != nullptr; c = c->base()) {
            if (opt_.count_stats) ++stats_.slot_scans;
            idx = c->findSlot(method);
            if (idx >= 0) break;
        }
    }
    return idx < 0 ? nullptr : cls->slot(idx);
}

const VSlot* Dispatcher::resolve(const VClass* cls, const std::string& method) {
    if (!opt_.inline_cache) {
        const VSlot* s = resolveUncached(cls, method);
        if (opt_.count_stats && s == nullptr) ++stats_.misses;
        return s;
    }
    const std::uint64_t h = hashName(method);
    const std::size_t i = cacheIndex(cls, h);
    CacheEntry& e = cache_[i];
    if (e.cls == cls && e.name_hash == h) {
        if (opt_.count_stats) ++stats_.cache_hits;
        if (e.slot == nullptr && opt_.count_stats) ++stats_.misses;
        return e.slot;
    }
    if (opt_.count_stats) ++stats_.cache_misses;
    const VSlot* s = resolveUncached(cls, method);
    e.cls = cls;
    e.name_hash = h;
    e.slot = s;
    if (opt_.count_stats && s == nullptr) ++stats_.misses;
    return s;
}

Box Dispatcher::invoke(void* self, const VClass* cls, const std::string& method,
                       std::vector<Box> args) {
    if (opt_.count_stats) ++stats_.calls;
    const VSlot* s = resolve(cls, method);
    if (s == nullptr || s->fn == nullptr) {
        if (opt_.count_stats) ++stats_.misses;
        throw std::runtime_error("Dispatcher: no method '" + method + "' on " +
                                 (cls ? cls->name() : std::string("<null>")));
    }
    if (opt_.check_arity && s->arity >= 0 &&
        static_cast<int>(args.size()) != s->arity) {
        if (opt_.count_stats) ++stats_.misses;
        throw std::runtime_error("Dispatcher: arity mismatch on '" + method + "'");
    }
    return s->fn(self, args);
}

void Dispatcher::clearCache() {
    for (CacheEntry& e : cache_) e = CacheEntry{};
}

}  // namespace nebula

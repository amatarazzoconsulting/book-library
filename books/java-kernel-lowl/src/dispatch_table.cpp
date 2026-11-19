// MIT License - (c) Anthony Matarazzo 2026
//
// dispatch_table.cpp - vtable construction and cached invocation.

#include "dispatch_table.hpp"

#include <algorithm>
#include <cstdio>
#include <stdexcept>

namespace lowl {

std::int64_t Value::as_int() const {
    if (kind_ == Kind::INT) return i_;
    if (kind_ == Kind::FLOAT) return static_cast<std::int64_t>(d_);
    throw std::runtime_error("Value::as_int on " + to_string());
}

double Value::as_float() const {
    if (kind_ == Kind::FLOAT) return d_;
    if (kind_ == Kind::INT) return static_cast<double>(i_);
    throw std::runtime_error("Value::as_float on " + to_string());
}

const std::string& Value::as_text() const {
    if (kind_ != Kind::STRING) throw std::runtime_error("Value::as_text on " + to_string());
    return s_;
}

std::string Value::to_string() const {
    switch (kind_) {
        case Kind::VOID: return "void";
        case Kind::INT: return std::to_string(i_);
        case Kind::FLOAT: {
            char buffer[32];
            std::snprintf(buffer, sizeof buffer, "%g", d_);
            return std::string(buffer);
        }
        case Kind::STRING: return s_;
    }
    return "void";
}

DispatchTable::DispatchTable() { cache_.resize(64); }

std::int32_t DispatchTable::find_class(const std::string& name) const {
    const auto it = class_index_.find(name);
    if (it == class_index_.end()) return -1;
    return static_cast<std::int32_t>(it->second);
}

ClassInfo& DispatchTable::mutable_class(std::uint32_t id) {
    if (id >= classes_.size()) throw std::out_of_range("bad class id " + std::to_string(id));
    return classes_[id];
}

std::uint32_t DispatchTable::define_class(const std::string& name, const std::string& super) {
    if (name.empty()) throw std::invalid_argument("define_class: empty name");
    if (find_class(name) >= 0) throw std::invalid_argument("define_class: duplicate class " + name);

    std::int32_t super_id = -1;
    if (!super.empty()) {
        super_id = find_class(super);
        if (super_id < 0) throw std::invalid_argument("define_class: unknown superclass " + super);
    }

    ClassInfo info;
    info.name = name;
    info.id = static_cast<std::uint32_t>(classes_.size());
    info.super = super_id;
    if (super_id >= 0) {
        // Inherit the superclass vtable wholesale; overrides replace in place.
        const ClassInfo& parent = classes_[static_cast<std::size_t>(super_id)];
        info.vtable = parent.vtable;
        info.slot_of = parent.slot_of;
    }
    classes_.push_back(std::move(info));
    class_index_[name] = classes_.back().id;
    return classes_.back().id;
}

void DispatchTable::add_interface(std::uint32_t class_id_value, std::uint32_t iface_id) {
    ClassInfo& info = mutable_class(class_id_value);
    class_info(iface_id);  // range check
    const auto id = static_cast<std::int32_t>(iface_id);
    if (std::find(info.interfaces.begin(), info.interfaces.end(), id) != info.interfaces.end()) return;
    info.interfaces.push_back(id);
}

void DispatchTable::inherit_defaults(std::uint32_t class_id_value, std::uint32_t iface_id) {
    ClassInfo& info = mutable_class(class_id_value);
    const ClassInfo iface = class_info(iface_id);  // copy: may alias info
    for (const auto& entry : iface.slot_of) {
        if (info.slot_of.count(entry.first) != 0) continue;  // class already overrides
        const std::uint32_t method_index = iface.vtable[entry.second];
        const std::uint32_t slot = intern_name(entry.first);
        if (info.vtable.size() <= slot) info.vtable.resize(slot + 1, 0xffffffffu);
        info.vtable[slot] = method_index;
        info.slot_of[entry.first] = slot;
    }
}

std::uint32_t DispatchTable::intern_name(const std::string& name) const {
    const auto it = name_ids_.find(name);
    if (it != name_ids_.end()) return it->second;
    const std::uint32_t id = static_cast<std::uint32_t>(name_ids_.size());
    name_ids_[name] = id;
    return id;
}

std::uint32_t DispatchTable::define_method(std::uint32_t class_id_value, const std::string& name, NativeFn fn) {
    if (!fn) throw std::invalid_argument("define_method: empty function");
    if (name.empty()) throw std::invalid_argument("define_method: empty name");
    ClassInfo& info = mutable_class(class_id_value);

    std::uint32_t slot = 0;
    const auto existing = info.slot_of.find(name);
    if (existing != info.slot_of.end()) {
        slot = existing->second;
    } else {
        slot = intern_name(name);
        info.slot_of[name] = slot;
        if (info.vtable.size() <= slot) info.vtable.resize(slot + 1, 0xffffffffu);
    }

    MethodSlot method;
    method.name = name;
    method.fn = std::move(fn);
    methods_.push_back(std::move(method));
    info.vtable[slot] = static_cast<std::uint32_t>(methods_.size() - 1);
    return slot;
}

const ClassInfo& DispatchTable::class_info(std::uint32_t id) const {
    if (id >= classes_.size()) throw std::out_of_range("class_info: bad class id " + std::to_string(id));
    return classes_[id];
}

std::uint32_t DispatchTable::class_id(const std::string& name) const {
    const std::int32_t id = find_class(name);
    if (id < 0) throw std::invalid_argument("class_id: unknown class " + name);
    return static_cast<std::uint32_t>(id);
}

std::uint32_t DispatchTable::resolve_slot(std::uint32_t class_id_value, const std::string& name) const {
    const ClassInfo& info = class_info(class_id_value);
    const auto it = info.slot_of.find(name);
    if (it != info.slot_of.end()) return it->second;

    for (const std::int32_t iface_id : info.interfaces) {
        const ClassInfo& iface = class_info(static_cast<std::uint32_t>(iface_id));
        const auto iface_it = iface.slot_of.find(name);
        if (iface_it != iface.slot_of.end()) return iface_it->second;
    }
    throw std::invalid_argument("resolve_slot: no method '" + name + "' on class " + info.name);
}

bool DispatchTable::has_method(std::uint32_t class_id_value, const std::string& name) const {
    try {
        (void)resolve_slot(class_id_value, name);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

const MethodSlot* DispatchTable::slot_at(std::uint32_t class_id_value, std::uint32_t slot) const {
    const ClassInfo& info = class_info(class_id_value);
    if (slot >= info.vtable.size()) return nullptr;
    const std::uint32_t method_index = info.vtable[slot];
    if (method_index == 0xffffffffu || method_index >= methods_.size()) return nullptr;
    return &methods_[method_index];
}

Value DispatchTable::invoke(std::uint32_t class_id_value, std::uint32_t slot, const std::vector<Value>& args) {
    const ClassInfo& info = class_info(class_id_value);
    if (slot >= info.vtable.size()) {
        throw std::invalid_argument("invoke: slot " + std::to_string(slot) + " out of range on class " + info.name);
    }
    const std::uint32_t method_index = info.vtable[slot];
    if (method_index == 0xffffffffu || method_index >= methods_.size()) {
        throw std::invalid_argument("invoke: unbound slot " + std::to_string(slot) + " on class " + info.name);
    }
    MethodSlot& method = methods_[method_index];
    ++method.calls;
    return method.fn(args);
}

std::size_t DispatchTable::cache_index(std::uint32_t class_id_value, std::uint32_t name_id) const {
    // Two independent multipliers keep (class, name) pairs from piling up.
    std::uint64_t mixed = static_cast<std::uint64_t>(class_id_value) * 0x9e3779b97f4a7c15ull;
    mixed ^= static_cast<std::uint64_t>(name_id) * 0xc2b2ae3d27d4eb4full;
    mixed ^= mixed >> 29;
    return static_cast<std::size_t>(mixed % cache_.size());
}

Value DispatchTable::invoke_by_name(std::uint32_t class_id_value, const std::string& name,
                                    const std::vector<Value>& args) {
    const std::uint32_t name_id = intern_name(name);
    const std::size_t index = cache_index(class_id_value, name_id);
    CacheEntry& entry = cache_[index];

    if (entry.valid && entry.class_id == class_id_value && entry.name_id == name_id) {
        ++cache_stats_.hits;
        return invoke(class_id_value, entry.slot, args);
    }

    if (entry.valid) ++cache_stats_.collisions;
    ++cache_stats_.misses;
    const std::uint32_t slot = resolve_slot(class_id_value, name);
    entry.valid = true;
    entry.class_id = class_id_value;
    entry.name_id = name_id;
    entry.slot = slot;
    return invoke(class_id_value, slot, args);
}

Value DispatchTable::invoke_at(CallSite& site, std::uint32_t class_id_value, const std::string& name,
                               const std::vector<Value>& args) {
    if (site.bound && site.class_id == class_id_value && site.name == name) {
        ++site.hits;
        return invoke(class_id_value, site.slot, args);
    }
    if (site.bound) ++site.rebinds;  // polymorphic call site: re-resolve
    site.name = name;
    site.class_id = class_id_value;
    site.slot = resolve_slot(class_id_value, name);
    site.bound = true;
    return invoke(class_id_value, site.slot, args);
}

std::vector<std::string> DispatchTable::method_names(std::uint32_t class_id_value) const {
    const ClassInfo& info = class_info(class_id_value);
    std::vector<std::string> names;
    names.reserve(info.slot_of.size());
    for (const auto& entry : info.slot_of) names.push_back(entry.first);
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> DispatchTable::vtable_dump(std::uint32_t class_id_value) const {
    const ClassInfo& info = class_info(class_id_value);
    std::vector<std::string> rows;
    rows.reserve(info.vtable.size());
    for (std::size_t slot = 0; slot < info.vtable.size(); ++slot) {
        const std::uint32_t method_index = info.vtable[slot];
        if (method_index == 0xffffffffu || method_index >= methods_.size()) {
            rows.push_back("slot " + std::to_string(slot) + " = <unbound>");
        } else {
            rows.push_back("slot " + std::to_string(slot) + " = " + methods_[method_index].name);
        }
    }
    return rows;
}

std::size_t DispatchTable::vtable_bytes() const {
    std::size_t total = 0;
    for (const ClassInfo& info : classes_) total += info.vtable.size() * sizeof(std::uint32_t);
    return total;
}

std::uint64_t DispatchTable::total_calls() const {
    std::uint64_t total = 0;
    for (const MethodSlot& method : methods_) total += method.calls;
    return total;
}

DispatchTable::CacheStats DispatchTable::cache_stats() const {
    CacheStats snapshot = cache_stats_;
    snapshot.entries = 0;
    for (const CacheEntry& entry : cache_) {
        if (entry.valid) ++snapshot.entries;
    }
    return snapshot;
}

void DispatchTable::clear_cache() {
    for (CacheEntry& entry : cache_) entry = CacheEntry{};
    cache_stats_ = CacheStats{};
}

}  // namespace lowl

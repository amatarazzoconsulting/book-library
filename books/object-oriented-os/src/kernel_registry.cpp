// MIT License - (c) Anthony Matarazzo 2026
#include "kernel_registry.hpp"

#include <algorithm>

namespace nebula {

KObject::~KObject() = default;

std::string KObject::toString() const {
    return typeName() + "#" + std::to_string(id_) + "(" + name_ + ")";
}

bool KObject::equals(const KObject* other) const {
    return other != nullptr && other->id_ == id_ && other->typeName() == typeName();
}

ObjectRegistry::ObjectRegistry(RegistryOptions opt) : opt_(opt) {
    if (opt_.capacity_hint > 0) {
        slots_.reserve(opt_.capacity_hint);
        free_.reserve(opt_.capacity_hint);
    }
    if (opt_.name_index) names_.reserve(opt_.capacity_hint);
}

ObjectRegistry::~ObjectRegistry() {
    for (Slot& s : slots_) {
        delete s.obj;
        s.obj = nullptr;
    }
}

bool ObjectRegistry::nameTaken(const std::string& name) const {
    if (!opt_.name_index || opt_.allow_duplicate_names) return false;
    return names_.find(name) != names_.end();
}

ObjectID ObjectRegistry::adopt(KObject* obj, const std::string& name) {
    std::uint32_t idx;
    if (!free_.empty()) {
        idx = free_.back();
        free_.pop_back();
    } else {
        idx = static_cast<std::uint32_t>(slots_.size());
        slots_.push_back(Slot{nullptr, opt_.first_generation});
    }

    const std::uint64_t handle =
        (static_cast<std::uint64_t>(slots_[idx].gen) << 32) |
        static_cast<std::uint64_t>(idx + 1u);

    obj->id_ = handle;
    obj->name_ = name;
    obj->refs_ = 1;
    slots_[idx].obj = obj;
    if (opt_.name_index) names_[name] = handle;

    ++stats_.live;
    ++stats_.created;
    stats_.slots = slots_.size();
    stats_.free_slots = free_.size();
    stats_.peak_live = std::max(stats_.peak_live, stats_.live);
    return handle;
}

KObject* ObjectRegistry::resolve(ObjectID id) const {
    ++stats_.resolves;
    if (id == kInvalidID || (id & 0xffffffffu) == 0) {
        ++stats_.stale_resolves;
        return nullptr;
    }
    const std::uint32_t idx = indexOf(id);
    if (idx >= slots_.size() || slots_[idx].gen != generationOf(id) ||
        slots_[idx].obj == nullptr) {
        ++stats_.stale_resolves;
        return nullptr;
    }
    return slots_[idx].obj;
}

bool ObjectRegistry::retain(ObjectID id) {
    KObject* o = resolve(id);
    if (o == nullptr) return false;
    o->ref();
    return true;
}

bool ObjectRegistry::release(ObjectID id) {
    KObject* o = resolve(id);
    if (o == nullptr) return false;
    if (!o->unref()) return false;

    const std::uint32_t idx = indexOf(id);
    if (opt_.name_index) {
        auto it = names_.find(o->name_);
        if (it != names_.end() && it->second == id) names_.erase(it);
    }
    delete o;
    slots_[idx].obj = nullptr;
    ++slots_[idx].gen;  // invalidates every outstanding copy of the handle
    free_.push_back(idx);

    --stats_.live;
    ++stats_.destroyed;
    stats_.free_slots = free_.size();
    return true;
}

bool ObjectRegistry::rename(ObjectID id, const std::string& newName) {
    KObject* o = resolve(id);
    if (o == nullptr) return false;
    if (newName == o->name_) return true;
    if (nameTaken(newName)) return false;
    if (opt_.name_index) {
        auto it = names_.find(o->name_);
        if (it != names_.end() && it->second == id) names_.erase(it);
        names_[newName] = id;
    }
    o->setName(newName);
    return true;
}

KObject* ObjectRegistry::findByName(const std::string& name) const {
    if (!opt_.name_index) return nullptr;
    auto it = names_.find(name);
    if (it == names_.end()) return nullptr;
    return resolve(it->second);
}

std::vector<ObjectID> ObjectRegistry::list() const {
    std::vector<ObjectID> out;
    out.reserve(stats_.live);
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        if (slots_[i].obj == nullptr) continue;
        out.push_back((static_cast<std::uint64_t>(slots_[i].gen) << 32) |
                      static_cast<std::uint64_t>(i + 1u));
    }
    return out;
}

const RegistryStats& ObjectRegistry::stats() const {
    stats_.slots = slots_.size();
    stats_.free_slots = free_.size();
    return stats_;
}

}  // namespace nebula

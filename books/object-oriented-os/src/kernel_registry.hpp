// MIT License - (c) Anthony Matarazzo 2026
//
// NEBULA kernel object registry (book section 3.1 / 3.2).
// Generational handles, a name index, reference counting and typed resolve.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nebula {

// Handle layout: high 32 bits = generation, low 32 bits = slot index + 1.
// 0 is the invalid handle, so a default constructed id never resolves.
using ObjectID = std::uint64_t;
constexpr ObjectID kInvalidID = 0;

struct RegistryOptions {
    bool name_index = true;              // maintain name -> id lookup
    bool allow_duplicate_names = false;  // permit two live objects with one name
    std::size_t capacity_hint = 16;      // slots reserved up front
    std::uint32_t first_generation = 1;  // generation stamped on a fresh slot
};

// Root of the kernel object tree. Objects are owned by the registry, which
// holds one reference for the handle it hands out.
class KObject {
public:
    virtual ~KObject();

    ObjectID id() const { return id_; }
    const std::string& name() const { return name_; }
    void setName(const std::string& n) { name_ = n; }
    std::uint32_t refCount() const { return refs_; }

    void ref() { ++refs_; }
    bool unref() { return --refs_ == 0; }

    virtual std::string typeName() const { return "KObject"; }
    virtual std::string toString() const;
    virtual bool equals(const KObject* other) const;

private:
    friend class ObjectRegistry;
    ObjectID id_ = kInvalidID;
    std::string name_;
    std::uint32_t refs_ = 1;
};

struct RegistryStats {
    std::size_t live = 0;
    std::size_t peak_live = 0;
    std::size_t created = 0;
    std::size_t destroyed = 0;
    std::size_t slots = 0;
    std::size_t free_slots = 0;
    mutable std::uint64_t resolves = 0;
    mutable std::uint64_t stale_resolves = 0;
};

class ObjectRegistry {
public:
    explicit ObjectRegistry(RegistryOptions opt = RegistryOptions());
    ~ObjectRegistry();
    ObjectRegistry(const ObjectRegistry&) = delete;
    ObjectRegistry& operator=(const ObjectRegistry&) = delete;

    // Allocate a T, stamp it with a fresh handle and return that handle.
    // Returns kInvalidID when the name is taken and duplicates are refused.
    template <typename T, typename... Args>
    ObjectID create(const std::string& name, Args&&... args) {
        static_assert(std::is_base_of<KObject, T>::value,
                      "T must derive from nebula::KObject");
        if (nameTaken(name)) return kInvalidID;
        return adopt(new T(std::forward<Args>(args)...), name);
    }

    bool retain(ObjectID id);   // ++refs, false if the handle is stale
    bool release(ObjectID id);  // --refs, destroys the object at zero
    bool rename(ObjectID id, const std::string& newName);

    KObject* resolve(ObjectID id) const;
    template <typename T>
    T* resolveAs(ObjectID id) const {
        return dynamic_cast<T*>(resolve(id));
    }

    KObject* findByName(const std::string& name) const;
    std::vector<ObjectID> list() const;  // ascending slot order, deterministic
    bool valid(ObjectID id) const { return resolve(id) != nullptr; }

    std::size_t liveCount() const { return stats_.live; }
    std::size_t slotCount() const { return slots_.size(); }
    const RegistryOptions& options() const { return opt_; }
    const RegistryStats& stats() const;

private:
    struct Slot {
        KObject* obj = nullptr;
        std::uint32_t gen = 1;
    };

    ObjectID adopt(KObject* obj, const std::string& name);
    bool nameTaken(const std::string& name) const;
    static std::uint32_t indexOf(ObjectID id) {
        return static_cast<std::uint32_t>(id & 0xffffffffu) - 1u;
    }
    static std::uint32_t generationOf(ObjectID id) {
        return static_cast<std::uint32_t>(id >> 32);
    }

    RegistryOptions opt_;
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_;
    std::unordered_map<std::string, ObjectID> names_;
    mutable RegistryStats stats_;
};

}  // namespace nebula

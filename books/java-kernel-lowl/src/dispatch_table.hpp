// MIT License - (c) Anthony Matarazzo 2026
//
// dispatch_table.hpp - virtual method dispatch across the JNI boundary.
//
// The Java side calls a native method by name.  Hashing that name on every
// call is the slow path.  This file models the fast path used by the JNI
// bridge: each class owns a dense vtable, overrides replace slots in place,
// and a call site caches (class id -> slot) so a call becomes one bounds check
// plus one indirect call.

#ifndef LOWL_DISPATCH_TABLE_HPP
#define LOWL_DISPATCH_TABLE_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace lowl {

class Value;
using NativeFn = std::function<Value(const std::vector<Value>&)>;

// A tiny dynamic value so demos and benchmarks have something to pass.
class Value {
public:
    enum class Kind { VOID, INT, FLOAT, STRING };

    Value() = default;
    explicit Value(std::int64_t v) : kind_(Kind::INT), i_(v) {}
    explicit Value(double v) : kind_(Kind::FLOAT), d_(v) {}
    explicit Value(std::string v) : kind_(Kind::STRING), s_(std::move(v)) {}

    Kind kind() const { return kind_; }
    bool is_void() const { return kind_ == Kind::VOID; }
    std::int64_t as_int() const;
    double as_float() const;
    const std::string& as_text() const;
    std::string to_string() const;

private:
    Kind kind_ = Kind::VOID;
    std::int64_t i_ = 0;
    double d_ = 0.0;
    std::string s_;
};

// One concrete method implementation.
struct MethodSlot {
    std::string name;
    NativeFn fn;
    std::uint32_t calls = 0;
};

// A registered class.  `id` is a small dense integer used as the cache key.
struct ClassInfo {
    std::string name;
    std::uint32_t id = 0;
    std::int32_t super = -1;                    // -1 means no superclass
    std::vector<std::int32_t> interfaces;       // declared interface ids
    std::unordered_map<std::string, std::uint32_t> slot_of;  // name -> vtable slot
    std::vector<std::uint32_t> vtable;          // slot -> method index
};

// A monomorphic call site.  After the first call it holds the resolved slot,
// so repeated calls from the same location do no string work at all.
struct CallSite {
    std::string name;
    std::uint32_t class_id = 0xffffffffu;
    std::uint32_t slot = 0xffffffffu;
    bool bound = false;
    std::uint64_t hits = 0;
    std::uint64_t rebinds = 0;
};

class DispatchTable {
public:
    DispatchTable();

    // Registers a class and returns its id.  Superclass must already exist.
    std::uint32_t define_class(const std::string& name, const std::string& super = "");
    // Declares that `class_id` implements interface `iface_id`.
    void add_interface(std::uint32_t class_id, std::uint32_t iface_id);
    // Copies interface defaults into slots the class has not overridden.
    void inherit_defaults(std::uint32_t class_id, std::uint32_t iface_id);
    // Adds or overrides a method.  Returns the vtable slot.
    std::uint32_t define_method(std::uint32_t class_id, const std::string& name, NativeFn fn);

    // Slow path: name lookup through the class chain, then its interfaces.
    std::uint32_t resolve_slot(std::uint32_t class_id, const std::string& name) const;
    // Fast path: direct vtable index, no string work.
    const MethodSlot* slot_at(std::uint32_t class_id, std::uint32_t slot) const;
    bool has_method(std::uint32_t class_id, const std::string& name) const;

    // Calls through the vtable.  Throws if the slot is unbound.
    Value invoke(std::uint32_t class_id, std::uint32_t slot, const std::vector<Value>& args);
    // Name-based invocation through the shared inline cache.
    Value invoke_by_name(std::uint32_t class_id, const std::string& name, const std::vector<Value>& args);
    // Call-site invocation: bind once, then a bounds check and an indirect call.
    Value invoke_at(CallSite& site, std::uint32_t class_id, const std::string& name,
                    const std::vector<Value>& args);

    std::uint32_t class_id(const std::string& name) const;
    const ClassInfo& class_info(std::uint32_t id) const;
    std::vector<std::string> method_names(std::uint32_t class_id) const;
    std::vector<std::string> vtable_dump(std::uint32_t class_id) const;
    std::size_t class_count() const { return classes_.size(); }
    std::size_t method_count() const { return methods_.size(); }
    std::size_t vtable_bytes() const;
    std::uint64_t total_calls() const;

    struct CacheStats {
        std::uint64_t hits = 0;
        std::uint64_t misses = 0;
        std::uint64_t collisions = 0;
        std::size_t entries = 0;
    };
    CacheStats cache_stats() const;
    void clear_cache();

private:
    struct CacheEntry {
        std::uint32_t class_id = 0xffffffffu;
        std::uint32_t name_id = 0xffffffffu;
        std::uint32_t slot = 0xffffffffu;
        bool valid = false;
    };

    ClassInfo& mutable_class(std::uint32_t id);
    std::int32_t find_class(const std::string& name) const;
    std::uint32_t intern_name(const std::string& name) const;
    std::size_t cache_index(std::uint32_t class_id, std::uint32_t name_id) const;

    std::vector<ClassInfo> classes_;
    std::unordered_map<std::string, std::uint32_t> class_index_;
    std::vector<MethodSlot> methods_;
    mutable std::unordered_map<std::string, std::uint32_t> name_ids_;
    mutable std::vector<CacheEntry> cache_;
    mutable CacheStats cache_stats_;
};

}  // namespace lowl

#endif  // LOWL_DISPATCH_TABLE_HPP

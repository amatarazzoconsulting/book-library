// MIT License - (c) Anthony Matarazzo 2026
//
// NEBULA generated vtable dispatch (book section 3, virtual calls).
// Class descriptions are built at run time; every class owns a slot table laid
// out base-first, overrides reuse the base slot, new methods append. A
// dispatcher resolves names to slots and can cache the result inline.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "box_value.hpp"

namespace nebula {

using MethodFn = Box (*)(void* self, const std::vector<Box>& args);

struct MethodDef {
    std::string name;
    MethodFn fn = nullptr;
    int arity = 0;  // required argument count, -1 means any
};

// One entry per vtable slot. Overrides keep the slot of the base method so a
// call site compiled against the base class still lands on the right body.
struct VSlot {
    std::string name;
    MethodFn fn = nullptr;
    int arity = 0;
    const class VClass* owner = nullptr;  // class that supplied the body
};

class VClass {
public:
    VClass(std::string name, const VClass* base = nullptr);

    const std::string& name() const { return name_; }
    const VClass* base() const { return base_; }

    // Returns the slot index, or -1 when the signature conflicts with the base.
    int addMethod(const MethodDef& def);
    int findSlot(const std::string& method) const;
    const VSlot* slot(int index) const;
    std::size_t slotCount() const { return slots_.size(); }

    bool isA(const VClass* other) const;  // this class or one of its bases
    std::vector<std::string> methodNames() const;

private:
    std::string name_;
    const VClass* base_;
    std::vector<VSlot> slots_;
    std::unordered_map<std::string, int> by_name_;
};

struct DispatchOptions {
    bool inline_cache = true;       // memoize class+method -> slot resolution
    std::size_t cache_entries = 64; // rounded up to a power of two
    bool check_arity = true;        // reject calls with the wrong argument count
    bool allow_base_fallback = true;// search base classes when a slot is missing
    bool count_stats = true;        // keep hit/miss counters
};

struct DispatchStats {
    std::uint64_t calls = 0;
    std::uint64_t cache_hits = 0;
    std::uint64_t cache_misses = 0;
    std::uint64_t misses = 0;        // method not found or arity rejected
    std::uint64_t slot_scans = 0;    // name comparisons during resolution
};

class Dispatcher {
public:
    explicit Dispatcher(DispatchOptions opt = DispatchOptions());

    // Invoke "method" on "self" using the layout of "cls".
    Box invoke(void* self, const VClass* cls, const std::string& method,
               std::vector<Box> args = {});

    // Resolve without calling. Returns nullptr when not found.
    const VSlot* resolve(const VClass* cls, const std::string& method);

    const DispatchOptions& options() const { return opt_; }
    const DispatchStats& stats() const { return stats_; }
    void resetStats() { stats_ = DispatchStats(); }
    void clearCache();

private:
    struct CacheEntry {
        const VClass* cls = nullptr;
        std::uint64_t name_hash = 0;
        const VSlot* slot = nullptr;
    };

    static std::uint64_t hashName(const std::string& s);
    std::size_t cacheIndex(const VClass* cls, std::uint64_t h) const;
    const VSlot* resolveUncached(const VClass* cls, const std::string& method);

    DispatchOptions opt_;
    DispatchStats stats_;
    std::vector<CacheEntry> cache_;
};

}  // namespace nebula

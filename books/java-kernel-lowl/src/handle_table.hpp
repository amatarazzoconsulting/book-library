// MIT License - (c) Anthony Matarazzo 2026
//
// handle_table.hpp - the native handle registry behind every lowl.java object.
//
// A Java object stores one `long nativeHandle`.  That value is not a raw
// pointer: it is (generation << 32) | slot.  Slots are recycled, and the
// generation counter makes a stale handle detectable instead of letting it
// alias a new object.  Reference counts keep the payload alive, and pinning
// prevents the slot from being recycled while native code holds a raw pointer.

#ifndef LOWL_HANDLE_TABLE_HPP
#define LOWL_HANDLE_TABLE_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace lowl {

using Handle = std::uint64_t;
using Generation = std::uint32_t;
using SlotIndex = std::uint32_t;

constexpr Handle kInvalidHandle = 0;

inline Handle make_handle(SlotIndex slot, Generation generation) {
    return (static_cast<Handle>(generation) << 32) | static_cast<Handle>(slot);
}
inline SlotIndex handle_slot(Handle handle) { return static_cast<SlotIndex>(handle & 0xffffffffu); }
inline Generation handle_generation(Handle handle) { return static_cast<Generation>(handle >> 32); }

// Result of a resolve: a borrowed pointer plus the generation it belongs to.
struct Resolved {
    void* pointer = nullptr;
    Generation generation = 0;
    bool ok() const { return pointer != nullptr; }
};

struct HandleStats {
    std::size_t live = 0;        // slots currently holding a payload
    std::size_t capacity = 0;    // slots ever allocated
    std::size_t pinned = 0;      // live slots that are pinned
    std::size_t free_slots = 0;  // slots on the free list
    std::size_t high_water = 0;  // peak number of live slots
    std::uint64_t total_created = 0;
    std::uint64_t total_destroyed = 0;
    std::uint64_t stale_rejections = 0;
};

class HandleTable {
public:
    using Deleter = void (*)(void*);

    explicit HandleTable(std::size_t reserve = 0);

    HandleTable(const HandleTable&) = delete;
    HandleTable& operator=(const HandleTable&) = delete;

    // Registers a payload.  Refcount starts at 1 and the slot starts unpinned.
    Handle insert(void* payload, Deleter deleter, std::string type_tag = "Object");
    // Registers a payload that is already pinned (used for singletons).
    Handle insert_pinned(void* payload, Deleter deleter, std::string type_tag = "Object");

    bool retain(Handle handle);   // refcount + 1
    bool release(Handle handle);  // refcount - 1, destroys at 0 unless pinned
    bool pin(Handle handle);
    bool unpin(Handle handle);

    // Borrowed pointer.  Does not change the refcount.
    Resolved resolve(Handle handle) const;
    // Same, but marks the slot pinned for the duration of the borrow.
    Resolved acquire(Handle handle);
    bool release_borrow(Handle handle);

    int refcount(Handle handle) const;
    bool is_pinned(Handle handle) const;
    bool is_valid(Handle handle) const;
    std::string type_of(Handle handle) const;
    std::vector<Handle> live_handles() const;
    HandleStats stats() const;

    // Destroys every unpinned payload and empties the table.
    void clear();

private:
    struct Slot {
        void* payload = nullptr;
        Deleter deleter = nullptr;
        std::string type_tag;
        std::int32_t refcount = 0;
        std::uint32_t pins = 0;
        Generation generation = 1;
        bool live = false;
    };

    Slot* find(Handle handle);
    const Slot* find(Handle handle) const;
    void destroy_slot(Slot& slot);

    std::vector<Slot> slots_;
    std::vector<SlotIndex> free_list_;
    HandleStats stats_;
};

}  // namespace lowl

#endif  // LOWL_HANDLE_TABLE_HPP

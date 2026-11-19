// MIT License - (c) Anthony Matarazzo 2026
//
// handle_table.cpp - generation-checked, refcounted handle registry.

#include "handle_table.hpp"

#include <algorithm>
#include <stdexcept>

namespace lowl {

HandleTable::HandleTable(std::size_t reserve) {
    if (reserve != 0) slots_.reserve(reserve);
}

Handle HandleTable::insert(void* payload, Deleter deleter, std::string type_tag) {
    if (payload == nullptr) throw std::invalid_argument("HandleTable::insert: null payload");
    if (deleter == nullptr) throw std::invalid_argument("HandleTable::insert: null deleter");

    Handle handle = kInvalidHandle;
    if (!free_list_.empty()) {
        const SlotIndex index = free_list_.back();
        free_list_.pop_back();
        Slot& slot = slots_[index];
        slot.payload = payload;
        slot.deleter = deleter;
        slot.type_tag = std::move(type_tag);
        slot.refcount = 1;
        slot.pins = 0;
        slot.live = true;
        handle = make_handle(index, slot.generation);
    } else {
        if (slots_.size() >= 0xffffffffu) throw std::length_error("HandleTable: slot space exhausted");
        Slot slot;
        slot.payload = payload;
        slot.deleter = deleter;
        slot.type_tag = std::move(type_tag);
        slot.refcount = 1;
        slot.pins = 0;
        slot.generation = 1;
        slot.live = true;
        slots_.push_back(std::move(slot));
        handle = make_handle(static_cast<SlotIndex>(slots_.size() - 1), 1);
    }

    ++stats_.live;
    ++stats_.total_created;
    stats_.capacity = slots_.size();
    stats_.free_slots = free_list_.size();
    stats_.high_water = std::max(stats_.high_water, stats_.live);
    return handle;
}

Handle HandleTable::insert_pinned(void* payload, Deleter deleter, std::string type_tag) {
    const Handle handle = insert(payload, deleter, std::move(type_tag));
    pin(handle);
    return handle;
}

HandleTable::Slot* HandleTable::find(Handle handle) {
    if (handle == kInvalidHandle) return nullptr;
    const SlotIndex index = handle_slot(handle);
    if (index >= slots_.size()) return nullptr;
    Slot& slot = slots_[index];
    // Generation is bumped when a slot is recycled, so a mismatch means the
    // handle refers to an object that has already been destroyed.
    if (slot.generation != handle_generation(handle)) {
        ++stats_.stale_rejections;
        return nullptr;
    }
    if (!slot.live) return nullptr;
    return &slot;
}

const HandleTable::Slot* HandleTable::find(Handle handle) const {
    return const_cast<HandleTable*>(this)->find(handle);
}

void HandleTable::destroy_slot(Slot& slot) {
    if (slot.deleter != nullptr && slot.payload != nullptr) slot.deleter(slot.payload);
    slot.payload = nullptr;
    slot.deleter = nullptr;
    slot.type_tag.clear();
    slot.refcount = 0;
    slot.pins = 0;
    slot.live = false;
    ++slot.generation;  // invalidates every outstanding copy of the old handle
    if (slot.generation == 0) slot.generation = 1;
}

bool HandleTable::retain(Handle handle) {
    Slot* slot = find(handle);
    if (slot == nullptr) return false;
    if (slot->refcount <= 0) return false;
    ++slot->refcount;
    return true;
}

bool HandleTable::release(Handle handle) {
    Slot* slot = find(handle);
    if (slot == nullptr) return false;
    if (slot->refcount <= 0) return false;
    --slot->refcount;
    if (slot->refcount > 0) return true;
    if (slot->pins > 0) return true;  // pinned: keep payload, refcount stays 0
    destroy_slot(*slot);
    --stats_.live;
    ++stats_.total_destroyed;
    free_list_.push_back(handle_slot(handle));
    stats_.free_slots = free_list_.size();
    return true;
}

bool HandleTable::pin(Handle handle) {
    Slot* slot = find(handle);
    if (slot == nullptr) return false;
    if (slot->pins == 0) ++stats_.pinned;
    ++slot->pins;
    return true;
}

bool HandleTable::unpin(Handle handle) {
    Slot* slot = find(handle);
    if (slot == nullptr) return false;
    if (slot->pins == 0) return false;
    --slot->pins;
    if (slot->pins == 0) --stats_.pinned;
    if (slot->pins == 0 && slot->refcount <= 0) {
        destroy_slot(*slot);
        --stats_.live;
        ++stats_.total_destroyed;
        free_list_.push_back(handle_slot(handle));
        stats_.free_slots = free_list_.size();
    }
    return true;
}

Resolved HandleTable::resolve(Handle handle) const {
    const Slot* slot = find(handle);
    if (slot == nullptr) return Resolved{};
    return Resolved{slot->payload, slot->generation};
}

Resolved HandleTable::acquire(Handle handle) {
    Slot* slot = find(handle);
    if (slot == nullptr) return Resolved{};
    if (slot->pins == 0) ++stats_.pinned;
    ++slot->pins;
    return Resolved{slot->payload, slot->generation};
}

bool HandleTable::release_borrow(Handle handle) { return unpin(handle); }

int HandleTable::refcount(Handle handle) const {
    const Slot* slot = find(handle);
    return slot == nullptr ? -1 : slot->refcount;
}

bool HandleTable::is_pinned(Handle handle) const {
    const Slot* slot = find(handle);
    return slot != nullptr && slot->pins > 0;
}

bool HandleTable::is_valid(Handle handle) const { return find(handle) != nullptr; }

std::string HandleTable::type_of(Handle handle) const {
    const Slot* slot = find(handle);
    return slot == nullptr ? std::string() : slot->type_tag;
}

std::vector<Handle> HandleTable::live_handles() const {
    std::vector<Handle> handles;
    handles.reserve(stats_.live);
    for (SlotIndex i = 0; i < slots_.size(); ++i) {
        if (slots_[i].live) handles.push_back(make_handle(i, slots_[i].generation));
    }
    return handles;
}

HandleStats HandleTable::stats() const {
    HandleStats snapshot = stats_;
    snapshot.capacity = slots_.size();
    snapshot.free_slots = free_list_.size();
    snapshot.pinned = 0;
    snapshot.live = 0;
    for (const Slot& slot : slots_) {
        if (!slot.live) continue;
        ++snapshot.live;
        if (slot.pins > 0) ++snapshot.pinned;
    }
    return snapshot;
}

void HandleTable::clear() {
    for (Slot& slot : slots_) {
        if (!slot.live) continue;
        if (slot.pins > 0) continue;  // pinned singletons survive clear()
        destroy_slot(slot);
        --stats_.live;
        ++stats_.total_destroyed;
        free_list_.push_back(static_cast<SlotIndex>(&slot - slots_.data()));
    }
    stats_.free_slots = free_list_.size();
}

}  // namespace lowl

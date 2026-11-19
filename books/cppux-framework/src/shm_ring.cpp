// MIT License - (c) Anthony Matarazzo 2026
//
// shm_ring.cpp - implementation of the shared memory ring buffer.

#include "shm_ring.hpp"

#include <algorithm>
#include <cstring>

namespace cppux {

namespace {

const uint32_t* crc_table() {
    static uint32_t table[256];
    static bool ready = false;
    if (!ready) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        ready = true;
    }
    return table;
}

size_t round_up_pow2(size_t v) {
    if (v <= 1) return 1;
    size_t p = 1;
    while (p < v) p <<= 1;
    return p;
}

} // namespace

uint32_t shm_ring::crc32(const void* data, size_t size) {
    const uint32_t* table = crc_table();
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < size; ++i) {
        c = table[(c ^ p[i]) & 0xFFu] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFu;
}

shm_ring::shm_ring(size_t capacity_bytes) {
    capacity_ = round_up_pow2(std::max(capacity_bytes, min_capacity));
    buffer_.assign(capacity_, 0);
}

void shm_ring::reset() {
    std::fill(buffer_.begin(), buffer_.end(), 0);
    read_pos_ = 0;
    write_pos_ = 0;
    used_ = 0;
    frames_ = 0;
    next_sequence_ = 1;
    stats_ = stats{};
}

void shm_ring::clear_bytes() {
    std::fill(buffer_.begin(), buffer_.end(), 0);
    read_pos_ = 0;
    write_pos_ = 0;
    used_ = 0;
    frames_ = 0;
    next_sequence_ = 1;
}

void shm_ring::copy_in(size_t offset, const void* src, size_t n) {
    size_t pos = (write_pos_ + offset) & mask();
    size_t first = std::min(n, capacity_ - pos);
    std::memcpy(&buffer_[pos], src, first);
    if (n > first) {
        std::memcpy(&buffer_[0], static_cast<const uint8_t*>(src) + first, n - first);
    }
}

void shm_ring::copy_out(size_t offset, void* dst, size_t n) const {
    size_t pos = (read_pos_ + offset) & mask();
    size_t first = std::min(n, capacity_ - pos);
    std::memcpy(dst, &buffer_[pos], first);
    if (n > first) {
        std::memcpy(static_cast<uint8_t*>(dst) + first, &buffer_[0], n - first);
    }
}

uint32_t shm_ring::load_u32(size_t offset) const {
    size_t pos = (read_pos_ + offset) & mask();
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
        v |= static_cast<uint32_t>(buffer_[(pos + static_cast<size_t>(i)) & mask()]) << (8 * i);
    }
    return v;
}

uint16_t shm_ring::load_u16(size_t offset) const {
    size_t pos = (read_pos_ + offset) & mask();
    uint16_t v = 0;
    for (int i = 0; i < 2; ++i) {
        v |= static_cast<uint16_t>(buffer_[(pos + static_cast<size_t>(i)) & mask()]) << (8 * i);
    }
    return v;
}

void shm_ring::store_u32(size_t offset, uint32_t v) {
    uint8_t tmp[4];
    for (int i = 0; i < 4; ++i) tmp[i] = static_cast<uint8_t>((v >> (8 * i)) & 0xFFu);
    copy_in(offset, tmp, 4);
}

void shm_ring::store_u16(size_t offset, uint16_t v) {
    uint8_t tmp[2];
    for (int i = 0; i < 2; ++i) tmp[i] = static_cast<uint8_t>((v >> (8 * i)) & 0xFFu);
    copy_in(offset, tmp, 2);
}

bool shm_ring::push(const void* data, size_t size, uint16_t type, uint32_t client_id) {
    frame_header h;
    h.type = type;
    h.client_id = client_id;
    h.sequence = next_sequence_;
    if (push_frame(h, data, size)) {
        ++next_sequence_;
        return true;
    }
    return false;
}

bool shm_ring::push_frame(const frame_header& header, const void* data, size_t size) {
    if (size > capacity_ - header_size) {
        ++stats_.push_rejects;
        return false;
    }
    size_t total = header_size + size;

    if (total > free_space()) {
        if (policy_ == overflow_policy::drop_oldest) {
            while (frames_ > 0 && total > free_space()) {
                discard();
            }
        }
        if (total > free_space()) {
            ++stats_.push_rejects;
            return false;
        }
    }

    frame_header h = header;
    h.magic = frame_magic;
    h.length = static_cast<uint32_t>(size);
    h.crc = crc32(data, size);

    store_u32(0, h.magic);
    store_u32(4, h.length);
    store_u32(8, h.client_id);
    store_u32(12, h.sequence);
    store_u16(16, h.type);
    store_u16(18, h.flags);
    store_u32(20, h.crc);

    if (size > 0 && data != nullptr) {
        copy_in(header_size, data, size);
    }
    if ((write_pos_ + total) >= capacity_) {
        ++stats_.wraps;
    }

    write_pos_ = (write_pos_ + total) & mask();
    used_ += total;
    ++frames_;
    ++stats_.frames_pushed;
    stats_.bytes_in += total;
    return true;
}

size_t shm_ring::front_frame_size() const {
    if (frames_ == 0) return 0;
    uint32_t length = load_u32(4);
    if (length > used_ - header_size) return 0;
    return header_size + length;
}

bool shm_ring::peek_header(frame_header& out) const {
    if (frames_ == 0) return false;
    out.magic = load_u32(0);
    out.length = load_u32(4);
    out.client_id = load_u32(8);
    out.sequence = load_u32(12);
    out.type = load_u16(16);
    out.flags = load_u16(18);
    out.crc = load_u32(20);
    return out.magic == frame_magic;
}

bool shm_ring::peek(frame& out) const {
    frame_header h;
    if (!peek_header(h)) return false;
    if (h.length > used_ - header_size) return false;
    out.header = h;
    out.payload.resize(h.length);
    if (h.length > 0) copy_out(header_size, out.payload.data(), h.length);
    return true;
}

bool shm_ring::pop(frame& out) {
    if (frames_ == 0) return false;
    frame_header h;
    if (!peek_header(h)) {
        // The front of the ring is not a frame we wrote: recover by clearing.
        ++stats_.corrupt_frames;
        clear_bytes();
        return false;
    }
    if (h.length > used_ - header_size) {
        ++stats_.corrupt_frames;
        clear_bytes();
        return false;
    }
    out.header = h;
    out.payload.resize(h.length);
    if (h.length > 0) copy_out(header_size, out.payload.data(), h.length);

    uint32_t actual = crc32(out.payload.data(), out.payload.size());
    if (actual != h.crc) {
        ++stats_.corrupt_frames;
        clear_bytes();
        return false;
    }

    size_t total = header_size + h.length;
    read_pos_ = (read_pos_ + total) & mask();
    used_ -= total;
    --frames_;
    ++stats_.frames_popped;
    stats_.bytes_out += total;
    return true;
}

bool shm_ring::discard() {
    frame_header h;
    if (!peek_header(h)) return false;
    if (h.length > used_ - header_size) return false;
    size_t total = header_size + h.length;
    read_pos_ = (read_pos_ + total) & mask();
    used_ -= total;
    --frames_;
    ++stats_.frames_discarded;
    return true;
}

} // namespace cppux

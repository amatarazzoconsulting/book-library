// MIT License - (c) Anthony Matarazzo 2026
//
// shm_ring.hpp - Shared memory ring buffer for the CPPUX client/desktop channel.
//
// The desktop manager hands every client one shared memory region (see
// shared_memory_region in the book). This class is the deterministic, in-process
// model of that region: a single producer (client) appends length prefixed
// frames, a single consumer (desktop) pops them. The buffer is a byte ring with
// power-of-two capacity so index arithmetic is a mask instead of a modulo.
//
// Frame layout on the wire (little endian, 24 byte header):
//   magic u32 | length u32 | client_id u32 | sequence u32
//   type u16 | flags u16 | crc32 u32 | payload[length]
//
// The header is written with memcpy, never by casting a struct, so the layout
// does not depend on compiler padding.

#ifndef CPPUX_SHM_RING_HPP
#define CPPUX_SHM_RING_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cppux {

class shm_ring {
public:
    static constexpr uint32_t frame_magic = 0x58555043u; // bytes 'C','P','U','X'
    static constexpr size_t header_size = 24;
    static constexpr size_t min_capacity = 128;

    struct frame_header {
        uint32_t magic = frame_magic;
        uint32_t length = 0;
        uint32_t client_id = 0;
        uint32_t sequence = 0;
        uint16_t type = 0;
        uint16_t flags = 0;
        uint32_t crc = 0;
    };

    struct frame {
        frame_header header;
        std::vector<uint8_t> payload;
    };

    // What to do when a push does not fit.
    enum class overflow_policy {
        reject,      // push returns false, the ring is untouched
        drop_oldest  // discard complete frames from the front until it fits
    };

    struct stats {
        uint64_t frames_pushed = 0;
        uint64_t frames_popped = 0;
        uint64_t frames_discarded = 0;
        uint64_t bytes_in = 0;
        uint64_t bytes_out = 0;
        uint64_t push_rejects = 0;
        uint64_t corrupt_frames = 0;
        uint64_t wraps = 0;
    };

    explicit shm_ring(size_t capacity_bytes);

    size_t capacity() const { return capacity_; }
    size_t used() const { return used_; }
    size_t free_space() const { return capacity_ - used_; }
    size_t frames() const { return frames_; }
    bool empty() const { return frames_ == 0; }

    // Clear all bytes and counters, keep the capacity.
    void reset();

    void set_policy(overflow_policy p) { policy_ = p; }
    overflow_policy policy() const { return policy_; }

    // Producer side. Sequence numbers are assigned automatically (starting at 1)
    // unless the caller supplies one through push_frame().
    bool push(const void* data, size_t size, uint16_t type = 0, uint32_t client_id = 0);

    // Producer side with an explicit header. length and crc are recomputed from
    // the payload; magic is forced to frame_magic.
    bool push_frame(const frame_header& header, const void* data, size_t size);

    // Consumer side. pop() copies the payload out and consumes the frame.
    bool pop(frame& out);
    // peek() copies without consuming.
    bool peek(frame& out) const;
    // Read only the header of the oldest frame.
    bool peek_header(frame_header& out) const;
    // Drop the oldest frame without copying its payload.
    bool discard();

    const stats& counters() const { return stats_; }

    // Direct view of the backing store. Real code would never touch this; it
    // exists so tests and the teaching program can inspect and deliberately
    // corrupt bytes to show what validation does.
    uint8_t* raw() { return buffer_.data(); }
    const uint8_t* raw() const { return buffer_.data(); }
    size_t raw_index(size_t offset) const { return (read_pos_ + offset) & mask(); }

    static uint32_t crc32(const void* data, size_t size);

    // Bytes of the oldest frame that are still ahead of the read cursor.
    size_t front_frame_size() const;

private:
    size_t mask() const { return capacity_ - 1; }
    // Empty the ring after a fault without losing the counters.
    void clear_bytes();
    void copy_in(size_t offset, const void* src, size_t n);
    void copy_out(size_t offset, void* dst, size_t n) const;
    uint32_t load_u32(size_t offset) const;
    uint16_t load_u16(size_t offset) const;
    void store_u32(size_t offset, uint32_t v);
    void store_u16(size_t offset, uint16_t v);

    std::vector<uint8_t> buffer_;
    size_t capacity_ = 0;
    size_t read_pos_ = 0;
    size_t write_pos_ = 0;
    size_t used_ = 0;
    size_t frames_ = 0;
    uint32_t next_sequence_ = 1;
    overflow_policy policy_ = overflow_policy::reject;
    stats stats_;
};

} // namespace cppux

#endif // CPPUX_SHM_RING_HPP

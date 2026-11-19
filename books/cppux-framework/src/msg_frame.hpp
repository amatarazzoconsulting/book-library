// MIT License - (c) Anthony Matarazzo 2026
//
// msg_frame.hpp - Message framing and validation for the CPPUX desktop channel.
//
// The book's rule is that "the desktop validates all client messages". A client
// is untrusted, so every byte that arrives is checked before it reaches a
// handler: magic, protocol version, message type whitelist, declared length,
// CRC32, client id range, timestamp skew and per-client sequence monotonicity.
//
// A frame on the wire (32 byte header, little endian):
//   magic u32 | version u16 | type u16 | client_id u32 | sequence u32
//   payload_size u32 | timestamp_ns u64 | crc32 u32
//
// The reader is a streaming decoder: bytes may arrive in arbitrary chunks, and
// it only emits complete, CRC-checked frames.

#ifndef CPPUX_MSG_FRAME_HPP
#define CPPUX_MSG_FRAME_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace cppux {

constexpr uint32_t frame_magic = 0x43505558u; // 'C','P','U','X'
constexpr uint16_t protocol_version = 4;      // CPPUX_VERSION 4.0.0
constexpr size_t frame_header_size = 32;

// Mirrors message_type in common.h.
enum class message_type : uint16_t {
    register_client = 0,
    unregister_client = 1,
    dom_update = 2,
    buffer_update = 3,
    event_ack = 4,
    event_dispatch = 5,
    ping = 6,
    pong = 7,
    sync_request = 8,
    sync_response = 9,
    _count = 10
};

const char* to_string(message_type t);

enum class frame_error : uint8_t {
    none = 0,
    short_header,
    bad_magic,
    bad_version,
    unknown_type,
    payload_too_large,
    crc_mismatch,
    bad_client_id,
    stale_timestamp,
    future_timestamp,
    sequence_regression,
    truncated_payload
};

const char* to_string(frame_error e);

struct wire_header {
    uint32_t magic = frame_magic;
    uint16_t version = protocol_version;
    uint16_t type = 0;
    uint32_t client_id = 0;
    uint32_t sequence = 0;
    uint32_t payload_size = 0;
    uint64_t timestamp_ns = 0;
    uint32_t crc = 0;
};

struct frame {
    wire_header header;
    std::vector<uint8_t> payload;

    message_type type() const { return static_cast<message_type>(header.type); }
};

// Every knob the desktop validator exposes.
struct validator_config {
    size_t max_payload = 1024 * 1024;      // MAX_DOM_SIZE from the book
    uint32_t min_client_id = 1;
    uint32_t max_client_id = 255;          // MAX_CLIENTS - 1
    uint64_t max_clock_skew_ns = 5000000000ull; // 5 s into the future
    uint64_t max_age_ns = 60000000000ull;       // 60 s in the past
    bool require_crc = true;
    bool require_monotonic_sequence = true;
    bool allow_all_types = false;
    uint32_t allowed_types = 0;            // bitmask over message_type values

    static validator_config strict();
    static validator_config permissive();
};

struct validation_result {
    frame_error error = frame_error::none;
    std::string detail;

    bool ok() const { return error == frame_error::none; }
    explicit operator bool() const { return ok(); }
};

// Encode one frame into out. Returns bytes written, 0 if out is too small.
size_t encode_frame(const frame& f, uint8_t* out, size_t capacity);
std::vector<uint8_t> encode_frame(const frame& f);

// Streaming decoder. feed() as many bytes as you like, then drain with next().
class frame_reader {
public:
    explicit frame_reader(size_t max_payload = 1024 * 1024) : max_payload_(max_payload) {}

    void feed(const void* data, size_t size);
    void feed(const std::vector<uint8_t>& bytes) { feed(bytes.data(), bytes.size()); }

    // Returns true when a complete frame was decoded. On false, inspect error().
    bool next(frame& out);

    frame_error error() const { return last_error_; }
    const std::string& error_detail() const { return last_detail_; }

    size_t buffered() const { return buffer_.size() - cursor_; }
    size_t frames_decoded() const { return decoded_; }
    uint64_t bytes_consumed() const { return consumed_; }
    // Drop everything still buffered (used after a fatal framing error).
    void reset();

private:
    bool take_header(wire_header& h);
    void consume(size_t n);

    std::vector<uint8_t> buffer_;
    size_t cursor_ = 0;
    size_t max_payload_ = 0;
    size_t decoded_ = 0;
    uint64_t consumed_ = 0;
    frame_error last_error_ = frame_error::none;
    std::string last_detail_;
};

// Stateful validator: owns the per-client sequence watermark.
class validator {
public:
    explicit validator(validator_config cfg = validator_config::strict()) : cfg_(cfg) {}

    validation_result check(const frame& f, uint64_t now_ns);

    // Convenience: decode plus validate from a byte buffer.
    validation_result check_bytes(const uint8_t* data, size_t size, uint64_t now_ns,
                                  frame* out = nullptr);

    void forget_client(uint32_t client_id);
    void reset() { last_sequence_.clear(); }
    const validator_config& config() const { return cfg_; }

    uint64_t accepted() const { return accepted_; }
    uint64_t rejected() const { return rejected_; }

    static uint32_t crc32(const void* data, size_t size);

private:
    validator_config cfg_;
    std::unordered_map<uint32_t, uint32_t> last_sequence_;
    uint64_t accepted_ = 0;
    uint64_t rejected_ = 0;
};

} // namespace cppux

#endif // CPPUX_MSG_FRAME_HPP

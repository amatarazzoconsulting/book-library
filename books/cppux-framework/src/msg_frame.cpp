// MIT License - (c) Anthony Matarazzo 2026
//
// msg_frame.cpp - framing, decoding and validation for client messages.

#include "msg_frame.hpp"

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

void put_u16(uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
}

void put_u32(uint8_t* p, uint32_t v) {
    for (int i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>((v >> (8 * i)) & 0xFFu);
}

void put_u64(uint8_t* p, uint64_t v) {
    for (int i = 0; i < 8; ++i) p[i] = static_cast<uint8_t>((v >> (8 * i)) & 0xFFu);
}

uint16_t get_u16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

uint32_t get_u32(const uint8_t* p) {
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= static_cast<uint32_t>(p[i]) << (8 * i);
    return v;
}

uint64_t get_u64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(p[i]) << (8 * i);
    return v;
}

void write_header(const wire_header& h, uint8_t* p) {
    put_u32(p + 0, h.magic);
    put_u16(p + 4, h.version);
    put_u16(p + 6, h.type);
    put_u32(p + 8, h.client_id);
    put_u32(p + 12, h.sequence);
    put_u32(p + 16, h.payload_size);
    put_u64(p + 20, h.timestamp_ns);
    put_u32(p + 28, h.crc);
}

wire_header read_header(const uint8_t* p) {
    wire_header h;
    h.magic = get_u32(p + 0);
    h.version = get_u16(p + 4);
    h.type = get_u16(p + 6);
    h.client_id = get_u32(p + 8);
    h.sequence = get_u32(p + 12);
    h.payload_size = get_u32(p + 16);
    h.timestamp_ns = get_u64(p + 20);
    h.crc = get_u32(p + 28);
    return h;
}

// CRC covers the header with the crc field zeroed plus the payload.
uint32_t frame_crc(const wire_header& h, const uint8_t* payload) {
    const uint32_t* table = crc_table();
    wire_header copy = h;
    copy.crc = 0;
    uint8_t header[frame_header_size];
    write_header(copy, header);

    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < frame_header_size; ++i) {
        c = table[(c ^ header[i]) & 0xFFu] ^ (c >> 8);
    }
    for (size_t i = 0; i < h.payload_size; ++i) {
        c = table[(c ^ payload[i]) & 0xFFu] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFu;
}

} // namespace

const char* to_string(message_type t) {
    switch (t) {
        case message_type::register_client: return "register_client";
        case message_type::unregister_client: return "unregister_client";
        case message_type::dom_update: return "dom_update";
        case message_type::buffer_update: return "buffer_update";
        case message_type::event_ack: return "event_ack";
        case message_type::event_dispatch: return "event_dispatch";
        case message_type::ping: return "ping";
        case message_type::pong: return "pong";
        case message_type::sync_request: return "sync_request";
        case message_type::sync_response: return "sync_response";
        case message_type::_count: break;
    }
    return "unknown";
}

const char* to_string(frame_error e) {
    switch (e) {
        case frame_error::none: return "none";
        case frame_error::short_header: return "short_header";
        case frame_error::bad_magic: return "bad_magic";
        case frame_error::bad_version: return "bad_version";
        case frame_error::unknown_type: return "unknown_type";
        case frame_error::payload_too_large: return "payload_too_large";
        case frame_error::crc_mismatch: return "crc_mismatch";
        case frame_error::bad_client_id: return "bad_client_id";
        case frame_error::stale_timestamp: return "stale_timestamp";
        case frame_error::future_timestamp: return "future_timestamp";
        case frame_error::sequence_regression: return "sequence_regression";
        case frame_error::truncated_payload: return "truncated_payload";
    }
    return "unknown";
}

validator_config validator_config::strict() {
    validator_config c;
    c.allowed_types = (1u << static_cast<uint32_t>(message_type::register_client)) |
                      (1u << static_cast<uint32_t>(message_type::unregister_client)) |
                      (1u << static_cast<uint32_t>(message_type::dom_update)) |
                      (1u << static_cast<uint32_t>(message_type::buffer_update)) |
                      (1u << static_cast<uint32_t>(message_type::event_ack)) |
                      (1u << static_cast<uint32_t>(message_type::sync_request)) |
                      (1u << static_cast<uint32_t>(message_type::ping));
    return c;
}

validator_config validator_config::permissive() {
    validator_config c;
    c.require_crc = false;
    c.require_monotonic_sequence = false;
    c.allow_all_types = true;
    c.max_payload = 64 * 1024 * 1024;
    c.max_client_id = 0xFFFFFFFFu;
    c.min_client_id = 0;
    c.max_clock_skew_ns = ~0ull;
    c.max_age_ns = ~0ull;
    return c;
}

uint32_t validator::crc32(const void* data, size_t size) {
    const uint32_t* table = crc_table();
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < size; ++i) {
        c = table[(c ^ p[i]) & 0xFFu] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFu;
}

size_t encode_frame(const frame& f, uint8_t* out, size_t capacity) {
    size_t total = frame_header_size + f.payload.size();
    if (capacity < total) return 0;
    wire_header h = f.header;
    h.magic = frame_magic;
    h.version = protocol_version;
    h.payload_size = static_cast<uint32_t>(f.payload.size());
    h.crc = frame_crc(h, f.payload.data());
    write_header(h, out);
    if (!f.payload.empty()) {
        std::memcpy(out + frame_header_size, f.payload.data(), f.payload.size());
    }
    return total;
}

std::vector<uint8_t> encode_frame(const frame& f) {
    std::vector<uint8_t> out(frame_header_size + f.payload.size());
    size_t n = encode_frame(f, out.data(), out.size());
    out.resize(n);
    return out;
}

void frame_reader::feed(const void* data, size_t size) {
    if (size == 0) return;
    const uint8_t* p = static_cast<const uint8_t*>(data);
    buffer_.insert(buffer_.end(), p, p + size);
}

void frame_reader::consume(size_t n) {
    cursor_ += n;
    consumed_ += n;
    if (cursor_ >= buffer_.size()) {
        buffer_.clear();
        cursor_ = 0;
    } else if (cursor_ > 4096 && cursor_ * 2 > buffer_.size()) {
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(cursor_));
        cursor_ = 0;
    }
}

void frame_reader::reset() {
    buffer_.clear();
    cursor_ = 0;
}

bool frame_reader::take_header(wire_header& h) {
    if (buffered() < frame_header_size) {
        last_error_ = frame_error::short_header;
        last_detail_ = "need " + std::to_string(frame_header_size) + " header bytes, have " +
                       std::to_string(buffered());
        return false;
    }
    h = read_header(buffer_.data() + cursor_);
    if (h.magic != frame_magic) {
        last_error_ = frame_error::bad_magic;
        last_detail_ = "magic mismatch";
        return false;
    }
    if (h.version != protocol_version) {
        last_error_ = frame_error::bad_version;
        last_detail_ = "version " + std::to_string(h.version) + " != " +
                       std::to_string(protocol_version);
        return false;
    }
    if (h.type >= static_cast<uint16_t>(message_type::_count)) {
        last_error_ = frame_error::unknown_type;
        last_detail_ = "type " + std::to_string(h.type);
        return false;
    }
    if (h.payload_size > max_payload_) {
        last_error_ = frame_error::payload_too_large;
        last_detail_ = "payload " + std::to_string(h.payload_size) + " > " +
                       std::to_string(max_payload_);
        return false;
    }
    last_error_ = frame_error::none;
    last_detail_.clear();
    return true;
}

bool frame_reader::next(frame& out) {
    wire_header h;
    if (!take_header(h)) return false;

    size_t need = frame_header_size + h.payload_size;
    if (buffered() < need) {
        last_error_ = frame_error::truncated_payload;
        last_detail_ = "have " + std::to_string(buffered()) + " of " + std::to_string(need);
        return false;
    }

    const uint8_t* payload = buffer_.data() + cursor_ + frame_header_size;
    uint32_t expected = frame_crc(h, payload);
    if (expected != h.crc) {
        last_error_ = frame_error::crc_mismatch;
        last_detail_ = "crc mismatch";
        return false;
    }

    out.header = h;
    out.payload.assign(payload, payload + h.payload_size);
    consume(need);
    ++decoded_;
    last_error_ = frame_error::none;
    last_detail_.clear();
    return true;
}

validation_result validator::check(const frame& f, uint64_t now_ns) {
    validation_result r;
    const wire_header& h = f.header;

    auto fail = [&](frame_error e, const std::string& detail) {
        r.error = e;
        r.detail = detail;
        ++rejected_;
        return r;
    };

    if (h.magic != frame_magic) return fail(frame_error::bad_magic, "magic mismatch");
    if (h.version != protocol_version) {
        return fail(frame_error::bad_version, "version " + std::to_string(h.version));
    }
    if (h.type >= static_cast<uint16_t>(message_type::_count)) {
        return fail(frame_error::unknown_type, "type " + std::to_string(h.type));
    }
    if (!cfg_.allow_all_types && (cfg_.allowed_types & (1u << h.type)) == 0) {
        return fail(frame_error::unknown_type,
                    std::string("type not allowed: ") + to_string(static_cast<message_type>(h.type)));
    }
    if (f.payload.size() != h.payload_size) {
        return fail(frame_error::truncated_payload, "declared size differs from payload");
    }
    if (f.payload.size() > cfg_.max_payload) {
        return fail(frame_error::payload_too_large, "payload " + std::to_string(f.payload.size()));
    }
    if (h.client_id < cfg_.min_client_id || h.client_id > cfg_.max_client_id) {
        return fail(frame_error::bad_client_id, "client " + std::to_string(h.client_id));
    }
    if (now_ns > h.timestamp_ns && now_ns - h.timestamp_ns > cfg_.max_age_ns) {
        return fail(frame_error::stale_timestamp,
                    std::to_string((now_ns - h.timestamp_ns) / 1000000ull) + " ms old");
    }
    if (h.timestamp_ns > now_ns && h.timestamp_ns - now_ns > cfg_.max_clock_skew_ns) {
        return fail(frame_error::future_timestamp,
                    std::to_string((h.timestamp_ns - now_ns) / 1000000ull) + " ms ahead");
    }
    if (cfg_.require_crc) {
        uint32_t actual = frame_crc(h, f.payload.data());
        if (actual != h.crc) return fail(frame_error::crc_mismatch, "crc mismatch");
    }
    if (cfg_.require_monotonic_sequence) {
        auto it = last_sequence_.find(h.client_id);
        if (it != last_sequence_.end() && h.sequence <= it->second) {
            return fail(frame_error::sequence_regression,
                        "seq " + std::to_string(h.sequence) + " <= " + std::to_string(it->second));
        }
        last_sequence_[h.client_id] = h.sequence;
    }

    ++accepted_;
    return r;
}

validation_result validator::check_bytes(const uint8_t* data, size_t size, uint64_t now_ns,
                                         frame* out) {
    frame_reader reader(cfg_.max_payload);
    reader.feed(data, size);
    frame f;
    if (!reader.next(f)) {
        validation_result r;
        r.error = reader.error();
        r.detail = reader.error_detail();
        ++rejected_;
        return r;
    }
    if (out) *out = f;
    return check(f, now_ns);
}

void validator::forget_client(uint32_t client_id) {
    last_sequence_.erase(client_id);
}

} // namespace cppux

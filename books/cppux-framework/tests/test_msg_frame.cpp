// MIT License - (c) Anthony Matarazzo 2026
// tests/test_msg_frame.cpp - framing, streaming decode and validation.

#include "msg_frame.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace cppux;

static int failures = 0;

static void check(bool cond, const char* what) {
    if (cond) {
        std::printf("PASS %s\n", what);
    } else {
        std::printf("FAIL %s\n", what);
        ++failures;
    }
}

static frame make_frame(uint16_t type, uint32_t client, uint32_t seq, const std::string& payload,
                        uint64_t ts) {
    frame f;
    f.header.magic = frame_magic;
    f.header.version = protocol_version;
    f.header.type = type;
    f.header.client_id = client;
    f.header.sequence = seq;
    f.header.timestamp_ns = ts;
    f.payload.assign(payload.begin(), payload.end());
    // Build the frame the way a client does: encode it, then decode it back so
    // payload_size and crc are the values the wire really carries.
    auto wire = encode_frame(f);
    frame_reader reader;
    reader.feed(wire);
    frame decoded;
    reader.next(decoded);
    return decoded;
}

int main() {
    const uint64_t now = 1000000000000ull; // 1000 s in ns

    // round trip through encode and the streaming reader
    {
        frame f = make_frame(static_cast<uint16_t>(message_type::dom_update), 5, 1,
                             "<doc>hi</doc>", now);
        auto wire = encode_frame(f);
        check(wire.size() == frame_header_size + f.payload.size(), "encoded size is header plus payload");

        frame_reader reader;
        reader.feed(wire);
        frame out;
        check(reader.next(out), "reader decodes a complete frame");
        check(out.header.client_id == 5 && out.header.sequence == 1, "header fields decoded");
        check(std::string(out.payload.begin(), out.payload.end()) == "<doc>hi</doc>",
              "payload decoded exactly");
        check(reader.frames_decoded() == 1, "decoded counter is one");
        check(reader.buffered() == 0, "no bytes left buffered");
    }

    // byte at a time streaming
    {
        frame a = make_frame(static_cast<uint16_t>(message_type::ping), 2, 1, "ab", now);
        frame b = make_frame(static_cast<uint16_t>(message_type::pong), 2, 2, "cdef", now);
        auto wa = encode_frame(a);
        auto wb = encode_frame(b);
        std::vector<uint8_t> all;
        all.insert(all.end(), wa.begin(), wa.end());
        all.insert(all.end(), wb.begin(), wb.end());

        frame_reader reader;
        frame out;
        int decoded = 0;
        for (uint8_t byte : all) {
            reader.feed(&byte, 1);
            while (reader.next(out)) ++decoded;
        }
        check(decoded == 2, "two frames decoded one byte at a time");
    }

    // partial frame is reported as truncated, not as a frame
    {
        frame f = make_frame(static_cast<uint16_t>(message_type::ping), 1, 1, "0123456789", now);
        auto wire = encode_frame(f);
        frame_reader reader;
        reader.feed(wire.data(), wire.size() - 3);
        frame out;
        check(!reader.next(out), "truncated frame not emitted");
        check(reader.error() == frame_error::truncated_payload, "truncation reported");
        reader.feed(wire.data() + wire.size() - 3, 3);
        check(reader.next(out), "frame completes when the tail arrives");
    }

    // header-only feed reports short_header
    {
        frame_reader reader;
        uint8_t one = 0;
        reader.feed(&one, 1);
        frame out;
        check(!reader.next(out), "single byte is not a frame");
        check(reader.error() == frame_error::short_header, "short_header reported");
    }

    // bad magic, bad version, unknown type, oversized payload
    {
        frame f = make_frame(static_cast<uint16_t>(message_type::ping), 1, 1, "x", now);
        auto wire = encode_frame(f);

        auto bad = wire;
        bad[0] ^= 0xFF;
        frame_reader r1;
        r1.feed(bad);
        frame out;
        check(!r1.next(out) && r1.error() == frame_error::bad_magic, "bad magic rejected");

        auto badver = wire;
        badver[4] = 99;
        frame_reader r2;
        r2.feed(badver);
        check(!r2.next(out) && r2.error() == frame_error::bad_version, "bad version rejected");

        auto badtype = wire;
        badtype[6] = 200;
        frame_reader r3;
        r3.feed(badtype);
        check(!r3.next(out) && r3.error() == frame_error::unknown_type, "unknown type rejected");

        frame big = make_frame(static_cast<uint16_t>(message_type::ping), 1, 1, "0123456789", now);
        auto big_wire = encode_frame(big);
        frame_reader r4(4);
        r4.feed(big_wire);
        check(!r4.next(out) && r4.error() == frame_error::payload_too_large,
              "payload over the reader limit rejected");
    }

    // crc catches a flipped payload bit
    {
        frame f = make_frame(static_cast<uint16_t>(message_type::dom_update), 3, 1, "abcdef", now);
        auto wire = encode_frame(f);
        wire[frame_header_size + 2] ^= 0x01;
        frame_reader reader;
        reader.feed(wire);
        frame out;
        check(!reader.next(out) && reader.error() == frame_error::crc_mismatch, "crc mismatch rejected");
    }

    // validator accepts a well formed frame
    {
        validator v;
        frame f = make_frame(static_cast<uint16_t>(message_type::dom_update), 7, 1, "ok", now);
        auto wire = encode_frame(f);
        frame decoded;
        auto r = v.check_bytes(wire.data(), wire.size(), now, &decoded);
        check(r.ok(), "validator accepts a valid frame");
        check(v.accepted() == 1 && v.rejected() == 0, "accept counters move");
        check(decoded.payload.size() == 2, "decoded payload available to caller");
    }

    // client id bounds
    {
        validator v;
        frame low = make_frame(static_cast<uint16_t>(message_type::ping), 0, 1, "", now);
        frame high = make_frame(static_cast<uint16_t>(message_type::ping), 999, 1, "", now);
        check(!v.check(low, now).ok(), "client id 0 rejected");
        check(v.check(low, now).error == frame_error::bad_client_id, "bad_client_id reported for 0");
        check(!v.check(high, now).ok(), "client id 999 rejected");
    }

    // timestamp skew both directions
    {
        validator v;
        frame old = make_frame(static_cast<uint16_t>(message_type::ping), 1, 1, "",
                               now - 120ull * 1000000000ull);
        frame future = make_frame(static_cast<uint16_t>(message_type::ping), 1, 1, "",
                                  now + 10ull * 1000000000ull);
        check(v.check(old, now).error == frame_error::stale_timestamp, "stale timestamp rejected");
        check(v.check(future, now).error == frame_error::future_timestamp,
              "future timestamp rejected");
    }

    // sequence must increase per client
    {
        validator v;
        frame a = make_frame(static_cast<uint16_t>(message_type::ping), 4, 5, "", now);
        frame b = make_frame(static_cast<uint16_t>(message_type::ping), 4, 5, "", now);
        frame c = make_frame(static_cast<uint16_t>(message_type::ping), 4, 4, "", now);
        frame d = make_frame(static_cast<uint16_t>(message_type::ping), 4, 6, "", now);
        check(v.check(a, now).ok(), "first sequence accepted");
        check(v.check(b, now).error == frame_error::sequence_regression, "duplicate sequence rejected");
        check(v.check(c, now).error == frame_error::sequence_regression, "lower sequence rejected");
        check(v.check(d, now).ok(), "higher sequence accepted");

        v.forget_client(4);
        check(v.check(b, now).ok(), "forget_client resets the watermark");
    }

    // type whitelist
    {
        validator v;
        frame dispatch = make_frame(static_cast<uint16_t>(message_type::event_dispatch), 1, 1, "", now);
        auto r = v.check(dispatch, now);
        check(!r.ok(), "desktop-to-client type rejected from a client");
        check(r.error == frame_error::unknown_type, "type not in whitelist reported");

        validator p(validator_config::permissive());
        check(p.check(dispatch, now).ok(), "permissive config allows every type");
    }

    // crc can be switched off
    {
        validator_config cfg = validator_config::permissive();
        cfg.require_crc = false;
        validator v(cfg);
        frame f = make_frame(static_cast<uint16_t>(message_type::ping), 1, 1, "abc", now);
        f.header.crc = 0;
        check(v.check(f, now).ok(), "crc check disabled accepts a zero crc");
    }

    // payload size mismatch between header and vector
    {
        validator v;
        frame f = make_frame(static_cast<uint16_t>(message_type::ping), 1, 1, "abc", now);
        auto wire = encode_frame(f);
        frame_reader reader;
        reader.feed(wire);
        frame decoded;
        reader.next(decoded);
        decoded.payload.push_back(0);
        auto r = v.check(decoded, now);
        check(r.error == frame_error::truncated_payload, "payload size mismatch rejected");
    }

    // many frames back to back stay in order
    {
        validator v;
        std::vector<uint8_t> stream;
        for (uint32_t i = 1; i <= 50; ++i) {
            frame f = make_frame(static_cast<uint16_t>(message_type::dom_update), 9, i,
                                 "frame" + std::to_string(i), now);
            auto w = encode_frame(f);
            stream.insert(stream.end(), w.begin(), w.end());
        }
        frame_reader reader;
        reader.feed(stream);
        frame out;
        int n = 0;
        bool ordered = true;
        while (reader.next(out)) {
            ++n;
            if (out.header.sequence != static_cast<uint32_t>(n)) ordered = false;
            if (!v.check(out, now).ok()) ordered = false;
        }
        check(n == 50, "50 frames decoded from one stream");
        check(ordered, "frames arrive in sequence and all validate");
    }

    // error strings are complete
    {
        check(std::string(to_string(frame_error::crc_mismatch)) == "crc_mismatch", "error to_string");
        check(std::string(to_string(message_type::dom_update)) == "dom_update", "type to_string");
    }

    std::printf("%s\n", failures == 0 ? "test_msg_frame: all checks passed"
                                     : "test_msg_frame: FAILURES");
    return failures == 0 ? 0 : 1;
}

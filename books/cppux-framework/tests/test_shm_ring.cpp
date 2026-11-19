// MIT License - (c) Anthony Matarazzo 2026
// tests/test_shm_ring.cpp - normal and edge cases for the shared memory ring.

#include "shm_ring.hpp"

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

static std::vector<uint8_t> bytes(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

static std::string as_string(const std::vector<uint8_t>& v) {
    return std::string(v.begin(), v.end());
}

int main() {
    // capacity rounds up to a power of two
    {
        shm_ring r(1000);
        check(r.capacity() == 1024, "capacity rounds up to 1024");
        check(r.empty() && r.used() == 0, "new ring is empty");
        check(r.free_space() == 1024, "new ring reports full free space");
    }

    // minimum capacity clamp
    {
        shm_ring r(8);
        check(r.capacity() == shm_ring::min_capacity, "tiny capacity clamps to minimum");
    }

    // push then pop round trip
    {
        shm_ring r(4096);
        auto payload = bytes("hello desktop");
        check(r.push(payload.data(), payload.size(), 2, 7), "push small frame");
        check(r.frames() == 1, "frame counted");
        check(r.used() == shm_ring::header_size + payload.size(), "used equals header plus payload");

        shm_ring::frame f;
        check(r.pop(f), "pop returns a frame");
        check(as_string(f.payload) == "hello desktop", "payload survives round trip");
        check(f.header.type == 2 && f.header.client_id == 7, "header fields survive round trip");
        check(f.header.sequence == 1, "first sequence number is 1");
        check(f.header.magic == shm_ring::frame_magic, "magic is set");
        check(r.empty(), "ring is empty after pop");
    }

    // sequence numbers increment
    {
        shm_ring r(4096);
        const char* p = "x";
        r.push(p, 1);
        r.push(p, 1);
        r.push(p, 1);
        shm_ring::frame f;
        r.pop(f);
        check(f.header.sequence == 1, "sequence 1");
        r.pop(f);
        check(f.header.sequence == 2, "sequence 2");
        r.pop(f);
        check(f.header.sequence == 3, "sequence 3");
        check(r.counters().frames_popped == 3, "pop counter tracks three frames");
    }

    // peek does not consume
    {
        shm_ring r(4096);
        auto payload = bytes("peek me");
        r.push(payload.data(), payload.size());
        shm_ring::frame f;
        check(r.peek(f), "peek succeeds");
        check(as_string(f.payload) == "peek me", "peek returns payload");
        check(r.frames() == 1, "peek does not consume");
        check(r.pop(f), "pop after peek");
        check(r.empty(), "empty after pop");
    }

    // wrap around the end of the buffer
    {
        shm_ring r(256);
        std::string s(40, 'a');
        int pushed = 0;
        for (int i = 0; i < 200; ++i) {
            if (!r.push(s.data(), s.size())) break;
            ++pushed;
            shm_ring::frame f;
            if (!r.pop(f)) {
                check(false, "pop during wrap loop");
                break;
            }
        }
        check(pushed == 200, "200 sequential push/pop cycles across wraps");
        check(r.counters().wraps > 0, "at least one wrap recorded");
    }

    // reject policy leaves ring untouched
    {
        shm_ring r(256);
        std::string big(120, 'b');
        check(r.push(big.data(), big.size()), "first big frame fits");
        size_t used_before = r.used();
        check(!r.push(big.data(), big.size()), "second big frame rejected");
        check(r.used() == used_before, "rejected push did not change used bytes");
        check(r.counters().push_rejects == 1, "reject counter incremented");
    }

    // drop_oldest policy discards from the front
    {
        shm_ring r(256);
        r.set_policy(shm_ring::overflow_policy::drop_oldest);
        std::string s(80, 'c');
        for (int i = 0; i < 5; ++i) {
            check(r.push(s.data(), s.size()), "push under drop_oldest");
        }
        check(r.counters().frames_discarded > 0, "drop_oldest discarded frames");
        check(r.used() <= r.capacity(), "used never exceeds capacity");

        shm_ring::frame f;
        bool all_ok = true;
        while (r.pop(f)) {
            if (f.payload.size() != s.size()) all_ok = false;
        }
        check(all_ok, "every surviving frame is intact");
    }

    // oversized payload is rejected
    {
        shm_ring r(128);
        std::vector<uint8_t> huge(1000, 0);
        check(!r.push(huge.data(), huge.size()), "payload larger than capacity rejected");
    }

    // empty payload is legal
    {
        shm_ring r(256);
        check(r.push(nullptr, 0), "zero length push accepted");
        shm_ring::frame f;
        check(r.pop(f), "zero length pop accepted");
        check(f.payload.empty(), "zero length payload");
        check(f.header.crc == shm_ring::crc32(nullptr, 0), "crc of empty payload is stable");
    }

    // crc32 known vector: "123456789" -> 0xCBF43926
    {
        const char* v = "123456789";
        check(shm_ring::crc32(v, 9) == 0xCBF43926u, "crc32 matches the standard check value");
    }

    // corruption is detected and the ring recovers
    {
        shm_ring r(256);
        auto p = bytes("payload");
        r.push(p.data(), p.size());
        shm_ring::frame f;
        check(r.peek(f), "peek before corruption");
        check(r.counters().corrupt_frames == 0, "no corruption on a clean ring");

        // Flip one payload byte in the backing store, the way a buggy or
        // hostile client could, and confirm the crc rejects the frame.
        r.raw()[r.raw_index(shm_ring::header_size)] ^= 0xFF;
        check(!r.pop(f), "corrupted payload is rejected");
        check(r.counters().corrupt_frames == 1, "corrupt counter incremented");
        check(r.empty(), "ring recovered to an empty state");

        // And a bad magic byte is rejected the same way.
        r.push(p.data(), p.size());
        r.raw()[r.raw_index(0)] ^= 0xFF;
        check(!r.pop(f), "corrupted magic is rejected");
        check(r.counters().corrupt_frames == 2, "corrupt counter counts both faults");
    }

    // front_frame_size
    {
        shm_ring r(1024);
        auto p = bytes("12345");
        r.push(p.data(), p.size());
        check(r.front_frame_size() == shm_ring::header_size + 5, "front frame size is header plus payload");
        shm_ring::frame f;
        r.pop(f);
        check(r.front_frame_size() == 0, "front frame size is zero when empty");
    }

    // reset clears everything
    {
        shm_ring r(512);
        auto p = bytes("abc");
        r.push(p.data(), p.size());
        r.reset();
        check(r.empty() && r.used() == 0, "reset empties the ring");
        check(r.counters().frames_pushed == 0, "reset clears counters");
        r.push(p.data(), p.size());
        shm_ring::frame f;
        r.pop(f);
        check(f.header.sequence == 1, "reset restarts sequence numbering");
    }

    // byte accounting
    {
        shm_ring r(4096);
        auto p = bytes("0123456789");
        r.push(p.data(), p.size());
        shm_ring::frame f;
        r.pop(f);
        check(r.counters().bytes_in == shm_ring::header_size + 10, "bytes_in accounts for header");
        check(r.counters().bytes_out == shm_ring::header_size + 10, "bytes_out accounts for header");
    }

    // explicit header path keeps caller sequence
    {
        shm_ring r(1024);
        shm_ring::frame_header h;
        h.sequence = 4242;
        h.type = 9;
        h.client_id = 3;
        auto p = bytes("hdr");
        check(r.push_frame(h, p.data(), p.size()), "push_frame accepted");
        shm_ring::frame f;
        r.pop(f);
        check(f.header.sequence == 4242 && f.header.type == 9, "explicit header preserved");
    }

    std::printf("%s\n", failures == 0 ? "test_shm_ring: all checks passed"
                                     : "test_shm_ring: FAILURES");
    return failures == 0 ? 0 : 1;
}

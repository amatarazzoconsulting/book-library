// MIT License - (c) Anthony Matarazzo 2026
//
// teaching.cpp - guided tour of every option the four channel pieces expose.
// Each block prints what it is about to do, does it, and prints the outcome.

#include "blit_surface.hpp"
#include "msg_frame.hpp"
#include "request_dispatch.hpp"
#include "shm_ring.hpp"

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace cppux;

static std::string text(const std::vector<uint8_t>& v) {
    return std::string(v.begin(), v.end());
}

static std::vector<uint8_t> bytes_of(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

static void head(const char* title) { std::printf("\n--- %s\n", title); }

// ---------------------------------------------------------------------------
// 1. shm_ring: capacity, policies, every accessor, every counter.
// ---------------------------------------------------------------------------
static void teach_ring() {
    std::printf("========== shm_ring ==========\n");

    head("constructor: capacity is rounded up to a power of two, then clamped");
    for (size_t request : {8u, 1000u, 4096u}) {
        shm_ring r(request);
        std::printf("  asked %5zu -> capacity %5zu (min_capacity %zu)\n", request, r.capacity(),
                    shm_ring::min_capacity);
    }

    head("push: payload, type and client_id are all recorded; sequence auto-increments");
    shm_ring ring(512);
    std::printf("  capacity %zu header_size %zu magic 0x%08X\n", ring.capacity(),
                shm_ring::header_size, shm_ring::frame_magic);
    auto p1 = bytes_of("register");
    auto p2 = bytes_of("dom");
    ring.push(p1.data(), p1.size(), 0, 42);
    ring.push(p2.data(), p2.size(), 2, 42);
    std::printf("  frames %zu used %zu free %zu empty %d\n", ring.frames(), ring.used(),
                ring.free_space(), ring.empty() ? 1 : 0);

    head("peek_header / peek / pop: read without consuming, then consume");
    shm_ring::frame_header hdr;
    if (ring.peek_header(hdr)) {
        std::printf("  peek_header: magic 0x%08X len %u type %u client %u seq %u crc 0x%08X\n",
                    hdr.magic, hdr.length, hdr.type, hdr.client_id, hdr.sequence, hdr.crc);
    }
    std::printf("  front_frame_size %zu\n", ring.front_frame_size());
    shm_ring::frame peeked;
    if (ring.peek(peeked)) std::printf("  peek -> \"%s\" (frames still %zu)\n",
                                      text(peeked.payload).c_str(), ring.frames());
    shm_ring::frame popped;
    if (ring.pop(popped)) std::printf("  pop  -> \"%s\" seq %u (frames now %zu)\n",
                                      text(popped.payload).c_str(), popped.header.sequence,
                                      ring.frames());

    head("discard: drop the oldest frame without copying its payload");
    bool dropped = ring.discard();
    std::printf("  discard -> %d, frames %zu, frames_discarded %llu\n", dropped ? 1 : 0,
                ring.frames(),
                static_cast<unsigned long long>(ring.counters().frames_discarded));

    head("policy reject: a push that does not fit changes nothing");
    shm_ring reject_ring(256);
    reject_ring.set_policy(shm_ring::overflow_policy::reject);
    std::string block(100, 'R');
    std::printf("  push 100 bytes -> %d (used %zu)\n",
                reject_ring.push(block.data(), block.size()) ? 1 : 0, reject_ring.used());
    std::printf("  push 100 bytes -> %d (used %zu, push_rejects %llu)\n",
                reject_ring.push(block.data(), block.size()) ? 1 : 0, reject_ring.used(),
                static_cast<unsigned long long>(reject_ring.counters().push_rejects));

    head("policy drop_oldest: the oldest frames are evicted to make room");
    shm_ring drop_ring(256);
    drop_ring.set_policy(shm_ring::overflow_policy::drop_oldest);
    std::printf("  policy is %s\n",
                drop_ring.policy() == shm_ring::overflow_policy::drop_oldest ? "drop_oldest"
                                                                            : "reject");
    for (int i = 0; i < 4; ++i) {
        drop_ring.push(block.data(), block.size(), static_cast<uint16_t>(i));
    }
    std::printf("  after 4 pushes of 100 bytes: frames %zu discarded %llu used %zu\n",
                drop_ring.frames(),
                static_cast<unsigned long long>(drop_ring.counters().frames_discarded),
                drop_ring.used());
    shm_ring::frame survivor;
    while (drop_ring.pop(survivor)) {
        std::printf("  survivor type %u len %zu\n", survivor.header.type,
                    survivor.payload.size());
    }

    head("oversized payload: larger than capacity minus the header, always rejected");
    shm_ring tiny(128);
    std::vector<uint8_t> huge(500, 0);
    std::printf("  500 bytes into a 128 byte ring -> %d\n", tiny.push(huge.data(), huge.size()));

    head("zero length payload is legal and has a stable crc");
    shm_ring empty_ring(128);
    std::printf("  push(nullptr, 0) -> %d\n", empty_ring.push(nullptr, 0) ? 1 : 0);
    std::printf("  crc32 of nothing is 0x%08X\n", shm_ring::crc32(nullptr, 0));

    head("push_frame: supply the header yourself, length and crc are recomputed");
    shm_ring explicit_ring(256);
    shm_ring::frame_header custom;
    custom.sequence = 4242;
    custom.type = 7;
    custom.client_id = 9;
    custom.flags = 0x0002;
    auto body = bytes_of("custom");
    explicit_ring.push_frame(custom, body.data(), body.size());
    shm_ring::frame got;
    explicit_ring.pop(got);
    std::printf("  seq %u (kept) type %u client %u flags 0x%04X len %u\n", got.header.sequence,
                got.header.type, got.header.client_id, got.header.flags, got.header.length);

    head("crc32: the standard check vector");
    std::printf("  crc32(\"123456789\") = 0x%08X (expected 0xCBF43926)\n",
                shm_ring::crc32("123456789", 9));

    head("raw() and raw_index(): flip a byte and watch validation catch it");
    shm_ring victim(256);
    auto vp = bytes_of("guarded");
    victim.push(vp.data(), vp.size());
    victim.raw()[victim.raw_index(shm_ring::header_size)] ^= 0xFF;
    shm_ring::frame bad;
    std::printf("  pop after tampering -> %d, corrupt_frames %llu, frames %zu\n",
                victim.pop(bad) ? 1 : 0,
                static_cast<unsigned long long>(victim.counters().corrupt_frames), victim.frames());

    head("counters: every field after a clean run");
    shm_ring stats_ring(1024);
    auto sp = bytes_of("0123456789");
    for (int i = 0; i < 6; ++i) {
        stats_ring.push(sp.data(), sp.size(), 2, 1);
        stats_ring.pop(popped);
    }
    const auto& c = stats_ring.counters();
    std::printf("  frames_pushed %llu frames_popped %llu frames_discarded %llu\n",
                static_cast<unsigned long long>(c.frames_pushed),
                static_cast<unsigned long long>(c.frames_popped),
                static_cast<unsigned long long>(c.frames_discarded));
    std::printf("  bytes_in %llu bytes_out %llu push_rejects %llu corrupt %llu wraps %llu\n",
                static_cast<unsigned long long>(c.bytes_in),
                static_cast<unsigned long long>(c.bytes_out),
                static_cast<unsigned long long>(c.push_rejects),
                static_cast<unsigned long long>(c.corrupt_frames),
                static_cast<unsigned long long>(c.wraps));

    head("reset: clears bytes and counters, keeps capacity, restarts sequences at 1");
    stats_ring.reset();
    stats_ring.push(sp.data(), sp.size());
    stats_ring.pop(popped);
    std::printf("  after reset: capacity %zu frames_pushed %llu first seq %u\n",
                stats_ring.capacity(),
                static_cast<unsigned long long>(stats_ring.counters().frames_pushed),
                popped.header.sequence);
}

// ---------------------------------------------------------------------------
// 2. msg_frame: wire format, the streaming reader, and every validator knob.
// ---------------------------------------------------------------------------
static void teach_frame() {
    std::printf("\n========== msg_frame ==========\n");

    head("message types: the whole enum, and to_string");
    for (uint16_t i = 0; i < static_cast<uint16_t>(message_type::_count); ++i) {
        std::printf("  %u %s\n", i, to_string(static_cast<message_type>(i)));
    }

    head("frame errors: the whole enum, and to_string");
    for (uint8_t i = 0; i <= static_cast<uint8_t>(frame_error::truncated_payload); ++i) {
        std::printf("  %u %s\n", i, to_string(static_cast<frame_error>(i)));
    }

    head("encode_frame: 32 byte header, little endian, crc covers header plus payload");
    frame f;
    f.header.type = static_cast<uint16_t>(message_type::dom_update);
    f.header.client_id = 3;
    f.header.sequence = 1;
    f.header.timestamp_ns = 5000000000ull;
    f.payload = bytes_of("<window/>");
    auto wire = encode_frame(f);
    std::printf("  magic 0x%08X version %u header_size %zu total %zu\n", frame_magic,
                protocol_version, frame_header_size, wire.size());

    head("encode_frame into a caller buffer: too small returns 0");
    uint8_t small[8] = {0};
    std::printf("  8 byte buffer -> %zu bytes written\n", encode_frame(f, small, sizeof small));

    head("frame_reader: feed arbitrary chunks, drain complete frames");
    frame_reader reader;
    for (size_t i = 0; i < wire.size(); i += 5) {
        size_t n = std::min<size_t>(5, wire.size() - i);
        reader.feed(wire.data() + i, n);
    }
    frame out;
    if (reader.next(out)) {
        std::printf("  decoded type %s client %u seq %u payload \"%s\"\n", to_string(out.type()),
                    out.header.client_id, out.header.sequence, text(out.payload).c_str());
    }
    std::printf("  buffered %zu frames_decoded %zu bytes_consumed %llu error %s\n",
                reader.buffered(), reader.frames_decoded(),
                static_cast<unsigned long long>(reader.bytes_consumed()),
                to_string(reader.error()));

    head("frame_reader: a short feed reports short_header, not a frame");
    frame_reader short_reader;
    uint8_t one = 0x58;
    short_reader.feed(&one, 1);
    std::printf("  next -> %d error %s detail \"%s\"\n", short_reader.next(out) ? 1 : 0,
                to_string(short_reader.error()), short_reader.error_detail().c_str());
    short_reader.reset();
    std::printf("  after reset buffered %zu\n", short_reader.buffered());

    head("frame_reader: max_payload caps the declared size before any copy");
    frame fat;
    fat.header.type = static_cast<uint16_t>(message_type::ping);
    fat.header.client_id = 1;
    fat.header.sequence = 1;
    fat.header.timestamp_ns = 5000000000ull;
    fat.payload.assign(64, 'x');
    auto fat_wire = encode_frame(fat);
    frame_reader capped(16);
    capped.feed(fat_wire);
    std::printf("  payload 64 with max_payload 16 -> %d error %s\n", capped.next(out) ? 1 : 0,
                to_string(capped.error()));

    head("validator_config::strict(): the client to desktop whitelist");
    auto strict = validator_config::strict();
    std::printf("  max_payload %zu client ids %u..%u\n", strict.max_payload, strict.min_client_id,
                strict.max_client_id);
    std::printf("  max_clock_skew_ns %llu max_age_ns %llu require_crc %d monotonic %d\n",
                static_cast<unsigned long long>(strict.max_clock_skew_ns),
                static_cast<unsigned long long>(strict.max_age_ns), strict.require_crc ? 1 : 0,
                strict.require_monotonic_sequence ? 1 : 0);
    for (uint16_t i = 0; i < static_cast<uint16_t>(message_type::_count); ++i) {
        bool allowed = (strict.allowed_types & (1u << i)) != 0;
        std::printf("  type %-18s allowed %d\n", to_string(static_cast<message_type>(i)),
                    allowed ? 1 : 0);
    }

    head("validator_config::permissive(): every check relaxed");
    auto loose = validator_config::permissive();
    std::printf("  allow_all_types %d require_crc %d monotonic %d client ids %u..%u\n",
                loose.allow_all_types ? 1 : 0, loose.require_crc ? 1 : 0,
                loose.require_monotonic_sequence ? 1 : 0, loose.min_client_id, loose.max_client_id);

    const uint64_t now = 1000000000000ull;
    auto make = [](uint16_t type, uint32_t client, uint32_t seq, const std::string& body,
                   uint64_t ts) {
        frame g;
        g.header.type = type;
        g.header.client_id = client;
        g.header.sequence = seq;
        g.header.timestamp_ns = ts;
        g.payload = bytes_of(body);
        auto w = encode_frame(g);
        frame_reader r;
        r.feed(w);
        frame d;
        r.next(d);
        return d;
    };

    head("validator: one rejection per knob, each with its own error code");
    {
        validator v;
        auto good = make(static_cast<uint16_t>(message_type::dom_update), 5, 1, "ok", now);
        auto r = v.check(good, now);
        std::printf("  valid frame -> %s (accepted %llu rejected %llu)\n", to_string(r.error),
                    static_cast<unsigned long long>(v.accepted()),
                    static_cast<unsigned long long>(v.rejected()));

        auto bad_magic = good;
        bad_magic.header.magic = 0;
        auto bad_version = good;
        bad_version.header.version = 3;
        auto bad_type = good;
        bad_type.header.type = 200;
        auto wrong_way = make(static_cast<uint16_t>(message_type::event_dispatch), 5, 1, "", now);
        auto bad_client = make(static_cast<uint16_t>(message_type::ping), 0, 1, "", now);
        auto stale = make(static_cast<uint16_t>(message_type::ping), 6, 1, "", now - 120000000000ull);
        auto future = make(static_cast<uint16_t>(message_type::ping), 7, 1, "", now + 10000000000ull);
        auto torn = good;
        torn.payload.push_back('!');

        validator v2;
        std::printf("  magic 0            -> %s\n", to_string(v2.check(bad_magic, now).error));
        std::printf("  version 3          -> %s\n", to_string(v2.check(bad_version, now).error));
        std::printf("  type 200           -> %s\n", to_string(v2.check(bad_type, now).error));
        std::printf("  desktop-only type  -> %s\n", to_string(v2.check(wrong_way, now).error));
        std::printf("  client id 0        -> %s\n", to_string(v2.check(bad_client, now).error));
        std::printf("  120 s old          -> %s\n", to_string(v2.check(stale, now).error));
        std::printf("  10 s in the future -> %s\n", to_string(v2.check(future, now).error));
        std::printf("  payload size lied  -> %s\n", to_string(v2.check(torn, now).error));

        validator v3;
        auto s1 = make(static_cast<uint16_t>(message_type::ping), 8, 5, "", now);
        auto s2 = make(static_cast<uint16_t>(message_type::ping), 8, 5, "", now);
        auto s3 = make(static_cast<uint16_t>(message_type::ping), 8, 9, "", now);
        std::printf("  seq 5 then 5       -> %s\n", to_string(v3.check(s1, now).error));
        std::printf("  same again         -> %s\n", to_string(v3.check(s2, now).error));
        v3.forget_client(8);
        std::printf("  after forget_client-> %s\n", to_string(v3.check(s2, now).error));
        std::printf("  seq 9              -> %s\n", to_string(v3.check(s3, now).error));
        v3.reset();
        std::printf("  after reset, seq 5 -> %s (accepted %llu rejected %llu)\n",
                    to_string(v3.check(s1, now).error),
                    static_cast<unsigned long long>(v3.accepted()),
                    static_cast<unsigned long long>(v3.rejected()));
    }

    head("validator knobs: max_payload, client id range, skew and age windows");
    {
        validator_config cfg = validator_config::strict();
        cfg.max_payload = 4;
        cfg.max_client_id = 10;
        cfg.min_client_id = 2;
        cfg.max_age_ns = 1000000ull;      // 1 ms
        cfg.max_clock_skew_ns = 1000000ull;
        validator v(cfg);
        std::printf("  config max_payload %zu\n", v.config().max_payload);
        auto big = make(static_cast<uint16_t>(message_type::ping), 5, 1, "0123456789", now);
        auto too_low = make(static_cast<uint16_t>(message_type::ping), 1, 1, "", now);
        auto too_high = make(static_cast<uint16_t>(message_type::ping), 11, 1, "", now);
        auto slightly_old = make(static_cast<uint16_t>(message_type::ping), 5, 1, "", now - 5000000ull);
        std::printf("  payload 10 vs 4    -> %s\n", to_string(v.check(big, now).error));
        std::printf("  client 1 vs min 2  -> %s\n", to_string(v.check(too_low, now).error));
        std::printf("  client 11 vs max 10-> %s\n", to_string(v.check(too_high, now).error));
        std::printf("  5 ms old vs 1 ms   -> %s\n", to_string(v.check(slightly_old, now).error));
    }

    head("knobs: require_crc and require_monotonic_sequence can be switched off");
    {
        validator_config cfg = validator_config::strict();
        cfg.require_crc = false;
        cfg.require_monotonic_sequence = false;
        validator v(cfg);
        frame zero_crc = make(static_cast<uint16_t>(message_type::ping), 4, 1, "abc", now);
        zero_crc.header.crc = 0;
        auto r = v.check(zero_crc, now);
        std::printf("  crc 0 with require_crc off -> %s\n", to_string(r.error));
        auto a = make(static_cast<uint16_t>(message_type::ping), 4, 3, "", now);
        auto b = make(static_cast<uint16_t>(message_type::ping), 4, 3, "", now);
        v.check(a, now);
        std::printf("  repeated seq with monotonic off -> %s\n", to_string(v.check(b, now).error));
    }

    head("knobs: allowed_types bitmask and allow_all_types");
    {
        validator_config cfg = validator_config::strict();
        cfg.allowed_types = 1u << static_cast<uint32_t>(message_type::ping);
        validator only_ping(cfg);
        auto dom = make(static_cast<uint16_t>(message_type::dom_update), 1, 1, "", now);
        auto ping = make(static_cast<uint16_t>(message_type::ping), 1, 1, "", now);
        std::printf("  dom_update vs ping-only mask -> %s\n", to_string(only_ping.check(dom, now).error));
        std::printf("  ping vs ping-only mask       -> %s\n", to_string(only_ping.check(ping, now).error));

        validator_config all = validator_config::strict();
        all.allow_all_types = true;
        validator any(all);
        auto dispatch = make(static_cast<uint16_t>(message_type::event_dispatch), 1, 1, "", now);
        std::printf("  desktop type with allow_all -> %s\n", to_string(any.check(dispatch, now).error));
    }

    head("check_bytes: decode and validate in one call");
    {
        validator v;
        frame g = make(static_cast<uint16_t>(message_type::buffer_update), 2, 1, "pixels", now);
        auto w = encode_frame(g);
        frame decoded;
        auto r = v.check_bytes(w.data(), w.size(), now, &decoded);
        std::printf("  -> %s payload \"%s\"\n", to_string(r.error), text(decoded.payload).c_str());
        uint8_t junk[4] = {1, 2, 3, 4};
        std::printf("  4 junk bytes -> %s\n", to_string(v.check_bytes(junk, sizeof junk, now).error));
    }

    head("validator::crc32 matches the ring crc32");
    std::printf("  validator::crc32(\"123456789\") = 0x%08X\n", validator::crc32("123456789", 9));
}

// ---------------------------------------------------------------------------
// 3. request_dispatch: ids, routes, filters, sync and async calls, timeouts.
// ---------------------------------------------------------------------------
static void teach_dispatch() {
    std::printf("\n========== request_dispatch ==========\n");

    head("request ids pack client id in the high 32 bits and sequence in the low");
    request_id id = make_request_id(5, 17);
    std::printf("  make_request_id(5, 17) = %llu -> client %u seq %u\n",
                static_cast<unsigned long long>(id), request_client(id), request_sequence(id));

    head("dispatch_status: the whole enum");
    for (uint8_t i = 0; i <= static_cast<uint8_t>(dispatch_status::handler_exception); ++i) {
        std::printf("  %u %s\n", i, to_string(static_cast<dispatch_status>(i)));
    }

    dispatcher desk;
    auto ping_type = static_cast<uint16_t>(message_type::ping);
    auto dom_type = static_cast<uint16_t>(message_type::dom_update);
    auto buf_type = static_cast<uint16_t>(message_type::buffer_update);

    head("register_route: one handler per type, with a readable name");
    desk.register_route(ping_type, [](const request& r) {
        response resp;
        resp.status = dispatch_status::ok;
        resp.payload = bytes_of("pong:" + std::to_string(r.sequence));
        return resp;
    }, "ping");
    desk.register_route(dom_type, [](const request& r) {
        response resp;
        resp.status = dispatch_status::ok;
        resp.payload = bytes_of("dom " + std::to_string(r.payload.size()));
        return resp;
    }, "dom_update");
    for (uint16_t t : desk.routes()) std::printf("  route %u registered\n", t);
    std::printf("  has_route(%u) %d has_route(200) %d\n", ping_type,
                desk.has_route(ping_type) ? 1 : 0, desk.has_route(200) ? 1 : 0);

    head("strict mode: an unknown route is reported as unknown_route");
    loopback_channel channel(desk);
    client_stub client = channel.make_client(5);
    auto missing = client.call(static_cast<uint16_t>(message_type::event_dispatch), {});
    std::printf("  strict %d -> status %s message \"%s\"\n", desk.strict() ? 1 : 0,
                to_string(missing.status), missing.message.c_str());

    desk.set_strict(false);
    auto swallowed = client.call(static_cast<uint16_t>(message_type::event_dispatch), {});
    std::printf("  strict %d -> status %s message \"%s\"\n", desk.strict() ? 1 : 0,
                to_string(swallowed.status), swallowed.message.c_str());
    desk.set_strict(true);

    head("set_filter: inspect or veto every request before it reaches a handler");
    desk.set_filter([](const request& r, response& out) {
        if (r.payload.size() > 8) {
            out.message = "payload " + std::to_string(r.payload.size()) + " bytes is too big";
            return false;
        }
        return true;
    });
    auto refused = client.call(dom_type, bytes_of("0123456789ABCDEF"));
    std::printf("  16 byte payload -> status %s message \"%s\"\n", to_string(refused.status),
                refused.message.c_str());
    desk.set_filter(nullptr);

    head("client_stub::call: synchronous, id and elapsed_us come back filled in");
    auto sync = client.call(ping_type, {});
    std::printf("  id %llu (client %u seq %u) status %s payload \"%s\"\n",
                static_cast<unsigned long long>(sync.id), request_client(sync.id),
                request_sequence(sync.id), to_string(sync.status), text(sync.payload).c_str());
    std::printf("  ok() %d elapsed_us %llu\n", sync.ok() ? 1 : 0,
                static_cast<unsigned long long>(sync.elapsed_us));

    head("call with a timeout: a handler slower than the budget reports timed_out");
    desk.register_route(buf_type, [](const request& r) {
        (void)r;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        response resp;
        resp.status = dispatch_status::ok;
        return resp;
    }, "slow");
    auto late = client.call(buf_type, {}, std::chrono::milliseconds(1));
    std::printf("  20 ms handler, 1 ms budget -> status %s message \"%s\"\n",
                to_string(late.status), late.message.c_str());

    head("call_async and wait: the id is returned immediately, the answer is collected later");
    request_id aid = client.call_async(dom_type, bytes_of("<doc/>"));
    std::printf("  id %llu pending %d pending_count %zu\n",
                static_cast<unsigned long long>(aid), client.pending(aid) ? 1 : 0,
                client.pending_count());
    response aresp;
    bool found = client.wait(aid, aresp);
    std::printf("  wait -> %d status %s payload \"%s\" pending_count %zu\n", found ? 1 : 0,
                to_string(aresp.status), text(aresp.payload).c_str(), client.pending_count());
    response never;
    std::printf("  wait on an unknown id -> %d\n", client.wait(999999, never) ? 1 : 0);

    head("cancel: withdraw a request that has not been answered");
    dispatcher slow_desk;
    slow_desk.register_route(buf_type, [](const request&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        response resp;
        resp.status = dispatch_status::ok;
        return resp;
    }, "slow");
    loopback_channel slow_channel(slow_desk);
    client_stub slow_client = slow_channel.make_client(6);
    request_id cid = slow_client.call_async(buf_type, {});
    std::printf("  cancel(answered id) -> %d\n", slow_client.cancel(cid) ? 1 : 0);
    std::printf("  cancel(unknown id)  -> %d\n", slow_client.cancel(424242) ? 1 : 0);

    head("unregister_route: removing a route turns it back into an unknown route");
    std::printf("  unregister_route(%u) -> %d, routes left %zu\n", dom_type,
                desk.unregister_route(dom_type) ? 1 : 0, desk.routes().size());
    std::printf("  unregister_route(%u) again -> %d\n", dom_type,
                desk.unregister_route(dom_type) ? 1 : 0);

    head("handler exceptions are caught and reported, never propagated");
    desk.register_route(dom_type, [](const request&) -> response {
        throw std::runtime_error("client sent a malformed document");
    }, "throwing");
    auto threw = client.call(dom_type, {});
    std::printf("  status %s message \"%s\"\n", to_string(threw.status), threw.message.c_str());

    head("per route stats and totals");
    const auto& rs = desk.stats(ping_type);
    std::printf("  ping: calls %llu failures %llu total_us %llu max_us %llu\n",
                static_cast<unsigned long long>(rs.calls),
                static_cast<unsigned long long>(rs.failures),
                static_cast<unsigned long long>(rs.total_us),
                static_cast<unsigned long long>(rs.max_us));
    std::printf("  total_calls %llu total_failures %llu loopback requests %llu\n",
                static_cast<unsigned long long>(desk.total_calls()),
                static_cast<unsigned long long>(desk.total_failures()),
                static_cast<unsigned long long>(channel.requests()));

    head("client stub counters");
    const auto& cs = client.counters();
    std::printf("  client_id %u next_sequence %u\n", client.client_id(), client.next_sequence());
    std::printf("  requests_sent %llu responses_ok %llu timeouts %llu cancelled %llu bytes_sent %llu\n",
                static_cast<unsigned long long>(cs.requests_sent),
                static_cast<unsigned long long>(cs.responses_ok),
                static_cast<unsigned long long>(cs.timeouts),
                static_cast<unsigned long long>(cs.cancelled),
                static_cast<unsigned long long>(cs.bytes_sent));

    head("a stub with no transport fails cleanly instead of crashing");
    client_stub orphan(7, nullptr);
    auto dead = orphan.call(ping_type, {});
    std::printf("  status %s message \"%s\"\n", to_string(dead.status), dead.message.c_str());
}

// ---------------------------------------------------------------------------
// 4. blit_surface: geometry, four copy modes, three blend modes, clipping.
// ---------------------------------------------------------------------------
static void teach_blit() {
    std::printf("\n========== blit_surface ==========\n");

    head("color32 packs 0xAARRGGBB and unpacks cleanly");
    color32 c = color32::rgba(10, 20, 30, 40);
    std::printf("  0x%08X -> a %u r %u g %u b %u\n", c.v, c.a(), c.r(), c.g(), c.b());

    head("rect: intersect, unite, contains, from_size");
    rect a = rect::from_size(10, 10);
    rect b{5, 5, 10, 10};
    rect i = rect::intersect(a, b);
    rect u = rect::unite(a, b);
    std::printf("  a %d,%d %dx%d  b %d,%d %dx%d\n", a.x, a.y, a.w, a.h, b.x, b.y, b.w, b.h);
    std::printf("  intersect -> %d,%d %dx%d  right %d bottom %d\n", i.x, i.y, i.w, i.h, i.right(),
                i.bottom());
    std::printf("  unite     -> %d,%d %dx%d\n", u.x, u.y, u.w, u.h);
    std::printf("  contains(7,7) %d contains(20,20) %d empty(disjoint) %d\n",
                i.contains(7, 7) ? 1 : 0, i.contains(20, 20) ? 1 : 0,
                rect::intersect(rect{0, 0, 2, 2}, rect{5, 5, 2, 2}).empty() ? 1 : 0);

    head("surface: size, pixel count, byte count, row pointer, at/set/fill/fill_rect");
    surface s(8, 4, color32::rgba(0, 0, 0, 255));
    std::printf("  %dx%d pixels %zu bytes %zu empty %d\n", s.width(), s.height(), s.pixels(),
                s.bytes(), s.empty() ? 1 : 0);
    s.set(3, 2, color32::rgba(255, 0, 0));
    std::printf("  at(3,2) = 0x%08X, at(-1,0) = 0x%08X (out of range reads zero)\n", s.at(3, 2).v,
                s.at(-1, 0).v);
    std::printf("  row(2)[3] = 0x%08X\n", s.row(2)[3]);
    s.fill_rect(rect{0, 0, 4, 2}, color32::rgba(0, 255, 0));
    std::printf("  after fill_rect: at(0,0) 0x%08X at(3,2) 0x%08X\n", s.at(0, 0).v, s.at(3, 2).v);
    s.fill(color32::rgba(9, 9, 9));
    std::printf("  after fill: at(7,3) 0x%08X checksum %016llX\n", s.at(7, 3).v,
                static_cast<unsigned long long>(s.checksum()));
    s.resize(4, 2, color32::rgba(1, 1, 1));
    std::printf("  after resize 4x2: pixels %zu at(0,0) 0x%08X\n", s.pixels(), s.at(0, 0).v);
    surface zero(0, 0);
    std::printf("  surface(0,0) empty %d bounds %dx%d\n", zero.empty() ? 1 : 0, zero.bounds().w,
                zero.bounds().h);

    head("diff: count differing pixels, optionally inside a clip");
    surface left(8, 8, color32::rgba(0, 0, 0));
    surface right(8, 8, color32::rgba(0, 0, 0));
    right.fill_rect(rect{0, 0, 4, 8}, color32::rgba(255, 255, 255));
    rect half{0, 0, 4, 8};
    std::printf("  full diff %zu, clipped to the changed half %zu\n", left.diff(right),
                left.diff(right, &half));

    head("copy_mode::copy: raw replacement, source alpha ignored");
    surface dst(6, 4, color32::rgba(20, 20, 20, 255));
    surface src(3, 2, color32::rgba(200, 100, 50, 0)); // alpha zero on purpose
    blit_options opt;
    opt.mode = copy_mode::copy;
    auto st = blit(dst, src, 1, 1, opt);
    std::printf("  mode %s pixels_in %llu written %llu clipped %llu skipped %llu bytes %llu\n",
                to_string(opt.mode), static_cast<unsigned long long>(st.pixels_in),
                static_cast<unsigned long long>(st.pixels_written),
                static_cast<unsigned long long>(st.pixels_clipped),
                static_cast<unsigned long long>(st.pixels_skipped),
                static_cast<unsigned long long>(st.bytes_moved));
    std::printf("  dst(1,1) = 0x%08X (alpha 0 was copied anyway)\n", dst.at(1, 1).v);

    head("copy_mode::over: source alpha blends against the destination");
    surface dst2(6, 4, color32::rgba(0, 0, 0, 255));
    surface half_src(3, 2, color32::rgba(255, 255, 255, 128));
    opt.mode = copy_mode::over;
    blit(dst2, half_src, 0, 0, opt);
    std::printf("  50%% white over black = 0x%08X (r %u)\n", dst2.at(0, 0).v, dst2.at(0, 0).r());

    head("blit_options::global_alpha: an extra multiplier on top of source alpha");
    for (uint8_t ga : {255, 128, 64}) {
        surface d(2, 2, color32::rgba(0, 0, 0, 255));
        surface src_opaque(2, 2, color32::rgba(255, 255, 255, 255));
        blit_options o;
        o.mode = copy_mode::over;
        o.global_alpha = ga;
        blit(d, src_opaque, 0, 0, o);
        std::printf("  global_alpha %3u -> white over black = %u\n", ga, d.at(0, 0).r());
    }

    head("copy_mode::add: channels add and saturate at 255");
    surface add_dst(2, 2, color32::rgba(200, 10, 0, 255));
    surface add_src(2, 2, color32::rgba(100, 20, 0, 255));
    blit_options add_opt;
    add_opt.mode = copy_mode::add;
    blit(add_dst, add_src, 0, 0, add_opt);
    color32 added = add_dst.at(0, 0);
    std::printf("  200+100 saturates to %u, 10+20 = %u\n", added.r(), added.g());

    head("copy_mode::color_key: pixels equal to the key are skipped");
    surface key_dst(4, 4, color32::rgba(0, 0, 255, 255));
    surface key_src(4, 4, color32::rgba(255, 0, 255, 255));
    key_src.fill_rect(rect{0, 0, 2, 4}, color32::rgba(0, 0, 0, 255));
    blit_options key_opt;
    key_opt.mode = copy_mode::color_key;
    key_opt.key = color32::rgba(255, 0, 255, 255);
    auto kst = blit(key_dst, key_src, 0, 0, key_opt);
    std::printf("  written %llu skipped %llu, dst(0,0) 0x%08X dst(3,0) 0x%08X\n",
                static_cast<unsigned long long>(kst.pixels_written),
                static_cast<unsigned long long>(kst.pixels_skipped), key_dst.at(0, 0).v,
                key_dst.at(3, 0).v);

    head("blit_options::flip_x and flip_y mirror the source");
    surface flat(2, 2, color32::rgba(0, 0, 0));
    flat.set(0, 0, color32::rgba(255, 0, 0));
    surface mirror(2, 2, color32::rgba(0, 0, 0));
    blit_options flip_opt;
    flip_opt.flip_x = true;
    flip_opt.flip_y = true;
    blit(mirror, flat, 0, 0, flip_opt);
    std::printf("  source (0,0) lands at (1,1): 0x%08X\n", mirror.at(1, 1).v);

    head("blit_options::clip: the destination is restricted to a rectangle");
    surface clipped(10, 10, color32::rgba(0, 0, 0, 255));
    surface patch(10, 10, color32::rgba(255, 255, 255, 255));
    blit_options clip_opt;
    clip_opt.clip = rect{2, 2, 3, 3};
    auto cst = blit(clipped, patch, 0, 0, clip_opt);
    std::printf("  pixels_in %llu written %llu clipped %llu, dst(0,0) 0x%08X dst(3,3) 0x%08X\n",
                static_cast<unsigned long long>(cst.pixels_in),
                static_cast<unsigned long long>(cst.pixels_written),
                static_cast<unsigned long long>(cst.pixels_clipped), clipped.at(0, 0).v,
                clipped.at(3, 3).v);

    head("blit_rect: copy only a sub rectangle of the source");
    surface dst3(8, 4, color32::rgba(0, 0, 0, 255));
    surface grid(4, 4, color32::rgba(255, 255, 255, 255));
    auto rst = blit_rect(dst3, grid, rect{1, 1, 2, 2}, 4, 0);
    std::printf("  source rect 2x2 at (4,0): written %llu, dst(4,0) 0x%08X dst(0,0) 0x%08X\n",
                static_cast<unsigned long long>(rst.pixels_written), dst3.at(4, 0).v,
                dst3.at(0, 0).v);
    surface dst4(8, 4, color32::rgba(0, 0, 0, 255));
    auto ost = blit(dst4, grid, 100, 100);
    std::printf("  blit entirely off screen: written %llu clipped %llu\n",
                static_cast<unsigned long long>(ost.pixels_written),
                static_cast<unsigned long long>(ost.pixels_clipped));

    head("blend_over: the single pixel primitive, exposed for teaching");
    color32 base = color32::rgba(0, 0, 0, 255);
    color32 over_half = blend_over(base, color32::rgba(255, 255, 255, 128));
    color32 over_transparent = blend_over(base, color32::rgba(255, 255, 255, 0));
    color32 over_quarter = blend_over(base, color32::rgba(255, 255, 255, 255), 64);
    std::printf("  50%% white over black -> %u\n", over_half.r());
    std::printf("  alpha 0 -> unchanged %u\n", over_transparent.r());
    std::printf("  opaque white at global_alpha 64 -> %u\n", over_quarter.r());

    head("composite: three sampling modes, same 16x16 source into a 7x7 box");
    for (blend_mode m : {blend_mode::nearest, blend_mode::bilinear, blend_mode::bilinear_alpha}) {
        surface big(16, 16, color32::rgba(0, 0, 0));
        // A hard red/blue edge makes the difference between modes visible.
        big.fill_rect(rect{0, 0, 8, 16}, color32::rgba(255, 0, 0, 255));
        big.fill_rect(rect{8, 0, 8, 16}, color32::rgba(0, 0, 255, 128));
        surface target(7, 7, color32::rgba(0, 0, 0, 255));
        auto cs = composite(target, big, rect{0, 0, 7, 7}, m);
        std::printf("  %-15s written %llu centre 0x%08X checksum %016llX\n", to_string(m),
                    static_cast<unsigned long long>(cs.pixels_written), target.at(3, 3).v,
                    static_cast<unsigned long long>(target.checksum()));
    }

    head("composite with a clip rectangle");
    surface big2(16, 16, color32::rgba(120, 120, 120, 255));
    surface target2(8, 8, color32::rgba(0, 0, 0, 255));
    rect comp_clip{0, 0, 4, 8};
    auto ccs = composite(target2, big2, rect{0, 0, 8, 8}, blend_mode::nearest, &comp_clip);
    std::printf("  written %llu (half the box), dst(0,0) 0x%08X dst(7,7) 0x%08X\n",
                static_cast<unsigned long long>(ccs.pixels_written), target2.at(0, 0).v,
                target2.at(7, 7).v);
    surface empty_target(0, 0);
    auto zst = composite(empty_target, big2, rect{0, 0, 4, 4}, blend_mode::nearest);
    std::printf("  empty destination -> written %llu\n",
                static_cast<unsigned long long>(zst.pixels_written));
}

int main() {
    std::printf("CPPUX channel teaching program\n");
    std::printf("Every option and parameter of shm_ring, msg_frame, request_dispatch and\n");
    std::printf("blit_surface, one block at a time. Numbers are deterministic; the only\n");
    std::printf("timing dependent lines are the timeout examples, which report a status.\n");
    teach_ring();
    teach_frame();
    teach_dispatch();
    teach_blit();
    std::printf("\nteaching program complete\n");
    return 0;
}

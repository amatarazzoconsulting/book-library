// MIT License - (c) Anthony Matarazzo 2026
//
// demo.cpp - one short pass over the four pieces of the CPPUX channel.

#include "blit_surface.hpp"
#include "msg_frame.hpp"
#include "request_dispatch.hpp"
#include "shm_ring.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace cppux;

static std::string text(const std::vector<uint8_t>& v) {
    return std::string(v.begin(), v.end());
}

static std::vector<uint8_t> bytes_of(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

int main() {
    std::printf("== 1. shared memory ring\n");
    shm_ring ring(4096);
    const char* names[] = {"register_client", "dom_update", "ping"};
    for (uint32_t i = 0; i < 3; ++i) {
        ring.push(names[i], std::string(names[i]).size(), static_cast<uint16_t>(i), 7);
    }
    std::printf("pushed %zu frames, used %zu of %zu bytes\n", ring.frames(), ring.used(),
                ring.capacity());

    shm_ring::frame f;
    while (ring.pop(f)) {
        std::printf("  seq %u type %u client %u payload \"%s\"\n", f.header.sequence,
                    f.header.type, f.header.client_id, text(f.payload).c_str());
    }
    std::printf("  pushed %llu popped %llu bytes_in %llu wraps %llu\n",
                static_cast<unsigned long long>(ring.counters().frames_pushed),
                static_cast<unsigned long long>(ring.counters().frames_popped),
                static_cast<unsigned long long>(ring.counters().bytes_in),
                static_cast<unsigned long long>(ring.counters().wraps));

    std::printf("== 2. message framing and validation\n");
    frame msg;
    msg.header.type = static_cast<uint16_t>(message_type::dom_update);
    msg.header.client_id = 3;
    msg.header.sequence = 1;
    msg.header.timestamp_ns = 1000000000ull;
    msg.payload = bytes_of("<window id=1/>");

    auto wire = encode_frame(msg);
    std::printf("encoded %zu bytes (header %zu)\n", wire.size(), frame_header_size);

    frame_reader reader;
    for (size_t i = 0; i < wire.size(); i += 7) {
        size_t n = std::min<size_t>(7, wire.size() - i);
        reader.feed(wire.data() + i, n);
    }
    frame decoded;
    std::printf("streaming decode: %s\n", reader.next(decoded) ? "ok" : "failed");
    std::printf("  type %s client %u seq %u payload \"%s\"\n",
                to_string(decoded.type()), decoded.header.client_id, decoded.header.sequence,
                text(decoded.payload).c_str());

    validator v;
    auto good = v.check(decoded, 1000000000ull);
    std::printf("validator accepts: %s\n", good.ok() ? "yes" : "no");

    auto broken = wire;
    broken[frame_header_size] ^= 0xFF;
    frame_reader bad_reader;
    bad_reader.feed(broken);
    frame ignored;
    bool ok = bad_reader.next(ignored);
    std::printf("tampered byte: decoded=%s error=%s\n", ok ? "yes" : "no",
                to_string(bad_reader.error()));

    std::printf("== 3. request dispatch\n");
    dispatcher desk;
    desk.register_route(static_cast<uint16_t>(message_type::ping),
                        [](const request& r) {
                            response resp;
                            resp.status = dispatch_status::ok;
                            resp.payload = bytes_of("pong:" + std::to_string(r.sequence));
                            return resp;
                        },
                        "ping");
    desk.register_route(static_cast<uint16_t>(message_type::dom_update),
                        [](const request& r) {
                            response resp;
                            resp.status = dispatch_status::ok;
                            resp.payload = bytes_of("applied " + std::to_string(r.payload.size()) +
                                                    " bytes");
                            return resp;
                        },
                        "dom_update");

    loopback_channel channel(desk);
    client_stub client = channel.make_client(5);

    auto sync = client.call(static_cast<uint16_t>(message_type::ping), {});
    std::printf("sync call id %llu status %s payload \"%s\"\n",
                static_cast<unsigned long long>(sync.id), to_string(sync.status),
                text(sync.payload).c_str());

    request_id async_id =
        client.call_async(static_cast<uint16_t>(message_type::dom_update), bytes_of("<doc/>"));
    response async_resp;
    client.wait(async_id, async_resp);
    std::printf("async call id %llu status %s payload \"%s\"\n",
                static_cast<unsigned long long>(async_id), to_string(async_resp.status),
                text(async_resp.payload).c_str());

    auto missing = client.call(static_cast<uint16_t>(message_type::event_dispatch), {});
    std::printf("unrouted type %u: status %s\n", static_cast<unsigned>(message_type::event_dispatch),
                to_string(missing.status));
    std::printf("dispatcher served %llu calls, %llu failures\n",
                static_cast<unsigned long long>(desk.total_calls()),
                static_cast<unsigned long long>(desk.total_failures()));

    std::printf("== 4. surface blit\n");
    surface screen(64, 48, color32::rgba(16, 16, 24));
    surface window(32, 24, color32::rgba(0, 0, 0, 0));
    window.fill_rect(rect{0, 0, 32, 24}, color32::rgba(40, 90, 200, 255));
    window.fill_rect(rect{8, 6, 16, 12}, color32::rgba(240, 220, 60, 255));

    blit_options opt;
    opt.mode = copy_mode::over;
    auto st = blit(screen, window, 10, 8, opt);
    std::printf("blit over: in %llu written %llu skipped %llu bytes %llu\n",
                static_cast<unsigned long long>(st.pixels_in),
                static_cast<unsigned long long>(st.pixels_written),
                static_cast<unsigned long long>(st.pixels_skipped),
                static_cast<unsigned long long>(st.bytes_moved));

    surface icon(16, 16, color32::rgba(220, 60, 90, 255));
    auto scaled = composite(screen, icon, rect{44, 4, 18, 18}, blend_mode::bilinear);
    std::printf("composite bilinear: in %llu written %llu\n",
                static_cast<unsigned long long>(scaled.pixels_in),
                static_cast<unsigned long long>(scaled.pixels_written));

    std::printf("screen %dx%d checksum %016llx\n", screen.width(), screen.height(),
                static_cast<unsigned long long>(screen.checksum()));
    std::printf("demo complete\n");
    return 0;
}

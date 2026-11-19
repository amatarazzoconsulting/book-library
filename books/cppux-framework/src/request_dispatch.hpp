// MIT License - (c) Anthony Matarazzo 2026
//
// request_dispatch.hpp - Client stub to desktop dispatcher with request ids.
//
// In the book the client library (src/client/lib.cpp) sends a message_header and
// the desktop manager routes it to a handler. This is that routing table made
// explicit and testable:
//
//   * the stub allocates a monotonic request id per call (client_id << 20 | seq)
//   * the dispatcher owns one handler per message type
//   * synchronous calls block on a promise until the desktop answers
//   * asynchronous calls return the request id immediately
//   * requests can time out, be cancelled, and are tracked for metrics
//
// The transport is a pluggable callable, so the same code runs over a loopback
// in-process channel, a socket, or the shared memory ring.

#ifndef CPPUX_REQUEST_DISPATCH_HPP
#define CPPUX_REQUEST_DISPATCH_HPP

#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace cppux {

using request_id = uint64_t;

inline request_id make_request_id(uint32_t client_id, uint32_t sequence) {
    return (static_cast<uint64_t>(client_id) << 32) | sequence;
}
inline uint32_t request_client(request_id id) { return static_cast<uint32_t>(id >> 32); }
inline uint32_t request_sequence(request_id id) { return static_cast<uint32_t>(id & 0xFFFFFFFFu); }

enum class dispatch_status : uint8_t {
    ok = 0,
    unknown_route,
    rejected,
    timed_out,
    cancelled,
    transport_error,
    handler_exception
};

const char* to_string(dispatch_status s);

struct request {
    request_id id = 0;
    uint16_t type = 0;
    uint32_t client_id = 0;
    uint32_t sequence = 0;
    std::vector<uint8_t> payload;
    std::chrono::steady_clock::time_point created;
};

struct response {
    request_id id = 0;
    dispatch_status status = dispatch_status::ok;
    std::vector<uint8_t> payload;
    std::string message;
    uint64_t elapsed_us = 0;

    bool ok() const { return status == dispatch_status::ok; }
};

// Transport abstraction: the desktop side handler plus an optional hook that
// sees every request before routing (useful for logging and access checks).
class dispatcher {
public:
    using handler_fn = std::function<response(const request&)>;
    using filter_fn = std::function<bool(const request&, response&)>;

    struct route_stats {
        uint64_t calls = 0;
        uint64_t failures = 0;
        uint64_t total_us = 0;
        uint64_t max_us = 0;
    };

    void register_route(uint16_t type, handler_fn fn, std::string name = {});
    bool unregister_route(uint16_t type);
    bool has_route(uint16_t type) const;
    std::vector<uint16_t> routes() const;

    void set_filter(filter_fn f) { filter_ = std::move(f); }
    // Strict (the default) reports an unknown route as unknown_route so a
    // misbehaving client hears about it. Non strict swallows the message with
    // an ok response, which is what a desktop does with a stale message type
    // it no longer implements.
    void set_strict(bool s) { strict_ = s; }
    bool strict() const { return strict_; }

    response dispatch(const request& req);

    const route_stats& stats(uint16_t type) const;
    uint64_t total_calls() const;
    uint64_t total_failures() const;

private:
    struct route {
        handler_fn fn;
        std::string name;
        route_stats stats;
    };
    mutable std::mutex mutex_;
    std::map<uint16_t, route> routes_;
    filter_fn filter_;
    bool strict_ = true;
    // Requests that arrived for a type nobody registered. Kept apart from the
    // per route table because there is no route to attribute them to.
    uint64_t unrouted_ = 0;
};

// The client side stub. One instance per client.
class client_stub {
public:
    using transport_fn = std::function<response(const request&)>;

    client_stub(uint32_t client_id, transport_fn transport);

    // Synchronous: sends, waits up to timeout, returns the response.
    response call(uint16_t type, const std::vector<uint8_t>& payload,
                  std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));

    // Asynchronous: returns the request id at once; collect later with wait().
    request_id call_async(uint16_t type, const std::vector<uint8_t>& payload);

    // Wait for an outstanding async request. Returns false if the id is unknown.
    bool wait(request_id id, response& out,
              std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));

    bool cancel(request_id id);
    bool pending(request_id id) const;
    size_t pending_count() const;

    uint32_t client_id() const { return client_id_; }
    uint32_t next_sequence() const { return next_sequence_; }

    struct stub_stats {
        uint64_t requests_sent = 0;
        uint64_t responses_ok = 0;
        uint64_t timeouts = 0;
        uint64_t cancelled = 0;
        uint64_t bytes_sent = 0;
    };
    const stub_stats& counters() const { return stats_; }

private:
    struct slot {
        std::shared_ptr<std::promise<response>> promise;
        std::shared_future<response> future;
        std::chrono::steady_clock::time_point deadline;
        bool cancelled = false;
        bool delivered = false; // the transport already put a value in the promise
    };

    request_id allocate(uint16_t type, const std::vector<uint8_t>& payload, request& out);

    uint32_t client_id_;
    transport_fn transport_;
    mutable std::mutex mutex_;
    std::unordered_map<request_id, slot> pending_;
    uint32_t next_sequence_ = 1;
    stub_stats stats_;
};

// Loopback wiring used by demo, teaching and benchmarks: a stub whose transport
// feeds a dispatcher synchronously, the way a local socket pair would.
class loopback_channel {
public:
    explicit loopback_channel(dispatcher& d) : dispatcher_(d) {}

    client_stub make_client(uint32_t client_id) {
        return client_stub(client_id, [this](const request& r) {
            ++requests_;
            return dispatcher_.dispatch(r);
        });
    }

    uint64_t requests() const { return requests_; }

private:
    dispatcher& dispatcher_;
    uint64_t requests_ = 0;
};

} // namespace cppux

#endif // CPPUX_REQUEST_DISPATCH_HPP

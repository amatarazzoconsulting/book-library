// MIT License - (c) Anthony Matarazzo 2026
//
// request_dispatch.cpp - dispatcher and client stub implementation.

#include "request_dispatch.hpp"

#include <algorithm>
#include <exception>

namespace cppux {

const char* to_string(dispatch_status s) {
    switch (s) {
        case dispatch_status::ok: return "ok";
        case dispatch_status::unknown_route: return "unknown_route";
        case dispatch_status::rejected: return "rejected";
        case dispatch_status::timed_out: return "timed_out";
        case dispatch_status::cancelled: return "cancelled";
        case dispatch_status::transport_error: return "transport_error";
        case dispatch_status::handler_exception: return "handler_exception";
    }
    return "unknown";
}

namespace {
const dispatcher::route_stats k_zero_stats{};
} // namespace

void dispatcher::register_route(uint16_t type, handler_fn fn, std::string name) {
    std::lock_guard<std::mutex> lock(mutex_);
    route r;
    r.fn = std::move(fn);
    r.name = name.empty() ? ("route_" + std::to_string(type)) : std::move(name);
    routes_[type] = std::move(r);
}

bool dispatcher::unregister_route(uint16_t type) {
    std::lock_guard<std::mutex> lock(mutex_);
    return routes_.erase(type) > 0;
}

bool dispatcher::has_route(uint16_t type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return routes_.find(type) != routes_.end();
}

std::vector<uint16_t> dispatcher::routes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint16_t> out;
    out.reserve(routes_.size());
    for (const auto& kv : routes_) out.push_back(kv.first);
    return out;
}

response dispatcher::dispatch(const request& req) {
    response resp;
    resp.id = req.id;

    handler_fn fn;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = routes_.find(req.type);
        if (it == routes_.end()) {
            ++unrouted_;
            resp.status = strict_ ? dispatch_status::unknown_route : dispatch_status::ok;
            resp.message = (strict_ ? "no route for type " : "ignored type ") +
                           std::to_string(req.type);
            return resp;
        }
        fn = it->second.fn;
    }

    if (filter_) {
        if (!filter_(req, resp)) {
            resp.status = dispatch_status::rejected;
            if (resp.message.empty()) resp.message = "rejected by filter";
            return resp;
        }
    }

    auto start = std::chrono::steady_clock::now();
    try {
        response handled = fn(req);
        handled.id = req.id;
        resp = std::move(handled);
    } catch (const std::exception& e) {
        resp.status = dispatch_status::handler_exception;
        resp.message = e.what();
    } catch (...) {
        resp.status = dispatch_status::handler_exception;
        resp.message = "unknown exception";
    }
    auto end = std::chrono::steady_clock::now();
    resp.elapsed_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

    std::lock_guard<std::mutex> lock(mutex_);
    route& r = routes_[req.type];
    ++r.stats.calls;
    if (resp.status != dispatch_status::ok) ++r.stats.failures;
    r.stats.total_us += resp.elapsed_us;
    r.stats.max_us = std::max(r.stats.max_us, resp.elapsed_us);
    return resp;
}

const dispatcher::route_stats& dispatcher::stats(uint16_t type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = routes_.find(type);
    if (it == routes_.end()) return k_zero_stats;
    return it->second.stats;
}

uint64_t dispatcher::total_calls() const {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t n = unrouted_;
    for (const auto& kv : routes_) n += kv.second.stats.calls;
    return n;
}

uint64_t dispatcher::total_failures() const {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t n = strict_ ? unrouted_ : 0;
    for (const auto& kv : routes_) n += kv.second.stats.failures;
    return n;
}

client_stub::client_stub(uint32_t client_id, transport_fn transport)
    : client_id_(client_id), transport_(std::move(transport)) {}

request_id client_stub::allocate(uint16_t type, const std::vector<uint8_t>& payload, request& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t seq = next_sequence_++;
    out.id = make_request_id(client_id_, seq);
    out.type = type;
    out.client_id = client_id_;
    out.sequence = seq;
    out.payload = payload;
    out.created = std::chrono::steady_clock::now();
    ++stats_.requests_sent;
    stats_.bytes_sent += payload.size();
    return out.id;
}

response client_stub::call(uint16_t type, const std::vector<uint8_t>& payload,
                           std::chrono::milliseconds timeout) {
    request req;
    allocate(type, payload, req);
    if (!transport_) {
        response r;
        r.id = req.id;
        r.status = dispatch_status::transport_error;
        r.message = "no transport";
        return r;
    }
    response r = transport_(req);
    if (r.status == dispatch_status::ok) ++stats_.responses_ok;
    if (timeout.count() >= 0 && r.elapsed_us > static_cast<uint64_t>(timeout.count() * 1000)) {
        // The transport answered late; report it as a timeout to the caller.
        ++stats_.timeouts;
        r.status = dispatch_status::timed_out;
        r.message = "response arrived after timeout";
    }
    return r;
}

request_id client_stub::call_async(uint16_t type, const std::vector<uint8_t>& payload) {
    request req;
    request_id id = allocate(type, payload, req);

    slot s;
    s.promise = std::make_shared<std::promise<response>>();
    s.future = s.promise->get_future().share();
    s.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_[id] = s;
    }

    response r;
    if (transport_) {
        r = transport_(req);
    } else {
        r.status = dispatch_status::transport_error;
        r.message = "no transport";
        r.id = id;
    }
    if (r.status == dispatch_status::ok) ++stats_.responses_ok;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pending_.find(id);
    if (it != pending_.end() && !it->second.cancelled) {
        it->second.promise->set_value(r);
        it->second.delivered = true;
    }
    return id;
}

bool client_stub::wait(request_id id, response& out, std::chrono::milliseconds timeout) {
    std::shared_future<response> fut;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pending_.find(id);
        if (it == pending_.end()) return false;
        fut = it->second.future;
    }
    if (fut.wait_for(timeout) != std::future_status::ready) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++stats_.timeouts;
        out.id = id;
        out.status = dispatch_status::timed_out;
        out.message = "wait timed out";
        pending_.erase(id);
        return true;
    }
    out = fut.get();
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.erase(id);
    return true;
}

bool client_stub::cancel(request_id id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pending_.find(id);
    if (it == pending_.end()) return false;
    // A promise can only be satisfied once, so a request the transport already
    // answered cannot be cancelled.
    if (it->second.cancelled || it->second.delivered) return false;
    it->second.cancelled = true;
    response r;
    r.id = id;
    r.status = dispatch_status::cancelled;
    r.message = "cancelled by client";
    it->second.promise->set_value(r);
    ++stats_.cancelled;
    pending_.erase(it);
    return true;
}

bool client_stub::pending(request_id id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_.find(id) != pending_.end();
}

size_t client_stub::pending_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_.size();
}

} // namespace cppux

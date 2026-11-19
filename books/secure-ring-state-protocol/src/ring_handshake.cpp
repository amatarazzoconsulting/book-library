// MIT License - (c) Anthony Matarazzo 2026
//
// ring_handshake.cpp - Hello / Challenge / Finalize with HMAC-SHA256.

#include "ring_handshake.hpp"

#include "cipher.hpp"

#include "sha256.hpp"

#include <algorithm>
#include <array>

namespace srsp {
namespace handshake {

namespace {

void put_u64(bytes& out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xffu));
}

void put_u32(bytes& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xffu));
}

void put_blob(bytes& out, const bytes& b) {
    put_u32(out, static_cast<std::uint32_t>(b.size()));
    out.insert(out.end(), b.begin(), b.end());
}

void put_str(bytes& out, const std::string& s) {
    put_u32(out, static_cast<std::uint32_t>(s.size()));
    out.insert(out.end(), s.begin(), s.end());
}

bool read_u32(const bytes& in, std::size_t& pos, std::uint32_t& v) {
    if (pos + 4 > in.size()) return false;
    v = 0;
    for (int i = 0; i < 4; ++i) v |= static_cast<std::uint32_t>(in[pos + i]) << (8 * i);
    pos += 4;
    return true;
}

bool read_u64(const bytes& in, std::size_t& pos, std::uint64_t& v) {
    if (pos + 8 > in.size()) return false;
    v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<std::uint64_t>(in[pos + i]) << (8 * i);
    pos += 8;
    return true;
}

bool read_blob(const bytes& in, std::size_t& pos, bytes& out) {
    std::uint32_t n = 0;
    if (!read_u32(in, pos, n)) return false;
    if (pos + n > in.size()) return false;
    out.assign(in.begin() + static_cast<std::ptrdiff_t>(pos),
               in.begin() + static_cast<std::ptrdiff_t>(pos + n));
    pos += n;
    return true;
}

bool read_str(const bytes& in, std::size_t& pos, std::string& out) {
    bytes b;
    if (!read_blob(in, pos, b)) return false;
    out.assign(b.begin(), b.end());
    return true;
}

bool all_zero(const bytes& b) {
    for (std::uint8_t c : b) {
        if (c != 0) return false;
    }
    return true;
}

}  // namespace

const char* to_string(step_kind k) {
    switch (k) {
        case step_kind::hello: return "hello";
        case step_kind::challenge: return "challenge";
        case step_kind::finalize: return "finalize";
        case step_kind::confirm: return "confirm";
    }
    return "unknown";
}

const char* to_string(error_code e) {
    switch (e) {
        case error_code::none: return "none";
        case error_code::bad_frame: return "bad_frame";
        case error_code::wrong_step: return "wrong_step";
        case error_code::wrong_recipient: return "wrong_recipient";
        case error_code::bad_mac: return "bad_mac";
        case error_code::zero_nonce: return "zero_nonce";
        case error_code::replay: return "replay";
        case error_code::not_established: return "not_established";
        case error_code::bad_ciphertext: return "bad_ciphertext";
    }
    return "unknown";
}

bytes encode(const wire_message& m) {
    bytes out;
    out.reserve(48 + m.nonce.size() + m.mac.size() + m.payload.size());
    out.push_back(static_cast<std::uint8_t>(m.kind));
    put_str(out, m.from_id);
    put_str(out, m.to_id);
    put_u64(out, m.seq);
    put_blob(out, m.nonce);
    put_blob(out, m.mac);
    put_blob(out, m.payload);
    return out;
}

bool decode(const bytes& frame, wire_message& out) {
    std::size_t pos = 0;
    if (frame.empty()) return false;
    const std::uint8_t kind = frame[pos++];
    if (kind < 1 || kind > 4) return false;
    out.kind = static_cast<step_kind>(kind);
    if (!read_str(frame, pos, out.from_id)) return false;
    if (!read_str(frame, pos, out.to_id)) return false;
    if (!read_u64(frame, pos, out.seq)) return false;
    if (!read_blob(frame, pos, out.nonce)) return false;
    if (!read_blob(frame, pos, out.mac)) return false;
    if (!read_blob(frame, pos, out.payload)) return false;
    return pos == frame.size();
}

bytes derive_key_material(const std::string& label, const bytes& seed, std::size_t out_len) {
    bytes out;
    out.reserve(out_len + 32);
    std::uint32_t counter = 1;
    while (out.size() < out_len) {
        bytes block(label.begin(), label.end());
        block.insert(block.end(), seed.begin(), seed.end());
        put_u32(block, counter);
        const auto d = hash::sha256::digest(block.data(), block.size());
        out.insert(out.end(), d.begin(), d.end());
        ++counter;
    }
    out.resize(out_len);
    return out;
}

bytes make_nonce(const bytes& seed, std::uint64_t counter) {
    bytes material = seed;
    put_u64(material, counter);
    return derive_key_material("srsp-nonce", material, 32);
}

bytes transcript(const std::string& a_id, const bytes& a_nonce, const std::string& b_id,
                 const bytes& b_nonce, const bytes& payload) {
    bytes buf(a_id.begin(), a_id.end());
    buf.push_back(0x1f);
    buf.insert(buf.end(), a_nonce.begin(), a_nonce.end());
    buf.push_back(0x1f);
    buf.insert(buf.end(), b_id.begin(), b_id.end());
    buf.push_back(0x1f);
    buf.insert(buf.end(), b_nonce.begin(), b_nonce.end());
    buf.push_back(0x1f);
    buf.insert(buf.end(), payload.begin(), payload.end());
    const auto d = hash::sha256::digest(buf.data(), buf.size());
    return bytes(d.begin(), d.end());
}

namespace {

// Associated data for a sealed frame: the session id plus both identities and
// both nonces in canonical order. Ordering matters because the two endpoints hold
// the same values in mirrored fields; without it each side would compute a
// different tag for the same frame.
srsp::cipher::bytes session_aad(const srsp::handshake::session& s) {
    srsp::cipher::bytes aad(s.session_id.begin(), s.session_id.end());
    const std::string& lo_id = s.local_id < s.peer_id ? s.local_id : s.peer_id;
    const std::string& hi_id = s.local_id < s.peer_id ? s.peer_id : s.local_id;
    aad.insert(aad.end(), lo_id.begin(), lo_id.end());
    aad.insert(aad.end(), hi_id.begin(), hi_id.end());
    const auto& lo_nonce = s.local_nonce < s.peer_nonce ? s.local_nonce : s.peer_nonce;
    const auto& hi_nonce = s.local_nonce < s.peer_nonce ? s.peer_nonce : s.local_nonce;
    aad.insert(aad.end(), lo_nonce.begin(), lo_nonce.end());
    aad.insert(aad.end(), hi_nonce.begin(), hi_nonce.end());
    return aad;
}

}  // namespace

bytes session::seal(const bytes& plaintext) const {
    // Post handshake frames are sealed with the genetic_cipher envelope: the
    // header's operation pipeline for confidentiality, our HMAC for integrity,
    // and the transcript hash as associated data so a frame cannot be replayed
    // into a different session.
    return cipher::seal(key, plaintext, session_aad(*this));
}

bool session::open(const bytes& sealed, bytes& out) const {
    return cipher::open(key, sealed, session_aad(*this), out);
}

std::string session::key_hex() const { return hash::to_hex(key); }

// ---------------------------------------------------------------- endpoint ---

endpoint::endpoint(identity id, options opt) : id_(std::move(id)), opt_(opt) {
    if (opt_.nonce_len < 8) opt_.nonce_len = 8;
    if (opt_.key_len < 16) opt_.key_len = 16;
    bytes seed = id_.secret;
    seed.insert(seed.end(), id_.node_id.begin(), id_.node_id.end());
    const auto d = hash::sha256::digest(seed.data(), seed.size());
    nonce_seed_.assign(d.begin(), d.end());
}

endpoint::~endpoint() = default;

void endpoint::trust_peer(const std::string& peer_id, const bytes& verify_key) {
    peers_[peer_id] = verify_key;
}

bool endpoint::trusts(const std::string& peer_id) const {
    return peers_.find(peer_id) != peers_.end();
}

void endpoint::set_nonce_override(const bytes& n) {
    has_override_ = true;
    override_nonce_ = n;
}

bytes endpoint::next_nonce() {
    if (has_override_) {
        has_override_ = false;
        return override_nonce_;
    }
    const bytes full = make_nonce(nonce_seed_, ++nonce_counter_);
    return bytes(full.begin(), full.begin() + static_cast<std::ptrdiff_t>(opt_.nonce_len));
}

namespace {

// Canonical ordering by node id so both endpoints derive the same transcript
// regardless of which side initiated.
bytes session_seed(const std::string& a_id, const bytes& a_nonce, const std::string& b_id,
                   const bytes& b_nonce) {
    if (a_id <= b_id) return transcript(a_id, a_nonce, b_id, b_nonce, bytes{});
    return transcript(b_id, b_nonce, a_id, a_nonce, bytes{});
}

bytes mac_with(const bytes& key, step_kind kind, const std::string& from,
               const std::string& to, std::uint64_t seq, const bytes& nonce,
               const bytes& payload) {
    bytes input;
    input.push_back(static_cast<std::uint8_t>(kind));
    input.insert(input.end(), from.begin(), from.end());
    input.push_back(0x1f);
    input.insert(input.end(), to.begin(), to.end());
    input.push_back(0x1f);
    put_u64(input, seq);
    input.insert(input.end(), nonce.begin(), nonce.end());
    input.insert(input.end(), payload.begin(), payload.end());
    const auto tag = hash::hmac(key, input);
    return bytes(tag.begin(), tag.end());
}

}  // namespace

bytes endpoint::compute_mac(step_kind kind, const std::string& from, const std::string& to,
                            std::uint64_t seq, const bytes& nonce, const bytes& payload) const {
    return mac_with(id_.secret, kind, from, to, seq, nonce, payload);
}

// Verify an inbound message against the key registered for its sender. An
// unknown sender is rejected outright.
bool endpoint::verify_from_peer(const wire_message& m) {
    auto it = peers_.find(m.from_id);
    if (it == peers_.end()) {
        ++mac_failures_;
        return false;
    }
    const bytes expect =
        mac_with(it->second, m.kind, m.from_id, m.to_id, m.seq, m.nonce, m.payload);
    if (!hash::constant_time_equal(expect, m.mac)) {
        ++mac_failures_;
        return false;
    }
    return true;
}

wire_message endpoint::make_hello(const std::string& peer_id, std::uint64_t seq) {
    local_nonce_ = next_nonce();
    peer_id_ = peer_id;
    wire_message m;
    m.kind = step_kind::hello;
    m.from_id = id_.node_id;
    m.to_id = peer_id;
    m.seq = seq;
    m.nonce = local_nonce_;
    m.payload = bytes{'s', 'r', 's', 'p', '1'};
    m.mac = compute_mac(m.kind, m.from_id, m.to_id, m.seq, m.nonce, m.payload);
    return m;
}

std::optional<wire_message> endpoint::handle_hello(const wire_message& hello, error_code& err) {
    err = error_code::none;
    if (hello.kind != step_kind::hello) {
        err = error_code::wrong_step;
        return std::nullopt;
    }
    if (hello.to_id != id_.node_id) {
        err = error_code::wrong_recipient;
        return std::nullopt;
    }
    if (hello.nonce.size() < 8) {
        err = error_code::bad_frame;
        return std::nullopt;
    }
    if (opt_.reject_zero_nonce && all_zero(hello.nonce)) {
        err = error_code::zero_nonce;
        return std::nullopt;
    }
    // The hello MAC proves the sender holds the key it registered with us.
    if (!verify_from_peer(hello)) {
        err = error_code::bad_mac;
        return std::nullopt;
    }

    peer_id_ = hello.from_id;
    peer_nonce_ = hello.nonce;
    peer_hello_seq_ = hello.seq;
    local_nonce_ = next_nonce();

    wire_message m;
    m.kind = step_kind::challenge;
    m.from_id = id_.node_id;
    m.to_id = peer_id_;
    m.seq = 2;
    m.nonce = local_nonce_;
    // The challenge proves liveness and binds the transcript of both nonces.
    m.payload = transcript(id_.node_id, local_nonce_, peer_id_, peer_nonce_, bytes{});
    m.mac = compute_mac(m.kind, m.from_id, m.to_id, m.seq, m.nonce, m.payload);
    return m;
}

std::optional<wire_message> endpoint::handle_challenge(const wire_message& challenge,
                                                       error_code& err) {
    err = error_code::none;
    if (challenge.kind != step_kind::challenge) {
        err = error_code::wrong_step;
        return std::nullopt;
    }
    if (challenge.to_id != id_.node_id) {
        err = error_code::wrong_recipient;
        return std::nullopt;
    }
    if (!verify_from_peer(challenge)) {
        err = error_code::bad_mac;
        return std::nullopt;
    }
    if (opt_.reject_zero_nonce && all_zero(challenge.nonce)) {
        err = error_code::zero_nonce;
        return std::nullopt;
    }
    peer_nonce_ = challenge.nonce;
    peer_id_ = challenge.from_id;

    // Verify the challenge payload really commits to our nonce.
    const bytes expected_transcript =
        transcript(peer_id_, peer_nonce_, id_.node_id, local_nonce_, bytes{});
    if (opt_.bind_identity && !hash::constant_time_equal(expected_transcript, challenge.payload)) {
        ++mac_failures_;
        err = error_code::bad_mac;
        return std::nullopt;
    }

    const bytes key_seed = session_seed(id_.node_id, local_nonce_, peer_id_, peer_nonce_);
    wire_message m;
    m.kind = step_kind::finalize;
    m.from_id = id_.node_id;
    m.to_id = peer_id_;
    m.seq = 3;
    m.nonce = bytes{};
    m.payload = derive_key_material("srsp-session", key_seed, opt_.key_len);
    m.mac = compute_mac(m.kind, m.from_id, m.to_id, m.seq, m.nonce, m.payload);

    // The initiator can install its session as soon as it has verified the peer
    // challenge and produced the finalize: the key no longer depends on
    // anything the peer still has to send.
    session s;
    s.local_id = id_.node_id;
    s.peer_id = peer_id_;
    s.epoch = opt_.session_epoch;
    s.key = m.payload;
    s.local_nonce = local_nonce_;
    s.peer_nonce = peer_nonce_;
    s.established = true;
    s.session_id = "sess-" + hash::to_hex(m.payload).substr(0, 16);
    session_ = s;
    highest_seq_ = m.seq;
    return m;
}

bool endpoint::handle_finalize(const wire_message& fin, error_code& err) {
    err = error_code::none;
    if (fin.kind != step_kind::finalize) {
        err = error_code::wrong_step;
        return false;
    }
    if (fin.to_id != id_.node_id) {
        err = error_code::wrong_recipient;
        return false;
    }
    if (!verify_from_peer(fin)) {
        err = error_code::bad_mac;
        return false;
    }

    // Both sides must derive the same key from the same transcript.
    const bytes key_seed = session_seed(id_.node_id, local_nonce_, peer_id_, peer_nonce_);
    const bytes derived = derive_key_material("srsp-session", key_seed, opt_.key_len);
    if (!hash::constant_time_equal(derived, fin.payload)) {
        ++mac_failures_;
        err = error_code::bad_mac;
        return false;
    }

    session s;
    s.local_id = id_.node_id;
    s.peer_id = peer_id_;
    s.epoch = opt_.session_epoch;
    s.key = derived;
    s.local_nonce = local_nonce_;
    s.peer_nonce = peer_nonce_;
    s.established = true;
    s.session_id = "sess-" + hash::to_hex(derived).substr(0, 16);
    session_ = s;
    highest_seq_ = fin.seq;
    return true;
}

std::optional<wire_message> endpoint::make_confirm() {
    if (!session_) return std::nullopt;
    wire_message m;
    m.kind = step_kind::confirm;
    m.from_id = id_.node_id;
    m.to_id = peer_id_;
    m.seq = 4;
    m.payload = session_->key;
    m.mac = compute_mac(m.kind, m.from_id, m.to_id, m.seq, m.nonce, m.payload);
    return m;
}

bool endpoint::handle_confirm(const wire_message& conf, error_code& err) {
    err = error_code::none;
    if (!session_) {
        err = error_code::not_established;
        return false;
    }
    if (conf.kind != step_kind::confirm) {
        err = error_code::wrong_step;
        return false;
    }
    if (conf.to_id != id_.node_id) {
        err = error_code::wrong_recipient;
        return false;
    }
    if (!verify_from_peer(conf)) {
        err = error_code::bad_mac;
        return false;
    }
    if (!hash::constant_time_equal(conf.payload, session_->key)) {
        ++mac_failures_;
        err = error_code::bad_mac;
        return false;
    }
    return true;
}

bool endpoint::accept_seq(std::uint64_t seq) {
    if (seq > highest_seq_) {
        highest_seq_ = seq;
        window_[seq] = true;
        if (window_.size() > opt_.replay_window * 4) {
            const std::uint64_t floor = highest_seq_ > opt_.replay_window
                                            ? highest_seq_ - opt_.replay_window
                                            : 0;
            for (auto it = window_.begin(); it != window_.end();) {
                if (it->first < floor) {
                    it = window_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        return true;
    }
    if (highest_seq_ - seq > opt_.replay_window) {
        ++replays_blocked_;
        return false;
    }
    if (window_.find(seq) != window_.end()) {
        ++replays_blocked_;
        return false;
    }
    window_[seq] = true;
    return true;
}

outcome run_handshake(endpoint& initiator, endpoint& responder, bool confirm_step) {
    outcome o;
    const auto hello = initiator.make_hello(responder.node_id(), 1);
    o.messages += 1;
    o.bytes_on_wire += encode(hello).size();

    error_code err = error_code::none;
    const auto challenge = responder.handle_hello(hello, err);
    if (!challenge) {
        o.error = err;
        o.detail = std::string("responder rejected hello: ") + to_string(err);
        return o;
    }
    o.messages += 1;
    o.bytes_on_wire += encode(*challenge).size();

    const auto finalize = initiator.handle_challenge(*challenge, err);
    if (!finalize) {
        o.error = err;
        o.detail = std::string("initiator rejected challenge: ") + to_string(err);
        return o;
    }
    o.messages += 1;
    o.bytes_on_wire += encode(*finalize).size();

    if (!responder.handle_finalize(*finalize, err)) {
        o.error = err;
        o.detail = std::string("responder rejected finalize: ") + to_string(err);
        return o;
    }

    if (confirm_step) {
        const auto confirm = responder.make_confirm();
        if (confirm) {
            o.messages += 1;
            o.bytes_on_wire += encode(*confirm).size();
            if (!initiator.handle_confirm(*confirm, err)) {
                o.error = err;
                o.detail = std::string("initiator rejected confirm: ") + to_string(err);
                return o;
            }
        }
    }

    if (initiator.established_session() && responder.established_session() &&
        initiator.established_session()->key_hex() ==
            responder.established_session()->key_hex()) {
        o.ok = true;
        o.detail = "session established, keys match";
    } else {
        o.error = error_code::bad_mac;
        o.detail = "session keys diverged";
    }
    return o;
}

outcome self_test(const std::string& a_id, const std::string& b_id, options opt) {
    identity a{a_id, derive_key_material("srsp-identity", bytes(a_id.begin(), a_id.end()), 32)};
    identity b{b_id, derive_key_material("srsp-identity", bytes(b_id.begin(), b_id.end()), 32)};
    endpoint ea(a, opt);
    endpoint eb(b, opt);
    ea.trust_peer(b_id, b.secret);
    eb.trust_peer(a_id, a.secret);
    return run_handshake(ea, eb, opt.require_mutual_auth);
}

}  // namespace handshake
}  // namespace srsp

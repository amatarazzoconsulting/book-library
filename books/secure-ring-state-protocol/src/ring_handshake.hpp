// MIT License - (c) Anthony Matarazzo 2026
//
// ring_handshake.hpp - three step Hello / Challenge / Finalize handshake.
//
// Implements the session establishment described in "SYSTEM DESCRIPTION":
// each side generates an ephemeral nonce, proves possession of its long term
// identity key by answering the peer nonce with an HMAC, and both sides derive
// the same session key from the transcript. Because the transcript includes
// both nonces and both identities, a replayed or reflected handshake yields a
// different key and fails verification. A sliding window rejects replayed
// message sequence numbers after the session is live.
//
// The transport is injected: handshake_driver moves messages between two
// endpoints without sockets, so the whole protocol is deterministic and
// testable. All primitives come from sha256.hpp.

#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace srsp {
namespace handshake {

using bytes = std::vector<std::uint8_t>;

enum class step_kind : std::uint8_t {
    hello = 1,
    challenge = 2,
    finalize = 3,
    confirm = 4
};

const char* to_string(step_kind k);

struct wire_message {
    step_kind kind = step_kind::hello;
    std::string from_id;
    std::string to_id;
    std::uint64_t seq = 0;
    bytes nonce;      // 32 bytes for hello/challenge, empty otherwise
    bytes mac;        // HMAC tag, 32 bytes
    bytes payload;    // optional opaque data, included in the transcript
};

// Encode and decode a fixed binary frame so benchmarks can report real bytes
// per message. Layout is little endian and length prefixed.
bytes encode(const wire_message& m);
bool decode(const bytes& frame, wire_message& out);

struct identity {
    std::string node_id;
    bytes secret;   // long term identity key material, 32 bytes recommended
};

bytes derive_key_material(const std::string& label, const bytes& seed, std::size_t out_len);
bytes make_nonce(const bytes& seed, std::uint64_t counter);

// Transcript hash: binds both identities, both nonces and both payloads.
bytes transcript(const std::string& a_id, const bytes& a_nonce, const std::string& b_id,
                 const bytes& b_nonce, const bytes& payload);

struct session {
    std::string session_id;
    std::string local_id;
    std::string peer_id;
    std::uint64_t epoch = 0;
    bytes key;             // 32 byte session key
    bytes peer_nonce;
    bytes local_nonce;
    std::uint64_t send_seq = 0;
    bool established = false;

    bytes seal(const bytes& plaintext) const;      // MAC then XOR keystream
    bool open(const bytes& sealed, bytes& out) const;
    std::string key_hex() const;
};

struct options {
    std::size_t nonce_len = 32;        // 16, 24 or 32 are sensible
    std::size_t key_len = 32;          // derived session key length
    bool require_mutual_auth = true;   // both sides must prove key possession
    bool bind_identity = true;         // mix node ids into the transcript
    std::uint64_t replay_window = 64;  // accepted out of order sequence span
    std::uint64_t session_epoch = 0;   // epoch stamped into the session
    bool reject_zero_nonce = true;     // refuse an all zero nonce from a peer
};

enum class error_code {
    none = 0,
    bad_frame,
    wrong_step,
    wrong_recipient,
    bad_mac,
    zero_nonce,
    replay,
    not_established,
    bad_ciphertext
};

const char* to_string(error_code e);

struct outcome {
    bool ok = false;
    error_code error = error_code::none;
    std::string detail;
    std::size_t messages = 0;
    std::size_t bytes_on_wire = 0;
};

class endpoint {
public:
    endpoint(identity id, options opt = options{});
    ~endpoint();

    const std::string& node_id() const { return id_.node_id; }
    const options& settings() const { return opt_; }

    // Step one: produce a hello carrying a fresh nonce.
    wire_message make_hello(const std::string& peer_id, std::uint64_t seq = 1);

    // Step two: answer a hello with a challenge. Returns nullopt and fills err
    // when the hello is malformed or the nonce is unacceptable.
    std::optional<wire_message> handle_hello(const wire_message& hello, error_code& err);

    // Step three: verify the peer challenge, emit finalize.
    std::optional<wire_message> handle_challenge(const wire_message& challenge, error_code& err);

    // Step four: verify the peer finalize and confirm. On success the session
    // is established for both sides.
    bool handle_finalize(const wire_message& fin, error_code& err);
    std::optional<wire_message> make_confirm();

    // Optional last message so the responder also sees an explicit confirm.
    bool handle_confirm(const wire_message& conf, error_code& err);

    const std::optional<session>& established_session() const { return session_; }
    std::uint64_t highest_seq() const { return highest_seq_; }

    // Anti replay window used by session::open for post handshake traffic.
    bool accept_seq(std::uint64_t seq);

    // The handshake authenticates with pre-shared identity keys: each endpoint
    // must hold the peer's verification key, which plays the role the peer's
    // public key plays in the book. HMAC is symmetric, so this is a trust store
    // model, not a certificate chain. Documented in README.
    void trust_peer(const std::string& peer_id, const bytes& verify_key);
    std::size_t trusted_peers() const { return peers_.size(); }
    bool trusts(const std::string& peer_id) const;

    void set_nonce_override(const bytes& n);  // deterministic testing hook
    void set_epoch(std::uint64_t e) { opt_.session_epoch = e; }

    std::size_t mac_failures() const { return mac_failures_; }
    std::size_t replays_blocked() const { return replays_blocked_; }

private:
    bytes next_nonce();
    bytes compute_mac(step_kind kind, const std::string& from, const std::string& to,
                      std::uint64_t seq, const bytes& nonce, const bytes& payload) const;
    bool verify_from_peer(const wire_message& m);

    identity id_;
    options opt_;
    std::map<std::string, bytes> peers_;
    bytes nonce_seed_;
    std::uint64_t nonce_counter_ = 0;
    bool has_override_ = false;
    bytes override_nonce_;

    bytes local_nonce_;
    bytes peer_nonce_;
    std::string peer_id_;
    std::uint64_t peer_hello_seq_ = 0;
    std::uint64_t highest_seq_ = 0;
    std::map<std::uint64_t, bool> window_;

    std::optional<session> session_;
    std::size_t mac_failures_ = 0;
    std::size_t replays_blocked_ = 0;
};

// Drives two endpoints to completion. Returns the shared outcome; on success
// both endpoints hold a session with the same key.
outcome run_handshake(endpoint& initiator, endpoint& responder, bool confirm_step = true);

// One shot helper: builds identities from seeds and returns the outcome.
outcome self_test(const std::string& a_id, const std::string& b_id, options opt = options{});

}  // namespace handshake
}  // namespace srsp

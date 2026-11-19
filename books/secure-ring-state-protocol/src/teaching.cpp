// MIT License - (c) Anthony Matarazzo 2026
//
// teaching.cpp - every option, every parameter, explained as it prints.
//
// Sections follow the four algorithms. Each block prints what it is about to do,
// does it, and then prints the value it got back, so the reader can run this
// next to the lessons and check each claim.

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "audit_log.hpp"
#include "key_rotation.hpp"
#include "ring_handshake.hpp"
#include "consensus.hpp"
#include "ring_state_machine.hpp"
#include "spinlock.hpp"
#include "store.hpp"
#include "sha256.hpp"

namespace {

using bytes = std::vector<std::uint8_t>;

void head(const std::string& t) {
    std::cout << "\n------------------------------------------------------------\n";
    std::cout << t << "\n";
    std::cout << "------------------------------------------------------------\n";
}

void note(const std::string& t) { std::cout << "  # " << t << "\n"; }

bytes secret(const std::string& label) {
    return srsp::handshake::derive_key_material("teaching", bytes(label.begin(), label.end()), 32);
}

// ------------------------------------------------------- 0. the hash unit ---
void section_hash() {
    head("0. sha256.hpp - the one shared primitive");
    note("Every other algorithm is built on this header, so check it first.");
    std::cout << "  sha256(\"abc\") = " << srsp::hash::digest_hex("abc") << "\n";
    note("That is the FIPS 180-4 test vector. If it differs, nothing else can work.");

    const bytes key(32, 0x0b);
    const auto tag = srsp::hash::hmac(key, std::string("message"));
    std::cout << "  hmac tag       = " << srsp::hash::to_hex(tag).substr(0, 32) << "...\n";
    note("hmac() is what the handshake and the keyed audit log authenticate with.");

    const bytes a{1, 2, 3};
    const bytes b{1, 2, 4};
    std::cout << "  equal(a,a)     = " << srsp::hash::constant_time_equal(a, a)
              << ", equal(a,b) = " << srsp::hash::constant_time_equal(a, b) << "\n";
    note("constant_time_equal does not short circuit on the first differing byte.");
}

// ---------------------------------------------------- 1. ring state machine --
void section_state_machine() {
    head("1. ring_machine - guarded lifecycle");

    srsp::ring::ring_machine::config cfg;
    note("config fields and their defaults:");
    std::cout << "  quorum_required = " << cfg.quorum_required << "  (votes needed to join)\n";
    std::cout << "  peer_count      = " << cfg.peer_count << "  (ring size this node assumes)\n";
    std::cout << "  max_retries     = " << cfg.max_retries << "  (failed join attempts tolerated)\n";
    std::cout << "  allow_degraded  = " << cfg.allow_degraded << "  (permit active -> degraded)\n";
    std::cout << "  strict_guards   = " << cfg.strict_guards << "  (unknown guard fails)\n";
    std::cout << "  history_limit   = " << cfg.history_limit << "  (entries kept in history())\n";

    srsp::ring::ring_machine m;
    std::cout << "\n  states: ";
    for (std::size_t i = 0; i < srsp::ring::state_count(); ++i) {
        std::cout << srsp::ring::to_string(static_cast<srsp::ring::ring_state>(i)) << " ";
    }
    std::cout << "\n";
    note("state_count() is 9, and to_string/parse_state round trip every name.");
    srsp::ring::ring_state parsed = srsp::ring::ring_state::boot;
    std::cout << "  parse_state(\"syncing\") = " << srsp::ring::parse_state("syncing", parsed)
              << " -> " << srsp::ring::to_string(parsed) << "\n";
    std::cout << "  parse_state(\"nonsense\") = " << srsp::ring::parse_state("nonsense", parsed)
              << " (returns false, leaves out untouched)\n";

    note("The default table has " + std::to_string(m.transition_count()) + " edges.");
    for (std::size_t i = 0; i < srsp::ring::state_count(); ++i) {
        const auto from = static_cast<srsp::ring::ring_state>(i);
        const auto edges = m.transitions_from(from);
        if (edges.empty()) continue;
        std::cout << "  " << std::setw(9) << srsp::ring::to_string(from) << " ->";
        for (const auto& e : edges) std::cout << " " << e;
        std::cout << "\n";
    }
    note("Each edge string is target:guard, so the table doubles as documentation.");

    head("1a. guard_context - the facts a guard may read");
    srsp::ring::guard_context ctx;
    std::cout << "  quorum_votes=" << ctx.quorum_votes << " quorum_required=" << ctx.quorum_required
              << " peers_reachable=" << ctx.peers_reachable
              << " peers_expected=" << ctx.peers_expected << "\n";
    std::cout << "  integrity_ok=" << ctx.integrity_ok << " lease_valid=" << ctx.lease_valid
              << " drain_requested=" << ctx.drain_requested
              << " drain_complete=" << ctx.drain_complete << "\n";
    std::cout << "  rekey_ready=" << ctx.rekey_ready << " epoch=" << ctx.epoch
              << " retries=" << ctx.retries << "\n";
    note("The machine never reads a clock or a socket; the caller fills this in.");

    head("1b. guard evaluation table");
    note("Left column is the default context, right column is a permissive one.");
    srsp::ring::guard_context open;
    open.quorum_votes = 3;
    open.quorum_required = 2;
    open.peers_reachable = 3;
    open.drain_requested = true;
    open.drain_complete = true;
    open.rekey_ready = true;
    open.epoch = 7;
    std::cout << "  " << std::setw(15) << "guard" << std::setw(10) << "default"
              << std::setw(10) << "open" << "\n";
    for (const auto& g : srsp::ring::ring_machine::guard_names()) {
        std::cout << "  " << std::setw(15) << g << std::setw(10) << m.guard_ok(g, ctx)
                  << std::setw(10) << m.guard_ok(g, open) << "\n";
    }
    note("quorum_lost is true on the default context because 0 votes is below 2.");
    note("epoch_advanced compares ctx.epoch against the machine's own epoch.");

    head("1c. step() and advance()");
    srsp::ring::ring_machine walk;
    srsp::ring::guard_context good;
    good.peers_reachable = 3;
    good.quorum_votes = 3;
    good.quorum_required = 2;

    auto illegal = walk.step(srsp::ring::ring_state::active, good);
    std::cout << "  step(active) from boot : ok=" << illegal.ok << " reason=\"" << illegal.reason
              << "\"\n";
    note("There is no boot -> active edge, so the skip is refused and counted.");

    const std::size_t moved = walk.advance(good);
    std::cout << "  advance()              : moved " << moved << " states, now "
              << srsp::ring::to_string(walk.state()) << ", seq=" << walk.seq()
              << ", epoch=" << walk.epoch() << "\n";
    note("advance() only walks forward along probing, joining, syncing, active.");

    auto again = walk.step(srsp::ring::ring_state::active, good);
    std::cout << "  step(active) again     : ok=" << again.ok << " reason=\"" << again.reason
              << "\"\n";

    head("1d. reachability, paths and counters");
    std::cout << "  can_reach(sealed)      = " << walk.can_reach(srsp::ring::ring_state::sealed)
              << "\n";
    std::cout << "  can_reach(boot)        = " << walk.can_reach(srsp::ring::ring_state::boot)
              << " (sealed -> probing -> ... -> boot is impossible)\n";
    std::cout << "  path_to(sealed)        =";
    for (auto s : walk.path_to(srsp::ring::ring_state::sealed)) {
        std::cout << " " << srsp::ring::to_string(s);
    }
    std::cout << "\n";
    note("path_to ignores guards; it answers 'is there a route', not 'will it pass'.");

    std::cout << "  visits                 =";
    for (const auto& v : walk.visit_counts()) {
        if (v.second != 0) std::cout << " " << srsp::ring::to_string(v.first) << "=" << v.second;
    }
    std::cout << "\n";
    std::cout << "  rejected=" << walk.rejected() << " failures=" << walk.failures() << "\n";

    head("1e. editing the table");
    std::cout << "  add(boot,sealed,\"always\")  = "
              << walk.add_transition(srsp::ring::ring_state::boot, srsp::ring::ring_state::sealed,
                                     "always")
              << " (added, so boot can now jump to sealed)\n";
    std::cout << "  add(boot,probing,\"always\") = "
              << walk.add_transition(srsp::ring::ring_state::boot, srsp::ring::ring_state::probing,
                                     "always")
              << " (duplicate edge refused)\n";
    std::cout << "  add(boot,boot,\"always\")    = "
              << walk.add_transition(srsp::ring::ring_state::boot, srsp::ring::ring_state::boot,
                                     "always")
              << " (self edges refused)\n";
    std::cout << "  add(...,\"made_up\")         = "
              << walk.add_transition(srsp::ring::ring_state::boot, srsp::ring::ring_state::draining,
                                     "made_up")
              << " (unknown guard name refused)\n";
    std::cout << "  remove(probing,joining)     = "
              << walk.remove_transition(srsp::ring::ring_state::probing,
                                        srsp::ring::ring_state::joining)
              << " -> count " << walk.transition_count() << "\n";
    walk.install_default_table();
    std::cout << "  install_default_table()     -> count " << walk.transition_count() << "\n";
    note("install_default_table() wipes any edits and restores the 19 built in edges.");

    head("1f. config that changes behaviour");
    srsp::ring::ring_machine::config loose_cfg;
    loose_cfg.strict_guards = false;
    srsp::ring::ring_machine loose(loose_cfg);
    std::cout << "  strict_guards=false, unknown guard = " << loose.guard_ok("mystery", ctx)
              << " (passes when not strict)\n";

    srsp::ring::ring_machine::config tight;
    tight.max_retries = 1;
    srsp::ring::ring_machine retrying(tight);
    srsp::ring::guard_context tired;
    tired.peers_reachable = 1;
    tired.retries = 4;
    retrying.step(srsp::ring::ring_state::probing, tired);
    auto over = retrying.step(srsp::ring::ring_state::failed, tired);
    std::cout << "  max_retries=1, retries=4     -> ok=" << over.ok << " \"" << over.reason
              << "\"\n";

    srsp::ring::ring_machine::config small;
    small.history_limit = 2;
    srsp::ring::ring_machine trimmed(small);
    trimmed.advance(good);
    srsp::ring::guard_context bounce = good;
    bounce.drain_requested = true;
    for (int i = 0; i < 3; ++i) {
        trimmed.step(srsp::ring::ring_state::draining, bounce);
        trimmed.install_default_table();
        trimmed.step(srsp::ring::ring_state::active, bounce);
    }
    std::cout << "  history_limit=2 after " << trimmed.seq() << " transitions -> history size "
              << trimmed.history().size() << "\n";
    note("seq keeps counting even when old history entries are dropped.");

    srsp::ring::ring_machine::config no_degrade;
    no_degrade.allow_degraded = false;
    srsp::ring::ring_machine strict_ring(no_degrade);
    std::cout << "  allow_degraded=false         -> quorum_lost guard = "
              << strict_ring.guard_ok("quorum_lost", ctx) << "\n";
    note("With degradation off a node that loses quorum has nowhere to go but failed.");

    head("1g. epoch and reset");
    srsp::ring::ring_machine rekeyed;
    srsp::ring::guard_context sealed_ctx;
    sealed_ctx.peers_reachable = 1;
    sealed_ctx.quorum_votes = 3;
    rekeyed.advance(sealed_ctx);
    sealed_ctx.drain_requested = true;
    sealed_ctx.drain_complete = true;
    rekeyed.step(srsp::ring::ring_state::draining, sealed_ctx);
    rekeyed.step(srsp::ring::ring_state::sealed, sealed_ctx);
    sealed_ctx.epoch = 5;
    auto reopened = rekeyed.step(srsp::ring::ring_state::probing, sealed_ctx);
    std::cout << "  sealed -> probing with epoch=5 : ok=" << reopened.ok << " epoch now "
              << rekeyed.epoch() << "\n";
    note("epoch_advanced is the only guard that also writes the epoch counter.");
    rekeyed.reset();
    std::cout << "  reset()                        -> " << srsp::ring::to_string(rekeyed.state())
              << ", seq=" << rekeyed.seq() << ", history=" << rekeyed.history().size() << "\n";
}

// --------------------------------------------------------- 2. handshake -----
void section_handshake() {
    head("2. endpoint - hello / challenge / finalize");

    srsp::handshake::options opt;
    note("options fields and their defaults:");
    std::cout << "  nonce_len           = " << opt.nonce_len << "  (bytes of nonce per side)\n";
    std::cout << "  key_len             = " << opt.key_len << "  (session key length)\n";
    std::cout << "  require_mutual_auth = " << opt.require_mutual_auth << "  (send confirm step)\n";
    std::cout << "  bind_identity       = " << opt.bind_identity << "  (fold ids into transcript)\n";
    std::cout << "  replay_window       = " << opt.replay_window << "  (post handshake seq span)\n";
    std::cout << "  session_epoch       = " << opt.session_epoch << "  (stamped into the session)\n";
    std::cout << "  reject_zero_nonce   = " << opt.reject_zero_nonce << "  (refuse all zero nonce)\n";

    head("2a. a full handshake, message by message");
    srsp::handshake::identity a{"node-a", secret("node-a")};
    srsp::handshake::identity b{"node-b", secret("node-b")};
    srsp::handshake::endpoint ea(a);
    srsp::handshake::endpoint eb(b);
    ea.trust_peer("node-b", b.secret);
    eb.trust_peer("node-a", a.secret);
    std::cout << "  trusted_peers() : a=" << ea.trusted_peers() << " b=" << eb.trusted_peers()
              << ", a trusts node-b = " << ea.trusts("node-b") << "\n";
    note("trust_peer installs the peer's verification key. No key, no handshake.");

    const auto hello = ea.make_hello("node-b", 1);
    std::cout << "  1 hello     : kind=" << srsp::handshake::to_string(hello.kind)
              << " seq=" << hello.seq << " nonce=" << hello.nonce.size() << "B frame="
              << srsp::handshake::encode(hello).size() << "B\n";
    srsp::handshake::error_code err = srsp::handshake::error_code::none;
    const auto challenge = eb.handle_hello(hello, err);
    std::cout << "  2 challenge : kind=" << srsp::handshake::to_string(challenge->kind)
              << " nonce=" << challenge->nonce.size() << "B payload=" << challenge->payload.size()
              << "B (transcript hash)\n";
    const auto finalize = ea.handle_challenge(*challenge, err);
    std::cout << "  3 finalize  : kind=" << srsp::handshake::to_string(finalize->kind)
              << " payload=" << finalize->payload.size() << "B (derived session key)\n";
    const bool fin_ok = eb.handle_finalize(*finalize, err);
    std::cout << "    responder accepts finalize : " << fin_ok << "\n";
    const auto confirm = eb.make_confirm();
    const bool conf_ok = ea.handle_confirm(*confirm, err);
    std::cout << "  4 confirm   : kind=" << srsp::handshake::to_string(confirm->kind) << " ok="
              << conf_ok << "\n";
    std::cout << "  session ids : " << ea.established_session()->session_id << " / "
              << eb.established_session()->session_id << "\n";
    note("Both sides derive the same key from the same transcript, so ids match.");

    head("2b. wire codec");
    const bytes frame = srsp::handshake::encode(hello);
    srsp::handshake::wire_message decoded;
    const bool codec_ok = srsp::handshake::decode(frame, decoded);
    std::cout << "  encode(hello) = " << frame.size() << " bytes, decode -> " << codec_ok
              << ", from=" << decoded.from_id << " seq=" << decoded.seq << "\n";
    std::cout << "  decode(empty) = " << srsp::handshake::decode(bytes{}, decoded) << "\n";
    std::cout << "  decode(garbage) = "
              << srsp::handshake::decode(bytes{99, 1, 2, 3}, decoded) << "\n";
    note("A frame that does not consume exactly its own bytes is rejected.");

    head("2c. nonce_len and key_len");
    std::cout << "  " << std::setw(10) << "nonce_len" << std::setw(12) << "bytes_on_wire"
              << "\n";
    for (std::size_t n : {std::size_t{8}, std::size_t{16}, std::size_t{24}, std::size_t{32}}) {
        srsp::handshake::options o;
        o.nonce_len = n;
        const auto r = srsp::handshake::self_test("node-a", "node-b", o);
        std::cout << "  " << std::setw(10) << n << std::setw(14) << r.bytes_on_wire << "  ok="
                  << r.ok << "\n";
    }
    note("Every extra nonce byte costs 2 bytes on the wire, one per direction.");
    for (std::size_t k : {std::size_t{16}, std::size_t{32}, std::size_t{64}}) {
        srsp::handshake::options o;
        o.key_len = k;
        const auto r = srsp::handshake::self_test("node-a", "node-b", o);
        std::cout << "  key_len=" << std::setw(3) << k << " -> ok=" << r.ok << "\n";
    }

    head("2d. require_mutual_auth");
    srsp::handshake::options one_way;
    one_way.require_mutual_auth = false;
    const auto short_run = srsp::handshake::self_test("node-a", "node-b", one_way);
    std::cout << "  require_mutual_auth=false -> messages=" << short_run.messages << " bytes="
              << short_run.bytes_on_wire << " ok=" << short_run.ok << "\n";
    note("Three messages instead of four: the initiator still knows the key first.");

    head("2e. bind_identity");
    srsp::handshake::options unbound;
    unbound.bind_identity = false;
    const auto loose = srsp::handshake::self_test("node-a", "node-b", unbound);
    std::cout << "  bind_identity=false -> ok=" << loose.ok
              << " (challenge payload no longer compared)\n";
    note("Leave this on. Without it a challenge is not bound to the two identities.");

    head("2f. replay_window and accept_seq");
    srsp::handshake::endpoint guard(a);
    guard.trust_peer("node-b", b.secret);
    const std::uint64_t probes[] = {1, 1, 5, 2, 100, 40, 200, 10};
    for (std::uint64_t s : probes) {
        std::cout << "  accept_seq(" << std::setw(3) << s << ") = " << guard.accept_seq(s)
                  << "   highest=" << guard.highest_seq() << "\n";
    }
    std::cout << "  replays_blocked() = " << guard.replays_blocked() << "\n";
    note("A repeat is refused. A gap wider than replay_window is refused as too old.");
    std::cout << "  replay_window=" << guard.settings().replay_window << "\n";

    head("2g. session_epoch and set_epoch");
    srsp::handshake::options stamped;
    stamped.session_epoch = 12;
    srsp::handshake::identity a2{"node-a", secret("node-a")};
    srsp::handshake::identity b2{"node-b", secret("node-b")};
    srsp::handshake::endpoint ea2(a2, stamped);
    srsp::handshake::endpoint eb2(b2, stamped);
    ea2.trust_peer("node-b", b2.secret);
    eb2.trust_peer("node-a", a2.secret);
    srsp::handshake::run_handshake(ea2, eb2);
    std::cout << "  session epoch = " << ea2.established_session()->epoch << "\n";
    ea2.set_epoch(13);
    std::cout << "  after set_epoch(13), next session would carry 13 (session already built "
                 "keeps 12)\n";

    head("2h. seal() and open()");
    const auto s = *ea.established_session();
    const std::vector<std::string> samples{"", "ring", std::string(64, 'x')};
    for (const std::string& text : samples) {
        const bytes pt(text.begin(), text.end());
        const bytes sealed = s.seal(pt);
        bytes back;
        const bool ok = s.open(sealed, back);
        std::cout << "  plaintext " << std::setw(3) << pt.size() << "B -> sealed " << sealed.size()
                  << "B, open=" << ok << ", identical=" << (back == pt) << "\n";
    }
    note("sealed size is always plaintext + 32: the HMAC tag travels in front.");
    bytes corrupted = s.seal(bytes{'h', 'i'});
    corrupted[3] = static_cast<std::uint8_t>(corrupted[3] ^ 0xffu);
    bytes ignored;
    std::cout << "  one flipped bit -> open = " << s.open(corrupted, ignored) << "\n";
    std::cout << "  key_hex() = " << s.key_hex().substr(0, 16) << "...\n";

    head("2i. every error_code");
    srsp::handshake::endpoint fresh(b);
    fresh.trust_peer("node-a", a.secret);
    srsp::handshake::endpoint stranger(a);
    auto bad_recipient = stranger.make_hello("nobody");
    err = srsp::handshake::error_code::none;
    fresh.handle_hello(bad_recipient, err);
    std::cout << "  wrong recipient  -> " << srsp::handshake::to_string(err) << "\n";

    srsp::handshake::endpoint zero_maker(a);
    zero_maker.set_nonce_override(bytes(32, 0));
    auto zero_hello = zero_maker.make_hello("node-b");
    err = srsp::handshake::error_code::none;
    fresh.handle_hello(zero_hello, err);
    std::cout << "  all zero nonce   -> " << srsp::handshake::to_string(err) << "\n";

    err = srsp::handshake::error_code::none;
    fresh.handle_confirm(*confirm, err);
    std::cout << "  no session yet   -> " << srsp::handshake::to_string(err) << "\n";
    for (auto code : {srsp::handshake::error_code::none, srsp::handshake::error_code::bad_frame,
                      srsp::handshake::error_code::wrong_step,
                      srsp::handshake::error_code::wrong_recipient,
                      srsp::handshake::error_code::bad_mac, srsp::handshake::error_code::zero_nonce,
                      srsp::handshake::error_code::replay,
                      srsp::handshake::error_code::not_established,
                      srsp::handshake::error_code::bad_ciphertext}) {
        std::cout << "  " << srsp::handshake::to_string(code);
    }
    std::cout << "\n";
    note("An impostor without the trusted key fails at bad_mac on the hello itself.");

    head("2j. helpers");
    const auto nonce1 = srsp::handshake::make_nonce(secret("seed"), 1);
    const auto nonce2 = srsp::handshake::make_nonce(secret("seed"), 2);
    std::cout << "  make_nonce(counter 1 vs 2) differ = " << (nonce1 != nonce2) << "\n";
    const auto tr = srsp::handshake::transcript("a", nonce1, "b", nonce2, bytes{'p'});
    std::cout << "  transcript size = " << tr.size() << " bytes\n";
    const auto km = srsp::handshake::derive_key_material("label", nonce1, 48);
    std::cout << "  derive_key_material(out_len=48) = " << km.size() << " bytes\n";
    note("Transcripts and derived keys are plain SHA-256 outputs, so 32 bytes.");
}

// ------------------------------------------------------- 3. key rotation ----
void section_key_rotation() {
    head("3. rotation_schedule - epoch keys");

    srsp::keys::config cfg;
    note("config fields and their defaults:");
    std::cout << "  mode               = " << srsp::keys::to_string(cfg.mode) << "\n";
    std::cout << "  rotate_after_ops   = " << cfg.rotate_after_ops << "\n";
    std::cout << "  rotate_after_ticks = " << cfg.rotate_after_ticks << "\n";
    std::cout << "  overlap_epochs     = " << cfg.overlap_epochs << "\n";
    std::cout << "  key_len            = " << cfg.key_len << "\n";
    std::cout << "  start_epoch        = " << cfg.start_epoch << "\n";
    std::cout << "  allow_backfill     = " << cfg.allow_backfill << "\n";

    const bytes master = secret("ring-master");

    head("3a. fixed_epoch");
    srsp::keys::config fixed;
    fixed.rotate_after_ops = 3;
    srsp::keys::rotation_schedule fs(fixed, master);
    std::cout << "  start epoch " << fs.current_epoch() << " fingerprint "
              << fs.current().fingerprint() << "\n";
    for (int i = 1; i <= 7; ++i) {
        const auto v = fs.record_operation();
        std::cout << "  op " << i << " -> epoch " << v.epoch << " " << srsp::keys::to_string(v.kind)
                  << "  ops_in_epoch=" << fs.ops_in_epoch() << "\n";
    }
    note("The op that crosses the threshold is the one that rotates, so it serves the new epoch.");

    head("3b. time_based");
    srsp::keys::config timed;
    timed.mode = srsp::keys::policy::time_based;
    timed.rotate_after_ticks = 10;
    srsp::keys::rotation_schedule ts(timed, master);
    ts.tick(9);
    auto v9 = ts.record_operation();
    std::cout << "  now=" << ts.now() << " -> epoch " << v9.epoch << "\n";
    ts.tick(1);
    auto v10 = ts.record_operation();
    std::cout << "  now=" << ts.now() << " -> epoch " << v10.epoch << "\n";
    note("tick() is a logical clock. Nothing rotates until the caller ticks and records.");

    head("3c. sliding_window");
    srsp::keys::config sliding;
    sliding.mode = srsp::keys::policy::sliding_window;
    sliding.overlap_epochs = 2;
    srsp::keys::rotation_schedule ss(sliding, master);
    for (int i = 0; i < 3; ++i) ss.force_rotate(1);
    std::cout << "  current epoch " << ss.current_epoch() << ", acceptance_window="
              << ss.acceptance_window() << "\n";
    std::cout << "  retained epochs:";
    for (auto e : ss.retained_epochs()) std::cout << " " << e;
    std::cout << "\n";
    for (std::uint64_t e : {1ull, 2ull, 3ull, 4ull, 5ull}) {
        const auto c = ss.check(e);
        std::cout << "  check(" << e << ") -> " << (c.ok ? "accept" : "reject") << " "
                  << srsp::keys::to_string(c.kind) << " : " << c.reason << "\n";
    }
    note("overlap_epochs=2 accepts current plus two behind, and rejects anything ahead.");

    head("3d. acceptance windows side by side");
    for (auto p : {srsp::keys::policy::fixed_epoch, srsp::keys::policy::time_based,
                   srsp::keys::policy::sliding_window}) {
        srsp::keys::config c;
        c.mode = p;
        c.overlap_epochs = 3;
        srsp::keys::rotation_schedule s(c, master);
        std::cout << "  " << std::setw(15) << srsp::keys::to_string(p) << " window = "
                  << s.acceptance_window() << "\n";
    }
    note("Only sliding_window widens; overlap_epochs is ignored by the other two.");

    head("3e. derive, get, revoke");
    srsp::keys::rotation_schedule ds(cfg, master);
    std::cout << "  derive(4) == derive(4) : " << (ds.derive(4) == ds.derive(4)) << "\n";
    std::cout << "  derive(4) != derive(5) : " << (ds.derive(4) != ds.derive(5)) << "\n";
    std::cout << "  derive_labeled(\"other\",4) != derive(4) : "
              << (ds.derive_labeled("other", 4) != ds.derive(4)) << "\n";
    note("A node holding only the master can rebuild any epoch key, and only those.");
    std::cout << "  get(1) present = " << ds.get(1).has_value() << ", get(99) present = "
              << ds.get(99).has_value() << "\n";

    srsp::keys::config wide;
    wide.mode = srsp::keys::policy::sliding_window;
    wide.overlap_epochs = 2;
    srsp::keys::rotation_schedule ws(wide, master);
    ws.force_rotate(1);
    ws.force_rotate(1);
    const std::uint64_t older = ws.current_epoch() - 1;
    std::cout << "  check(" << older << ") before revoke = " << ws.check(older).ok << "\n";
    std::cout << "  revoke(" << older << ") = " << ws.revoke(older) << "\n";
    std::cout << "  check(" << older << ") after revoke  = " << ws.check(older).ok << " : "
              << ws.check(older).reason << "\n";
    std::cout << "  revoke(999) = " << ws.revoke(999) << " (unknown epoch)\n";

    head("3f. force_rotate, counters, journal");
    srsp::keys::rotation_schedule js(cfg, master);
    std::cout << "  force_rotate(0) -> epoch " << js.force_rotate(0).epoch << " (0 is clamped to 1)\n";
    std::cout << "  force_rotate(5) -> epoch " << js.force_rotate(5).epoch << "\n";
    (void)js.check(js.current_epoch());
    (void)js.check(js.current_epoch() + 3);
    std::cout << "  rotations=" << js.rotations() << " accepts=" << js.accepts()
              << " rejects=" << js.rejects() << "\n";
    note("accepts and rejects count check() verdicts, including the two just made.");
    std::cout << "  journal entries = " << js.journal().size() << "\n";
    for (const auto& e : js.journal()) {
        std::cout << "    epoch " << e.epoch << " " << srsp::keys::to_string(e.kind) << " : "
                  << e.detail << "\n";
    }
    js.reset();
    std::cout << "  reset() -> epoch " << js.current_epoch() << " journal "
              << js.journal().size() << "\n";
}

// --------------------------------------------------------- 4. audit log -----
void section_audit() {
    head("4. audit_log - chained records");

    srsp::audit::options opt;
    note("options fields and their defaults:");
    std::cout << "  keyed           = " << opt.keyed << "  (HMAC instead of plain hash)\n";
    std::cout << "  hash_len        = " << opt.hash_len << "  (stored hash bytes, max 32)\n";
    std::cout << "  include_payload = " << opt.include_payload << "  (fold payload into hash)\n";
    std::cout << "  max_records     = " << opt.max_records << "  (0 means unbounded)\n";
    std::cout << "  fsync_each      = " << opt.fsync_each << "  (record a durable write)\n";

    head("4a. append and verify");
    srsp::audit::audit_log log;
    for (int i = 0; i < 4; ++i) {
        log.tick(2);
        log.append(srsp::audit::severity::info, "node-a", "op",
                   std::string("record ") + std::to_string(i));
    }
    std::cout << "  size=" << log.size() << " empty=" << log.empty() << " now=" << log.now()
              << "\n";
    for (const auto& r : log.records()) {
        std::cout << "  [" << r.index << "] t=" << r.logical_time << " "
                  << srsp::audit::to_string(r.level) << " " << r.actor << " " << r.action << " : "
                  << r.detail << " hash=" << srsp::hash::to_hex(r.hash).substr(0, 12) << "\n";
    }
    std::cout << "  verify : ok=" << log.verify().ok << " checked=" << log.verify().checked << " \""
              << log.verify().reason << "\"\n";
    note("prev_hash of record 0 is all zeroes; every later record points at its parent.");

    head("4b. severities");
    srsp::audit::audit_log sev;
    for (auto s : {srsp::audit::severity::info, srsp::audit::severity::notice,
                   srsp::audit::severity::warning, srsp::audit::severity::critical}) {
        sev.append(s, "node-b", "event", srsp::audit::to_string(s));
    }
    for (const auto& r : sev.records()) {
        std::cout << "  level " << static_cast<int>(r.level) << " = "
                  << srsp::audit::to_string(r.level) << "\n";
    }
    srsp::audit::severity parsed = srsp::audit::severity::info;
    std::cout << "  parse_severity(\"warning\") = " << srsp::audit::parse_severity("warning", parsed)
              << " -> " << srsp::audit::to_string(parsed) << "\n";
    std::cout << "  parse_severity(\"nope\") = " << srsp::audit::parse_severity("nope", parsed)
              << "\n";

    head("4c. append_request, epoch and payload");
    srsp::audit::append_request req;
    req.level = srsp::audit::severity::critical;
    req.actor = "node-c";
    req.action = "revoke";
    req.detail = "key revoked after tamper";
    req.payload = bytes{0xde, 0xad, 0xbe, 0xef};
    req.epoch = 7;
    const auto stored = sev.append(req);
    std::cout << "  stored index=" << stored->index << " epoch=" << stored->epoch
              << " payload=" << stored->payload.size() << "B\n";
    std::cout << "  at(0) present=" << sev.at(0).has_value() << " at(99) present="
              << sev.at(99).has_value() << "\n";

    head("4d. keyed mode and hash_len");
    srsp::audit::audit_log keyed;
    keyed.set_key(secret("log-key"));
    keyed.append(srsp::audit::severity::notice, "node-a", "join", "keyed record");
    std::cout << "  keyed=" << keyed.keyed() << " verify=" << keyed.verify().ok << "\n";
    keyed.tamper_detail(0, "rewritten");
    std::cout << "  after tamper, break kind = " << srsp::audit::to_string(keyed.verify().kind)
              << " (the attacker cannot recompute the HMAC)\n";
    note("Unkeyed logs report broken_hash instead: anyone can recompute a plain hash.");

    srsp::audit::options short_hash;
    short_hash.hash_len = 16;
    srsp::audit::audit_log truncated(short_hash);
    truncated.append(srsp::audit::severity::info, "a", "b", "c");
    std::cout << "  hash_len=16 -> stored " << truncated.head_hash().size() << " bytes, verify="
              << truncated.verify().ok << "\n";
    srsp::audit::options no_payload;
    no_payload.include_payload = false;
    srsp::audit::audit_log blind(no_payload);
    blind.append(srsp::audit::severity::info, "a", "b", "c");
    std::cout << "  include_payload=false -> verify=" << blind.verify().ok
              << " (payload edits become invisible to the chain)\n";

    head("4e. max_records");
    srsp::audit::options capped;
    capped.max_records = 2;
    srsp::audit::audit_log small(capped);
    small.append(srsp::audit::severity::info, "a", "b", "one");
    small.append(srsp::audit::severity::info, "a", "b", "two");
    const auto third = small.append(srsp::audit::severity::info, "a", "b", "three");
    std::cout << "  max_records=2 -> size=" << small.size() << ", third append stored = "
              << third.has_value() << "\n";
    note("A full log returns nullopt; it never silently drops the oldest record.");

    head("4f. tamper detection, one mutation at a time");
    auto build = []() {
        srsp::audit::audit_log l;
        for (int i = 0; i < 5; ++i) {
            l.append(srsp::audit::severity::info, "node-a", "op", "d" + std::to_string(i));
        }
        return l;
    };
    {
        auto l = build();
        l.tamper_detail(2, "forged");
        const auto rep = l.verify();
        std::cout << "  tamper_detail(2) : ok=" << rep.ok << " bad_index=" << rep.bad_index << " "
                  << srsp::audit::to_string(rep.kind) << "\n";
    }
    {
        auto l = build();
        l.tamper_payload(1, bytes{1, 2, 3});
        const auto rep = l.verify();
        std::cout << "  tamper_payload(1): ok=" << rep.ok << " bad_index=" << rep.bad_index << " "
                  << srsp::audit::to_string(rep.kind) << "\n";
    }
    {
        auto l = build();
        l.drop_record(1);
        const auto rep = l.verify();
        std::cout << "  drop_record(1)   : ok=" << rep.ok << " bad_index=" << rep.bad_index << " "
                  << srsp::audit::to_string(rep.kind) << "\n";
    }
    {
        auto l = build();
        l.swap_records(1, 3);
        const auto rep = l.verify();
        std::cout << "  swap_records(1,3): ok=" << rep.ok << " bad_index=" << rep.bad_index << " "
                  << srsp::audit::to_string(rep.kind) << "\n";
    }
    note("Deleting a record shifts every index, so the break surfaces at the gap.");
    {
        auto l = build();
        const bytes expected = l.head_hash();
        l.truncate_to(3);
        const auto rep = l.verify(expected);
        std::cout << "  truncate_to(3) with expected head : ok=" << rep.ok << " "
                  << srsp::audit::to_string(rep.kind) << "\n";
    }
    note("verify(expected_head) is how a replica notices it fell behind or was rolled back.");
    {
        auto l = build();
        l.truncate_to(2);
        const auto rep = l.verify();
        std::cout << "  truncate_to(2) without expected head : ok=" << rep.ok
                  << " (a shorter chain is still self consistent)\n";
    }

    head("4g. merkle root and inclusion proofs");
    auto l = build();
    const bytes root = l.merkle_root();
    std::cout << "  merkle_root = " << srsp::hash::to_hex(root).substr(0, 24) << "...\n";
    for (std::size_t i = 0; i < l.size(); ++i) {
        const auto proof = l.inclusion_proof(i);
        const bool ok =
            srsp::audit::audit_log::verify_inclusion(l.at(i)->hash, proof, root, i, l.size());
        std::cout << "  record " << i << " proof depth " << proof.size() << " verifies = " << ok
                  << "\n";
    }
    const auto wrong = srsp::audit::audit_log::verify_inclusion(l.at(0)->hash, l.inclusion_proof(3),
                                                               root, 3, l.size());
    std::cout << "  wrong leaf against a real proof = " << wrong << "\n";
    note("A client that only knows the 32 byte root can check one record without the log.");

    head("4h. serialize and deserialize");
    const std::string text = l.serialize();
    std::cout << "  serialized bytes = " << text.size() << ", first line = "
              << text.substr(0, text.find('\n')) << "\n";
    srsp::audit::audit_log restored;
    const bool ok = srsp::audit::audit_log::deserialize(text, restored);
    std::cout << "  deserialize -> " << ok << ", size=" << restored.size()
              << ", verify=" << restored.verify().ok << "\n";
    std::cout << "  heads match = " << (restored.head_hex() == l.head_hex()) << "\n";
    std::cout << "  deserialize(\"junk\") = " << srsp::audit::audit_log::deserialize("junk", restored)
              << "\n";
    srsp::audit::audit_log keyed_src;
    keyed_src.set_key(secret("log-key"));
    keyed_src.append(srsp::audit::severity::info, "a", "b", "c");
    srsp::audit::audit_log keyed_back;
    srsp::audit::audit_log::deserialize(keyed_src.serialize(), keyed_back, secret("log-key"));
    std::cout << "  keyed round trip verify = " << keyed_back.verify().ok
              << " (the key is supplied, never stored)\n";

    head("4i. truncate_to and break kinds");
    std::cout << "  break kinds:";
    for (auto k : {srsp::audit::break_kind::none, srsp::audit::break_kind::bad_index,
                   srsp::audit::break_kind::broken_prev, srsp::audit::break_kind::broken_hash,
                   srsp::audit::break_kind::bad_mac, srsp::audit::break_kind::truncated}) {
        std::cout << " " << srsp::audit::to_string(k);
    }
    std::cout << "\n";
    std::cout << "  truncate_to(99) = " << l.truncate_to(99) << " (cannot grow a log)\n";
    srsp::audit::audit_log empty_log;
    std::cout << "  empty log verify = " << empty_log.verify().ok << ", head = "
              << empty_log.head_hex() << "\n";
    note("An empty log is vacuously valid; its head is all zero bytes.");
}


// ---------------------------------------------------------------------------
void section_consensus() {
    head("5. consensus - quorum voting");
    srsp::consensus::config cfg;
    std::cout << "  # config fields and defaults:\n"
              << "  ring_size              = " << cfg.ring_size << "\n"
              << "  quorum                 = " << cfg.quorum << "\n"
              << "  deadline               = " << cfg.deadline << " ticks\n"
              << "  reject_on_impossible   = " << (cfg.reject_on_impossible ? 1 : 0) << "\n";
    srsp::consensus::engine engine(cfg);
    std::string error;
    std::cout << "  # states: pending accepted committed rejected\n";
    engine.propose("a", "payload-a", 0, &error);
    std::cout << "  propose(a)             -> " << srsp::consensus::to_string(engine.find("a")->state) << "\n";
    engine.cast_vote("a", "n1", srsp::consensus::vote_kind::accept, 1, &error);
    std::cout << "  vote accept n1         -> " << srsp::consensus::to_string(engine.find("a")->state)
              << " (needs " << engine.settings().quorum << ")\n";
    engine.cast_vote("a", "n1", srsp::consensus::vote_kind::accept, 2, &error);
    std::cout << "  duplicate vote n1      -> refused: " << error << "\n";
    engine.cast_vote("a", "n2", srsp::consensus::vote_kind::accept, 3, &error);
    std::cout << "  vote accept n2         -> " << srsp::consensus::to_string(engine.find("a")->state) << "\n";
    error.clear();
    engine.propose("b", "payload-b", 4, &error);
    engine.cast_vote("b", "n1", srsp::consensus::vote_kind::reject, 5, &error);
    engine.cast_vote("b", "n2", srsp::consensus::vote_kind::reject, 6, &error);
    const auto* tb = engine.find("b");
    std::cout << "  two rejects of three   -> " << srsp::consensus::to_string(tb->state)
              << " (" << srsp::consensus::to_string(tb->reason) << ")\n";
    std::cout << "  # a proposal that cannot reach quorum is rejected early, not at the deadline\n";
    engine.propose("c", "payload-c", 7, &error);
    engine.cast_vote("c", "n1", srsp::consensus::vote_kind::abstain, 8, &error);
    std::cout << "  abstain leaves it      -> " << srsp::consensus::to_string(engine.find("c")->state) << "\n";
    engine.sweep(7 + cfg.deadline);
    std::cout << "  sweep at deadline      -> " << srsp::consensus::to_string(engine.find("c")->state)
              << " (" << srsp::consensus::to_string(engine.find("c")->reason) << ")\n";
    std::cout << "  committed=" << engine.committed() << " rejected=" << engine.rejected()
              << " votes=" << engine.votes_cast() << "\n";
    std::cout << "  # reject reasons: " << srsp::consensus::to_string(srsp::consensus::reject_reason::quorum_lost)
              << ", " << srsp::consensus::to_string(srsp::consensus::reject_reason::deadline)
              << ", " << srsp::consensus::to_string(srsp::consensus::reject_reason::duplicate_vote) << "\n";
}

// ---------------------------------------------------------------------------
void section_locks() {
    head("6. spinlock - leases and versions");
    srsp::locks::config cfg;
    std::cout << "  default_lease = " << cfg.default_lease << " ticks, max_lease = " << cfg.max_lease
              << ", reclaim_on_inspect = " << (cfg.reclaim_on_inspect ? 1 : 0) << "\n";
    srsp::locks::table locks(cfg);
    std::cout << "  # acquire results: " << srsp::locks::to_string(srsp::locks::acquire_result::acquired)
              << ", " << srsp::locks::to_string(srsp::locks::acquire_result::busy) << ", "
              << srsp::locks::to_string(srsp::locks::acquire_result::expired_reclaimed) << ", "
              << srsp::locks::to_string(srsp::locks::acquire_result::lease_too_long) << "\n";
    std::cout << "  acquire            -> " << srsp::locks::to_string(locks.acquire("res", "s1", "n1", 0)) << "\n";
    std::cout << "  acquire same tick  -> " << srsp::locks::to_string(locks.acquire("res", "s2", "n2", 0)) << "\n";
    std::cout << "  lease over max     -> "
              << srsp::locks::to_string(locks.acquire("res2", "s2", "n2", 0, cfg.max_lease + 1)) << "\n";
    std::cout << "  empty resource     -> "
              << srsp::locks::to_string(locks.acquire("", "s1", "n1", 0)) << "\n";
    std::cout << "  refresh owner      -> " << (locks.refresh("res", "s1", 100) ? "ok" : "refused")
              << " (expiry now " << (locks.inspect("res", 100)->lease_expiry) << ")\n";
    std::cout << "  release non-owner  -> " << (locks.release("res", "s2", 110) ? "ok" : "refused") << "\n";
    std::cout << "  acquire after lease-> " << srsp::locks::to_string(locks.acquire("res", "s2", "n2", 40000))
              << " (version " << locks.version_of("res") << ")\n";
    std::cout << "  # versions are monotonic per resource, so a stale holder sees a bump\n";
    std::cout << "  expire sweep       -> dropped " << locks.expire(100000) << "\n";
    const auto& c = locks.counters();
    std::cout << "  counters: acquired=" << c.acquired << " refused=" << c.refused
              << " refreshed=" << c.refreshed << " released=" << c.released
              << " expired=" << c.expired << "\n";
}

// ---------------------------------------------------------------------------
void section_store() {
    head("7. store - versions, scan, watch, batch");
    srsp::store::kv_store db;
    std::size_t fired = 0;
    const auto token = db.watch("k1", [&fired](const std::string&, const std::string&, bool) { ++fired; });
    db.put("k1", "one", 1);
    db.put("k2", "two", 2);
    db.put("k3", "three", 3);
    std::string v;
    std::uint64_t ver = 0;
    db.get("k1", &v, &ver);
    std::cout << "  put/get k1         -> " << v << " version " << ver << "\n";
    db.put("k1", "one-again", 4);
    db.get("k1", &v, &ver);
    std::cout << "  overwrite k1       -> " << v << " version " << ver << " (monotonic)\n";
    std::cout << "  watch fired        -> " << fired << " time(s) for the watched key only\n";
    std::cout << "  unwatch            -> " << (db.unwatch(token) ? "ok" : "missing")
              << ", watchers left " << db.watchers("k1") << "\n";
    const auto all = db.scan("k", 10);
    std::cout << "  scan(\"k\",10)       ->";
    for (const auto& kv : all) std::cout << " " << kv.first << "@" << kv.second.version;
    std::cout << "\n";
    const auto two = db.scan("k", 2);
    std::cout << "  scan limit 2       -> " << two.size() << " entry\n";
    std::string error;
    const bool rejected = db.batch({{srsp::store::op_kind::remove, "nope", ""}}, 5, &error);
    std::cout << "  batch remove nope  -> " << (rejected ? "applied" : "rejected") << ": " << error << "\n";
    std::cout << "  size unchanged     -> " << db.size() << "\n";
    const bool applied = db.batch({{srsp::store::op_kind::put, "k4", "four"},
                                   {srsp::store::op_kind::remove, "k2", ""}},
                                  6, &error);
    std::cout << "  batch put+remove   -> " << (applied ? "applied" : "rejected")
              << ", size " << db.size() << ", batches " << db.counters().batches << "\n";
    std::cout << "  # a batch is validated against a scratch key set, so it is all or nothing\n";
}
}  // namespace

int main() {
    std::cout << "Secure Ring State Protocol - every option, explained\n";
    section_hash();
    section_state_machine();
    section_handshake();
    section_key_rotation();
    section_audit();
    section_consensus();
    section_locks();
    section_store();
    std::cout << "\nteaching complete\n";
    return 0;
}

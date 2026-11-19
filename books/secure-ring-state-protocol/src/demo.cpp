// MIT License - (c) Anthony Matarazzo 2026
//
// demo.cpp - a short tour of all four SRSP algorithms.
//
// Every section prints a label before it acts so the output reads as a
// narrative rather than a wall of numbers.

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

namespace {

using srsp::handshake::bytes;

void banner(const std::string& title) {
    std::cout << "\n== " << title << " ==\n";
}

bytes secret_from(const std::string& label) {
    return srsp::handshake::derive_key_material("srsp-demo", bytes(label.begin(), label.end()), 32);
}

// 1 -------------------------------------------------------------------------
void demo_state_machine() {
    banner("ring state machine");
    srsp::ring::ring_machine machine;
    std::cout << "start state        : " << srsp::ring::to_string(machine.state()) << "\n";
    std::cout << "installed edges    : " << machine.transition_count() << "\n";

    // A context where everything a joining node needs is true.
    srsp::ring::guard_context ctx;
    ctx.peers_reachable = 3;
    ctx.peers_expected = 3;
    ctx.quorum_votes = 3;
    ctx.quorum_required = 2;

    const std::size_t moved = machine.advance(ctx);
    std::cout << "advance() moved    : " << moved << " states\n";
    for (const auto& h : machine.history()) {
        std::cout << "  seq " << h.seq << "  " << srsp::ring::to_string(h.from) << " -> "
                  << srsp::ring::to_string(h.to) << "  guard=" << h.guard << "\n";
    }

    // Asking for a state the table forbids is rejected, not guessed.
    auto skip = machine.step(srsp::ring::ring_state::probing, ctx);
    std::cout << "active -> probing  : " << (skip.ok ? "allowed" : "refused")
              << " (" << skip.reason << ")\n";

    // Losing quorum walks the node into degraded, then draining on request.
    srsp::ring::guard_context thin = ctx;
    thin.quorum_votes = 1;
    auto deg = machine.step(srsp::ring::ring_state::degraded, thin);
    std::cout << "quorum lost        : " << (deg.ok ? "degraded" : "stayed active")
              << " via guard " << deg.guard << "\n";

    srsp::ring::guard_context drain = thin;
    drain.drain_requested = true;
    drain.drain_complete = true;
    auto drain_step = machine.step(srsp::ring::ring_state::draining, drain);
    auto seal_step = machine.step(srsp::ring::ring_state::sealed, drain);
    std::cout << "drain then seal    : " << srsp::ring::to_string(machine.state())
              << " (guards " << drain_step.guard << ", " << seal_step.guard << ")\n";

    std::cout << "rejected attempts  : " << machine.rejected() << "\n";
}

// 2 -------------------------------------------------------------------------
void demo_handshake() {
    banner("challenge response handshake");
    const auto out = srsp::handshake::self_test("node-a", "node-b");
    std::cout << "outcome            : " << (out.ok ? "established" : "failed") << "\n";
    std::cout << "detail             : " << out.detail << "\n";
    std::cout << "messages           : " << out.messages << "\n";
    std::cout << "bytes on wire      : " << out.bytes_on_wire << "\n";

    // The same two identities again, so the session key can be shown.
    srsp::handshake::identity a{"node-a", secret_from("node-a")};
    srsp::handshake::identity b{"node-b", secret_from("node-b")};
    srsp::handshake::endpoint ea(a);
    srsp::handshake::endpoint eb(b);
    ea.trust_peer("node-b", b.secret);
    eb.trust_peer("node-a", a.secret);
    const auto run = srsp::handshake::run_handshake(ea, eb);
    const auto session = ea.established_session();
    std::cout << "session id         : " << session->session_id << "\n";
    std::cout << "keys agree         : "
              << (session->key_hex() == eb.established_session()->key_hex() ? "yes" : "no")
              << "\n";

    // Post handshake traffic is sealed with the session key.
    const bytes plaintext{'r', 'i', 'n', 'g', '-', 'p', 'a', 'y', 'l', 'o', 'a', 'd'};
    const bytes sealed = session->seal(plaintext);
    bytes recovered;
    const bool opened = session->open(sealed, recovered);
    std::cout << "sealed length      : " << sealed.size() << " bytes for "
              << plaintext.size() << " bytes of body (genetic_cipher envelope)\n";
    std::cout << "round trip         : " << (opened ? "ok" : "broken") << "\n";

    // An impostor that never held the trusted key cannot finish the handshake.
    srsp::handshake::identity mallory{"node-m", secret_from("mallory")};
    srsp::handshake::endpoint em(mallory);
    srsp::handshake::error_code err = srsp::handshake::error_code::none;
    const auto hello = em.make_hello("node-b");
    const auto rejected = eb.handle_hello(hello, err);
    std::cout << "impostor hello     : " << (rejected ? "accepted" : "rejected")
              << " (" << srsp::handshake::to_string(err) << ")\n";
    (void)run;
}

// 3 -------------------------------------------------------------------------
void demo_key_rotation() {
    banner("epoch key rotation");
    srsp::keys::config cfg;
    cfg.mode = srsp::keys::policy::fixed_epoch;
    cfg.rotate_after_ops = 4;
    srsp::keys::rotation_schedule schedule(cfg, secret_from("ring-master"));
    std::cout << "policy             : " << srsp::keys::to_string(cfg.mode)
              << " every " << cfg.rotate_after_ops << " ops\n";
    std::cout << "start epoch        : " << schedule.current_epoch() << " ("
              << schedule.current().fingerprint() << ")\n";

    for (int op = 1; op <= 10; ++op) {
        const auto v = schedule.record_operation();
        std::cout << "  op " << op << "  epoch " << v.epoch << "  "
                  << srsp::keys::to_string(v.kind) << "\n";
    }
    std::cout << "rotations          : " << schedule.rotations() << "\n";
    std::cout << "acceptance window  : " << schedule.acceptance_window() << " epoch(s)\n";

    const auto stale = schedule.check(1);
    std::cout << "check(1)           : " << (stale.ok ? "accepted" : "rejected") << " - "
              << stale.reason << "\n";
    const auto fresh = schedule.check(schedule.current_epoch());
    std::cout << "check(current)     : " << (fresh.ok ? "accepted" : "rejected") << "\n";

    // A wider policy keeps a few epochs usable during a rolling restart.
    srsp::keys::config sliding;
    sliding.mode = srsp::keys::policy::sliding_window;
    sliding.overlap_epochs = 2;
    srsp::keys::rotation_schedule wide(sliding, secret_from("ring-master"));
    wide.force_rotate(1);
    wide.force_rotate(1);
    std::cout << "sliding window     : " << wide.acceptance_window() << " epochs at epoch "
              << wide.current_epoch() << "\n";
}

// 4 -------------------------------------------------------------------------
void demo_audit_log() {
    banner("tamper evident audit log");
    srsp::audit::audit_log log;
    log.set_key(secret_from("log-key"));

    const char* actions[] = {"join", "sync", "lock", "unlock", "rekey"};
    for (int i = 0; i < 5; ++i) {
        log.tick(3);
        log.append(srsp::audit::severity::notice, "node-a", actions[i],
                   std::string("step ") + std::to_string(i));
    }
    auto rep = log.verify();
    std::cout << "records            : " << log.size() << "\n";
    std::cout << "keyed              : " << (log.keyed() ? "yes" : "no") << "\n";
    std::cout << "chain              : " << rep.reason << "\n";
    std::cout << "head hash          : " << log.head_hex().substr(0, 24) << "...\n";

    // Edit a record behind the log's back. The hash chain notices.
    log.tamper_detail(2, "step 2 edited by hand");
    const auto broken = log.verify();
    std::cout << "after tamper       : " << (broken.ok ? "still valid" : "detected") << "\n";
    std::cout << "first bad record   : " << broken.bad_index << " ("
              << srsp::audit::to_string(broken.kind) << ")\n";

    // Cutting back to the last good prefix restores a verifiable chain.
    log.truncate_to(broken.bad_index);
    std::cout << "truncated to       : " << log.size() << " records, chain "
              << (log.verify().ok ? "valid again" : "still broken") << "\n";
}


// ---------------------------------------------------------------------------
// Quorum consensus
// ---------------------------------------------------------------------------
void demo_consensus() {
    std::cout << "\n== quorum consensus ==\n";
    srsp::consensus::config cfg;
    cfg.ring_size = 5;
    cfg.quorum = 3;
    cfg.deadline = 100;
    srsp::consensus::engine engine(cfg);
    std::string error;

    engine.propose("tx-1", "balance:ada=100", 0, &error);
    engine.cast_vote("tx-1", "node-a", srsp::consensus::vote_kind::accept, 1, &error);
    engine.cast_vote("tx-1", "node-b", srsp::consensus::vote_kind::accept, 2, &error);
    std::cout << "  tx-1 after 2 accepts : " << srsp::consensus::to_string(engine.find("tx-1")->state) << "\n";
    engine.cast_vote("tx-1", "node-c", srsp::consensus::vote_kind::accept, 3, &error);
    const auto* t1 = engine.find("tx-1");
    std::cout << "  tx-1 after 3 accepts : " << srsp::consensus::to_string(t1->state)
              << " at tick " << t1->decided_at << "\n";

    engine.propose("tx-2", "balance:bob=0", 10, &error);
    engine.cast_vote("tx-2", "node-a", srsp::consensus::vote_kind::reject, 11, &error);
    engine.cast_vote("tx-2", "node-b", srsp::consensus::vote_kind::reject, 12, &error);
    engine.cast_vote("tx-2", "node-c", srsp::consensus::vote_kind::reject, 13, &error);
    const auto* t2 = engine.find("tx-2");
    std::cout << "  tx-2 after 3 rejects : " << srsp::consensus::to_string(t2->state)
              << " (" << srsp::consensus::to_string(t2->reason) << ")\n";

    engine.propose("tx-3", "balance:cy=5", 20, &error);
    engine.cast_vote("tx-3", "node-a", srsp::consensus::vote_kind::accept, 21, &error);
    engine.cast_vote("tx-3", "node-b", srsp::consensus::vote_kind::abstain, 22, &error);
    const std::size_t expired = engine.sweep(200);
    const auto* t3 = engine.find("tx-3");
    std::cout << "  tx-3 swept at 200   : " << srsp::consensus::to_string(t3->state)
              << " (" << srsp::consensus::to_string(t3->reason) << "), expired=" << expired << "\n";
    std::cout << "  committed=" << engine.committed() << " rejected=" << engine.rejected()
              << " pending=" << engine.pending() << " votes=" << engine.votes_cast() << "\n";
}

// ---------------------------------------------------------------------------
// Lease locks
// ---------------------------------------------------------------------------
void demo_locks() {
    std::cout << "\n== lease locks ==\n";
    srsp::locks::config cfg;
    cfg.default_lease = 100;
    cfg.max_lease = 400;
    srsp::locks::table locks(cfg);

    const auto first = locks.acquire("db/users", "sess-1", "node-a", 0);
    std::cout << "  acquire by sess-1   : " << srsp::locks::to_string(first) << "\n";
    const auto second = locks.acquire("db/users", "sess-2", "node-b", 10);
    std::cout << "  acquire by sess-2   : " << srsp::locks::to_string(second) << "\n";
    std::cout << "  refresh sess-1      : " << (locks.refresh("db/users", "sess-1", 50) ? "ok" : "refused") << "\n";
    std::cout << "  release by sess-2   : " << (locks.release("db/users", "sess-2", 60) ? "ok" : "refused") << "\n";
    const auto* info = locks.inspect("db/users", 120);
    std::cout << "  holder at 120       : " << (info ? info->session : "free")
              << " version=" << (info ? info->version : 0) << "\n";
    const auto third = locks.acquire("db/users", "sess-2", "node-b", 200);
    std::cout << "  acquire at 200      : " << srsp::locks::to_string(third)
              << " (version " << locks.version_of("db/users") << ")\n";
    std::cout << "  held now            : " << locks.held_count(210)
              << ", expired total " << locks.counters().expired << "\n";
}

// ---------------------------------------------------------------------------
// Versioned store
// ---------------------------------------------------------------------------
void demo_store() {
    std::cout << "\n== versioned store ==\n";
    srsp::store::kv_store db;
    std::vector<std::string> events;
    db.watch("user:1", [&events](const std::string& key, const std::string& value, bool deleted) {
        events.push_back(key + (deleted ? " deleted" : " -> " + value));
    });

    db.put("user:1", "ada", 10);
    db.put("user:2", "grace", 11);
    db.put("user:3", "alan", 12);
    std::string value;
    std::uint64_t version = 0;
    db.get("user:2", &value, &version);
    std::cout << "  get user:2          : " << value << " (version " << version << ")\n";

    const auto page = db.scan("user:", 2);
    std::cout << "  scan user: limit 2  :";
    for (const auto& kv : page) std::cout << " " << kv.first;
    std::cout << "\n";

    db.remove("user:1", 20);
    std::string error;
    const bool bad = db.batch({{srsp::store::op_kind::put, "user:9", "linus"},
                               {srsp::store::op_kind::remove, "user:404", ""}},
                              30, &error);
    std::cout << "  batch with bad key  : " << (bad ? "applied" : "rejected") << " (" << error << ")\n";
    const bool good = db.batch({{srsp::store::op_kind::put, "user:4", "linus"},
                                {srsp::store::op_kind::remove, "user:3", ""}},
                               31, &error);
    std::cout << "  valid batch         : " << (good ? "applied" : "rejected")
              << ", size now " << db.size() << "\n";
    std::cout << "  watch events        : " << events.size();
    for (const auto& e : events) std::cout << " [" << e << "]";
    std::cout << "\n";
}
}  // namespace

int main() {
    std::cout << "Secure Ring State Protocol - demo\n";
    demo_state_machine();
    demo_handshake();
    demo_key_rotation();
    demo_audit_log();
    demo_consensus();
    demo_locks();
    demo_store();
    std::cout << "\ndemo complete\n";
    return 0;
}

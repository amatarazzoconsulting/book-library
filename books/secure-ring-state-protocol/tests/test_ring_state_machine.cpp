// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>

#include "ring_state_machine.hpp"

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}
srsp::ring::guard_context healthy() {
    srsp::ring::guard_context ctx;
    ctx.quorum_votes = 3;
    ctx.quorum_required = 2;
    ctx.peers_reachable = 3;
    ctx.peers_expected = 3;
    return ctx;
}
}  // namespace

int main() {
    using srsp::ring::ring_state;
    check(srsp::ring::state_count() == 9, "nine states are declared");
    ring_state parsed = ring_state::boot;
    check(srsp::ring::parse_state("syncing", parsed) && parsed == ring_state::syncing, "parse_state round trips");
    check(!srsp::ring::parse_state("nonsense", parsed), "parse_state rejects junk");
    check(std::string(srsp::ring::to_string(ring_state::degraded)) == "degraded", "to_string names a state");

    srsp::ring::ring_machine m;
    check(m.state() == ring_state::boot, "starts in boot");
    check(m.transition_count() == 19, "default table installs 19 edges");

    auto ctx = healthy();
    check(!m.step(ring_state::active, ctx).ok, "cannot jump straight to active");
    check(m.state() == ring_state::boot, "state unchanged after a refused edge");

    check(m.advance(ctx) == 4, "advance walks boot to active in four steps");
    check(m.state() == ring_state::active, "reached active");
    check(m.seq() == 4, "sequence advanced with each transition");
    check(m.history().size() == 4, "history records each transition");

    ctx.quorum_votes = 0;
    ctx.peers_reachable = 1;
    check(m.step(ring_state::degraded, ctx).ok, "quorum loss allows active to degraded");

    ctx = healthy();
    ctx.drain_requested = true;
    ctx.drain_complete = true;
    check(m.step(ring_state::draining, ctx).ok, "drain_requested opens draining");
    check(m.step(ring_state::sealed, ctx).ok, "drain_complete seals the ring");

    ctx.rekey_ready = true;
    ctx.rekey_ready = true;
    ctx.epoch = m.epoch();
    check(!m.step(ring_state::probing, ctx).ok, "sealed refuses a stale epoch");
    ctx.epoch = m.epoch() + 1;
    check(m.step(ring_state::probing, ctx).ok, "sealed reopens on an advanced epoch");
    check(m.epoch() == ctx.epoch, "the machine adopts the advanced epoch");

    check(m.can_reach(ring_state::sealed), "sealed is reachable");
    check(!m.path_to(ring_state::active).empty(), "a path to active exists");
    check(!m.visit_counts().empty(), "visit counts are reported");

    m.reset();
    check(m.state() == ring_state::boot && m.seq() == 0, "reset returns to boot");

    std::cout << (failures == 0 ? "all ring state machine tests passed\n" : "ring state machine failures\n");
    return failures == 0 ? 0 : 1;
}

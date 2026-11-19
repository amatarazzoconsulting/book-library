// MIT License - (c) Anthony Matarazzo 2026
// Transition throughput and the cost of a refused edge.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

#include "ring_state_machine.hpp"

namespace {
using Clock = std::chrono::steady_clock;
double seconds(Clock::duration d) { return std::chrono::duration<double>(d).count(); }
using namespace srsp::ring;
}  // namespace

int main() {
    ring_machine m;
    guard_context ctx;
    ctx.quorum_votes = 3;
    ctx.quorum_required = 2;
    ctx.peers_reachable = 3;
    ctx.peers_expected = 3;

    const int cycles = 20000;
    std::size_t transitions = 0;
    const auto t0 = Clock::now();
    for (int i = 0; i < cycles; ++i) {
        m.reset();
        transitions += m.advance(ctx);
        ctx.drain_requested = true;
        ctx.drain_complete = true;
        m.step(ring_state::draining, ctx);
        m.step(ring_state::sealed, ctx);
        ctx.drain_requested = false;
        ctx.drain_complete = false;
        ctx.epoch = m.epoch() + 1;
        m.step(ring_state::probing, ctx);
    }
    const auto t1 = Clock::now();

    const int refusals = 200000;
    ring_machine r;
    const auto t2 = Clock::now();
    std::size_t rejected = 0;
    for (int i = 0; i < refusals; ++i) {
        if (!r.step(ring_state::active, ctx).ok) ++rejected;
    }
    const auto t3 = Clock::now();

    std::cout << std::left << std::setw(30) << "metric" << std::setw(16) << "value" << "\n"
              << std::string(46, '-') << "\n"
              << std::setw(30) << "lifecycle cycles" << std::setw(16) << cycles << "\n"
              << std::setw(30) << "transitions" << std::setw(16) << transitions << "\n"
              << std::setw(30) << "transitions per second" << std::setw(16) << std::fixed
              << std::setprecision(0) << (transitions / seconds(t1 - t0)) << "\n"
              << std::setw(30) << "refused edges per second" << std::setw(16)
              << (refusals / seconds(t3 - t2)) << "\n"
              << std::setw(30) << "refusals counted" << std::setw(16) << rejected << "\n";
    return 0;
}

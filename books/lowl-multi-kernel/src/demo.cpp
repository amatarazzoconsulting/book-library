// MIT License - (c) Anthony Matarazzo 2026
//
// Short program that exercises every algorithm in this book's library.
#include <cstdio>
#include <vector>

#include "capability.hpp"
#include "kernel_adapter.hpp"
#include "scheduler.hpp"
#include "syscall_map.hpp"

using namespace lowl::kal;

namespace {

void section(const char* title) {
    std::printf("\n=== %s ===\n", title);
}

void demo_kernel_adapter() {
    section("1. kernel adapter layer");
    const KernelId ids[] = {KernelId::Linux, KernelId::FreeBsd, KernelId::WindowsNt,
                            KernelId::Os2};
    for (KernelId id : ids) {
        auto a = make_adapter(id);
        std::printf("%-12s %s\n", a->name(), a->abi());
        std::printf("  probe=%d open=%s cost=%.0fns fork=%.0fns async=%s\n",
                    a->probe_score(), status_name(a->call(Op::Open).status),
                    a->profile(Op::Open).cost_ns, a->profile(Op::Fork).cost_ns,
                    a->supports(Op::AsyncIo) ? (a->is_emulated(Op::AsyncIo) ? "emulated"
                                                                           : "native")
                                             : "unsupported");
        a->call(Op::Close);
    }
    auto chosen = select_adapter({KernelId::Os2, KernelId::FreeBsd, KernelId::Linux});
    std::printf("selected adapter: %s (highest probe score)\n", chosen->name());
}

void demo_syscall_map() {
    section("2. syscall translation map");
    SyscallMap map;
    const Op ops[] = {Op::Read, Op::Open, Op::Fork, Op::AsyncIo};
    for (Op op : ops) {
        std::printf("%-14s", op_name(op));
        for (KernelId k : {KernelId::Linux, KernelId::FreeBsd, KernelId::WindowsNt,
                           KernelId::Os2}) {
            Translation t = map.translate(op, k);
            if (t.found) {
                std::printf(" %s=%u(%s)", kernel_name(k), t.entry.number,
                            t.entry.emulated ? "emu" : "nat");
            } else {
                std::printf(" %s=-", kernel_name(k));
            }
        }
        std::printf("\n");
    }
    std::printf("Windows NT numbers 7 shared by: ");
    for (Op op : map.reverse(KernelId::WindowsNt, 7)) std::printf("%s ", op_name(op));
    std::printf("\n");
    Translation r = map.remap(Op::Fork, KernelId::Linux, KernelId::WindowsNt);
    std::printf("remap fork linux->nt: %s number=%u\n", r.entry.native_name, r.entry.number);
    std::printf("%s", map.table(KernelId::Linux).c_str());
}

void demo_capability() {
    section("3. capability query and fallback");
    for (KernelId k : {KernelId::Linux, KernelId::FreeBsd, KernelId::WindowsNt,
                       KernelId::Os2}) {
        CapabilityProfile p = profile_for(k);
        CapabilityPlanner planner(p);
        planner.set_chain(Capability::Sandbox, default_sandbox_chain());
        Plan plan = planner.plan(Capability::Sandbox);
        std::printf("%-12s native=%zu emulated=%zu missing=%zu sandbox -> %s (%s, %zu hops)\n",
                    kernel_name(k), p.native_count(), p.emulated_count(), p.missing_count(),
                    plan.satisfied ? capability_name(plan.chosen) : "none",
                    plan.reason.c_str(), plan.hops());
    }
}

void demo_scheduler() {
    section("4. scheduler policy adapter");
    const KernelId ids[] = {KernelId::Linux, KernelId::FreeBsd, KernelId::WindowsNt,
                            KernelId::Os2};
    for (KernelId id : ids) {
        auto a = make_adapter(id);
        SchedOptions o;
        o.policy = Policy::FairShare;
        Scheduler s(a.get(), o);
        s.add_task(0, 1.0);
        s.add_task(0, 2.0);
        s.add_task(0, 3.0);
        s.run(600);
        std::printf("%-12s fair_share runs=%lu/%lu/%lu spread=%lu kernel_cost=%.0fns\n",
                    a->name(), s.runs(0), s.runs(1), s.runs(2), s.spread(),
                    s.stats().kernel_cost_ns);
    }
    SchedOptions g;
    g.policy = Policy::Gang;
    Scheduler gang(nullptr, g);
    for (unsigned i = 0; i < 6; ++i) gang.add_task(0, 1.0, i / 2);
    gang.run(60);
    std::printf("gang dispatch over 3 gangs: switches=%lu\n", gang.stats().gang_switches);
}

}  // namespace

int main() {
    std::printf("LOWL Multi Kernel - companion demo\n");
    demo_kernel_adapter();
    demo_syscall_map();
    demo_capability();
    demo_scheduler();
    std::printf("\ndone\n");
    return 0;
}

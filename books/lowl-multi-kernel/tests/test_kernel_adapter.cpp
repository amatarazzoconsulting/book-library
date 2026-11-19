// MIT License - (c) Anthony Matarazzo 2026
// Tests for the kernel adapter layer. Prints PASS lines, exits non-zero on failure.
#include <cmath>
#include <cstdio>
#include <vector>

#include "kernel_adapter.hpp"

using namespace lowl::kal;

static int failures = 0;

static void check(bool ok, const char* what) {
    if (ok) {
        std::printf("PASS %s\n", what);
    } else {
        std::printf("FAIL %s\n", what);
        ++failures;
    }
}

static bool near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) <= eps; }

int main() {
    // Normal: each adapter builds and reports its identity.
    auto linux = make_adapter(KernelId::Linux);
    auto bsd = make_adapter(KernelId::FreeBsd);
    auto nt = make_adapter(KernelId::WindowsNt);
    auto os2 = make_adapter(KernelId::Os2);
    check(linux && bsd && nt && os2, "all four adapters construct");
    check(std::string(linux->name()) == "Linux", "linux adapter name");
    check(linux->probe_score() == 100 && os2->probe_score() == 10,
          "probe scores are ordered linux > os2");

    // Normal: costs match the book's ratio table against the Linux baseline.
    check(near(bsd->ratio_vs(*linux, Op::Open), 1.1), "freebsd open ratio 1.1");
    check(near(nt->ratio_vs(*linux, Op::Fork), 850.0), "windows fork ratio 850");
    check(near(os2->ratio_vs(*linux, Op::SemAcquire), 8.0), "os2 semaphore ratio 8");
    check(near(os2->ratio_vs(*linux, Op::SignalDeliver), 25.0), "os2 signal ratio 25");
    check(near(bsd->ratio_vs(*linux, Op::AsyncIo), 2.5), "freebsd async ratio 2.5");
    check(near(linux->ratio_vs(*linux, Op::Read), 1.0), "linux is its own baseline");

    // Normal: capability flags.
    check(!bsd->is_emulated(Op::Open) && bsd->is_emulated(Op::AsyncIo),
          "freebsd async_io is emulated, open is native");
    check(!os2->supports(Op::AsyncIo), "os2 reports async_io unsupported");
    check(linux->supports(Op::AsyncIo), "linux reports async_io supported");

    // Normal: dispatch returns status and accumulates stats.
    linux->reset_stats();
    CallResult r = linux->call(Op::Open);
    check(r.status == Status::Ok && r.value == 1, "open returns handle 1");
    check(linux->open_handles() == 1, "one handle tracked");
    r = linux->call(Op::Write);
    check(r.status == Status::Ok && r.value == 4096, "write on open handle succeeds");
    r = linux->call(Op::Close);
    check(r.status == Status::Ok && linux->open_handles() == 0, "close releases handle");
    check(linux->stats().calls == 3, "three calls recorded");
    check(near(linux->stats().total_cost_ns,
               linux->profile(Op::Open).cost_ns + linux->profile(Op::Write).cost_ns +
                   linux->profile(Op::Close).cost_ns),
          "total cost is the sum of per-op costs");

    // Edge: close without open is a bad handle and counts as a failure.
    linux->reset_stats();
    r = linux->call(Op::Close);
    check(r.status == Status::BadHandle, "close without open is bad_handle");
    check(linux->stats().failures == 1, "failure counted");
    check(linux->open_handles() == 0, "handle count did not go negative");

    // Edge: read on no handle.
    check(linux->call(Op::Read).status == Status::BadHandle, "read without handle fails");

    // Edge: unsupported op reports Unsupported and no cost.
    os2->reset_stats();
    r = os2->call(Op::AsyncIo);
    check(r.status == Status::Unsupported, "os2 async_io unsupported at dispatch");
    check(near(r.cost_ns, 0.0), "unsupported op charges no cost");
    check(os2->stats().unsupported == 1 && os2->stats().emulated == 0,
          "unsupported counted, emulated not");

    // Edge: handle limit enforced at the configured boundary.
    struct LimitedAdapter : KernelAdapter {
        LimitedAdapter() : KernelAdapter(KernelId::Linux, 2) {
            set(Op::Open, 100.0, 2);
            set(Op::Close, 100.0, 3);
        }
        const char* name() const override { return "Limited"; }
        const char* abi() const override { return "test"; }
        int probe_score() const override { return 0; }
    };
    LimitedAdapter lim;
    check(lim.call(Op::Open).status == Status::Ok, "first open under limit ok");
    check(lim.call(Op::Open).status == Status::Ok, "second open under limit ok");
    check(lim.call(Op::Open).status == Status::LimitReached, "third open hits the limit");
    check(lim.open_handles() == 2, "handle count stops at the limit");
    check(lim.handle_limit() == 2, "configured limit is reported");
    check(linux->handle_limit() == 1024, "default handle limit is 1024");

    // Edge: null and unknown inputs.
    check(make_adapter(KernelId::Unknown) == nullptr, "unknown kernel makes no adapter");
    check(select_adapter({}) == nullptr, "empty candidate list selects nothing");
    auto picked = select_adapter({KernelId::Os2, KernelId::Linux, KernelId::FreeBsd});
    check(picked && picked->id() == KernelId::Linux, "selection picks the highest probe");
    check(select_adapter({KernelId::Unknown, KernelId::Os2})->id() == KernelId::Os2,
          "unknown candidates are skipped");

    // Edge: select_best ignores null entries.
    std::vector<KernelAdapter*> ptrs{nullptr, nt.get(), bsd.get()};
    check(select_best(ptrs) == bsd.get(), "select_best skips nulls and picks freebsd");

    // Edge: ratio against a zero-cost baseline is defined as 0.
    check(near(os2->ratio_vs(*os2, Op::AsyncIo), 0.0),
          "ratio against unsupported baseline is 0");

    // Edge: describe is stable and contains the probe score.
    check(linux->describe().find("probe=100") != std::string::npos,
          "describe contains probe score");

    std::printf(failures == 0 ? "all kernel_adapter tests passed\n"
                              : "kernel_adapter tests FAILED\n");
    return failures == 0 ? 0 : 1;
}

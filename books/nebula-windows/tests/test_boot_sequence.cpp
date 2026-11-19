// MIT License - (c) Anthony Matarazzo 2026
//
// test_boot_sequence.cpp - normal and edge cases for the boot phase state machine.
//
// The executor is always a lambda returning a fixed Outcome, so no assertion here
// depends on wall clock time. Timing is simulated on purpose: the state machine is
// what is under test, not the machine running it.

#include <cstdio>
#include <string>
#include <vector>

#include "boot_sequence.hpp"

using namespace nebula::boot;

namespace {

int failures = 0;

void check(bool cond, const char* what) {
    if (cond) {
        std::printf("PASS %s\n", what);
    } else {
        std::printf("FAIL %s\n", what);
        ++failures;
    }
}

void check_eq(long long got, long long want, const char* what) {
    if (got == want) {
        std::printf("PASS %s (%lld)\n", what, got);
    } else {
        std::printf("FAIL %s: got %lld want %lld\n", what, got, want);
        ++failures;
    }
}

void check_near(double got, double want, double tol, const char* what) {
    double d = got - want;
    if (d < 0.0) d = -d;
    if (d <= tol) {
        std::printf("PASS %s (%.3f)\n", what, got);
    } else {
        std::printf("FAIL %s: got %.3f want %.3f\n", what, got, want);
        ++failures;
    }
}

Outcome fixed(bool ok, double ms) { return Outcome{ok, ms, "fixed"}; }

} // namespace

int main() {
    std::printf("== test_boot_sequence\n");

    // ---- the seven phase default plan -------------------------------------
    BootPlan plan = BootPlan::nebula_default();
    check_eq(static_cast<long long>(plan.size()), 7, "default plan has seven phases");
    Validation v = plan.validate();
    check(v.ok, "default plan validates");
    check_eq(static_cast<long long>(v.errors.size()), 0, "default plan has no errors");

    auto order = plan.order();
    check(order.has_value(), "default plan is orderable");
    if (order) {
        check_eq(static_cast<long long>(order->size()), 7, "order covers every phase");
        check((*order)[0] == PhaseId::Firmware, "firmware runs first");
        check((*order)[6] == PhaseId::UserSpace, "user space runs last");
    }

    // The declared budgets sum to 1.40 ms and form a single chain, so the
    // critical path equals the sum.
    check_near(plan.critical_path_ms(), 1.40, 1e-9, "critical path of default plan");

    // Steps transcribed from the book: phase 5 has eight, phase 7 has five.
    const Phase* dd = plan.find(PhaseId::DeviceDiscovery);
    check(dd != nullptr, "device discovery phase exists");
    if (dd) check_eq(static_cast<long long>(dd->steps.size()), 8, "device discovery has 8 steps");
    const Phase* us = plan.find(PhaseId::UserSpace);
    if (us) check_eq(static_cast<long long>(us->steps.size()), 5, "user space has 5 steps");
    check(plan.find(PhaseId::SystemServices) != nullptr, "system services phase exists");

    // ---- add() rejects bad phases ----------------------------------------
    BootPlan bad;
    std::string err;
    Phase no_name;
    no_name.id = PhaseId::Firmware;
    no_name.budget_ms = 0.1;
    check(!bad.add(no_name, &err), "empty name rejected");
    check(err.find("empty name") != std::string::npos, "empty name error mentions the reason");

    Phase negative;
    negative.id = PhaseId::Firmware;
    negative.name = "Firmware";
    negative.budget_ms = -0.5;
    err.clear();
    check(!bad.add(negative, &err), "negative budget rejected");
    check(err.find("negative budget") != std::string::npos, "negative budget error is explicit");

    Phase good;
    good.id = PhaseId::Firmware;
    good.name = "Firmware";
    good.budget_ms = 0.2;
    check(bad.add(good, &err), "valid phase accepted");
    err.clear();
    check(!bad.add(good, &err), "duplicate id rejected");
    check(err.find("duplicate") != std::string::npos, "duplicate error is explicit");
    check(bad.add(Phase{PhaseId::Bootloader, "B", {}, 0.0, false, {}}, nullptr),
          "add without an error pointer is allowed");

    // ---- validation catches self dependency, unknown dependency, cycles ---
    BootPlan self_dep;
    self_dep.add(Phase{PhaseId::Firmware, "fw", {PhaseId::Firmware}, 0.1, false, {}});
    Validation sv = self_dep.validate();
    check(!sv.ok, "self dependency fails validation");
    check(!self_dep.order().has_value(), "self dependency cannot be ordered");

    BootPlan unknown_dep;
    unknown_dep.add(Phase{PhaseId::Firmware, "fw", {PhaseId::UserSpace}, 0.1, false, {}});
    Validation uv = unknown_dep.validate();
    check(!uv.ok, "unknown dependency fails validation");
    check(uv.errors[0].find("unknown phase") != std::string::npos,
          "unknown dependency error names the phase");

    BootPlan cycle;
    cycle.add(Phase{PhaseId::Firmware, "fw", {PhaseId::UserSpace}, 0.1, false, {}});
    cycle.add(Phase{PhaseId::UserSpace, "us", {PhaseId::Firmware}, 0.1, false, {}});
    Validation cv = cycle.validate();
    check(!cv.ok, "two phase cycle fails validation");
    check(!cycle.order().has_value(), "two phase cycle cannot be ordered");

    // A cycle is reported by run(), never executed.
    RunReport cycled = BootMachine(cycle).run(nullptr);
    check(!cycled.error.empty(), "cycle run reports a plan error");
    check_eq(static_cast<long long>(cycled.phases.size()), 0, "cycle run executes no phase");
    check(!cycled.completed, "cycle run is not completed");
    check(cycled.pcr.empty(), "cycle run has no measurement");

    // ---- a clean run ------------------------------------------------------
    BootMachine machine(plan);
    RunReport ok = machine.run([](const Phase& p) { return fixed(true, p.budget_ms * 0.5); });
    check(ok.completed, "clean run completes");
    check_eq(ok.done, 7, "clean run completes all seven phases");
    check_eq(ok.failed, 0, "clean run has no failures");
    check_eq(ok.blocked, 0, "clean run has no blocked phases");
    check(ok.within_budget, "clean run stays within budget");
    check_near(ok.total_ms, 0.70, 1e-9, "clean run total is half the budget");
    check_near(ok.budget_ms, 1.40, 1e-9, "clean run budget sum");
    check_eq(static_cast<long long>(ok.pcr.size()), 16, "pcr is sixteen hex characters");

    // Determinism: same outcomes give the same measurement chain.
    RunReport again = machine.run([](const Phase& p) { return fixed(true, p.budget_ms * 0.5); });
    check(ok.pcr == again.pcr, "identical runs produce identical pcr");
    RunReport shifted = machine.run([](const Phase& p) { return fixed(true, p.budget_ms * 0.6); });
    check(shifted.pcr != ok.pcr, "different timings produce a different pcr");

    // ---- budget overrun --------------------------------------------------
    RunReport over = machine.run([](const Phase& p) { return fixed(true, p.budget_ms + 0.01); });
    check(!over.within_budget, "overrun is flagged");
    check_eq(over.failed, 0, "overrun is not a failure by default");
    bool any_flagged = false;
    for (const auto& r : over.phases) any_flagged = any_flagged || r.over_budget;
    check(any_flagged, "at least one phase is marked over budget");

    RunOptions strict;
    strict.fail_on_over_budget = true;
    RunReport strict_run = machine.run(
        [](const Phase& p) { return fixed(true, p.budget_ms + 0.01); }, strict);
    check_eq(strict_run.failed, 1, "strict options turn the first overrun into a failure");
    check_eq(static_cast<long long>(strict_run.phases.size()), 1,
             "halt_on_failure stops after the first failure");

    // budget_scale stretches the budget, so the same timing now fits.
    RunOptions scaled;
    scaled.budget_scale = 2.0;
    RunReport scaled_run = machine.run([](const Phase&) { return fixed(true, 0.20); }, scaled);
    check_near(scaled_run.budget_ms, 2.80, 1e-9, "budget scale doubles the budget");
    check(scaled_run.within_budget, "scaled budget accepts the same timings");

    // ---- failure handling ------------------------------------------------
    auto fail_kernel = [](const Phase& p) {
        return fixed(p.id != PhaseId::KernelCore, 0.05);
    };
    RunReport halted = machine.run(fail_kernel);
    check_eq(halted.failed, 1, "one phase failed");
    check_eq(static_cast<long long>(halted.phases.size()), 3, "halt stops at kernel core");
    check(!halted.completed, "halted run is not completed");

    RunOptions keep_going;
    keep_going.halt_on_failure = false;
    RunReport continued = machine.run(fail_kernel, keep_going);
    check_eq(static_cast<long long>(continued.phases.size()), 7, "no halt visits every phase");
    check_eq(continued.failed, 1, "still exactly one failure");
    check_eq(continued.blocked, 4, "everything after the failure is blocked");
    check(!continued.completed, "run with blocked phases is not completed");

    // ---- optional phases -------------------------------------------------
    BootPlan optional;
    optional.add(Phase{PhaseId::Firmware, "fw", {}, 0.1, false, {}});
    optional.add(Phase{PhaseId::Bootloader, "bl", {PhaseId::Firmware}, 0.1, true, {}});
    optional.add(Phase{PhaseId::KernelCore, "kc", {PhaseId::Bootloader}, 0.1, false, {}});
    auto fail_fw = [](const Phase& p) { return fixed(p.id != PhaseId::Firmware, 0.05); };

    RunOptions skip_opts;
    skip_opts.halt_on_failure = false;
    RunReport skipped = BootMachine(optional).run(fail_fw, skip_opts);
    check_eq(skipped.skipped, 1, "optional phase is skipped when its dependency dies");
    check_eq(skipped.blocked, 0, "optional skip avoids a block");

    RunOptions no_skip;
    no_skip.halt_on_failure = false;
    no_skip.skip_optional_on_block = false;
    RunReport blocked_run = BootMachine(optional).run(fail_fw, no_skip);
    check_eq(blocked_run.skipped, 0, "skip policy can be disabled");
    check_eq(blocked_run.blocked, 2, "both dependants block when skipping is off");

    // ---- null executor ---------------------------------------------------
    RunReport no_exec = machine.run(nullptr);
    check(no_exec.completed, "a null executor still completes the plan");
    check_near(no_exec.total_ms, 1.40, 1e-9, "null executor charges the full budget");
    check(no_exec.phases[0].detail == "no executor", "null executor is reported in the detail");

    // ---- formatting ------------------------------------------------------
    std::string plan_text = format_plan(plan);
    check(plan_text.find("Device discovery") != std::string::npos, "plan table names phases");
    std::string report_text = format_report(ok);
    check(report_text.find("pcr=") != std::string::npos, "report prints the pcr");
    std::string error_text = format_report(cycled);
    check(error_text.find("plan error") != std::string::npos, "error report is labeled");

    check(std::string(to_string(PhaseId::DeviceDiscovery)) == "DeviceDiscovery",
          "phase id text");
    check(std::string(to_string(PhaseStatus::Blocked)) == "Blocked", "phase status text");

    if (failures == 0) {
        std::printf("all boot_sequence checks passed\n");
        return 0;
    }
    std::printf("%d boot_sequence checks FAILED\n", failures);
    return 1;
}

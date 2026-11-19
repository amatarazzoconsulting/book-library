// MIT License - (c) Anthony Matarazzo 2026
//
// demo.cpp - short labeled tour of all four algorithms.
//
// Every simulated timing here is a fixed constant, never a clock reading, so the
// printed output is identical on every machine and can be diffed in the lessons.

#include <cstdio>
#include <string>
#include <vector>

#include "boot_sequence.hpp"
#include "driver_inventory.hpp"
#include "driver_load_planner.hpp"
#include "profile_coverage.hpp"

using namespace nebula;

namespace {

// Simulated phase cost: the book's budgets with a little variance, fixed.
double simulated_ms(boot::PhaseId id) {
    switch (id) {
        case boot::PhaseId::Firmware:        return 0.180;
        case boot::PhaseId::Bootloader:      return 0.140;
        case boot::PhaseId::KernelCore:      return 0.195;
        case boot::PhaseId::Subsystems:      return 0.160;
        case boot::PhaseId::DeviceDiscovery: return 0.520;
        case boot::PhaseId::SystemServices:  return 0.095;
        case boot::PhaseId::UserSpace:       return 0.110;
    }
    return 0.0;
}

void section(const char* title) {
    std::printf("\n=== %s ===\n", title);
}

} // namespace

int main() {
    section("1 boot plan");
    boot::BootPlan plan = boot::BootPlan::nebula_default();
    std::printf("%s", boot::format_plan(plan).c_str());
    boot::Validation v = plan.validate();
    std::printf("valid=%s phases=%zu critical_path=%.2f ms\n", v.ok ? "yes" : "no", plan.size(),
                plan.critical_path_ms());

    section("2 boot run");
    boot::BootMachine machine(plan);
    boot::RunReport report = machine.run([](const boot::Phase& p) {
        return boot::Outcome{true, simulated_ms(p.id), "simulated"};
    });
    std::printf("%s", boot::format_report(report).c_str());

    section("3 driver catalog");
    drivers::DriverRegistry reg = drivers::DriverRegistry::book_catalog();
    std::printf("catalog size=%zu\n", reg.size());
    std::printf("%s", drivers::format_catalog(reg).c_str());

    section("4 capability matching");
    std::vector<drivers::HardwareDevice> sample = {
        {"GeForce RTX 4090", "PCI\\VEN_10DE&DEV_2684", drivers::DeviceClass::Display,
         {"d3d12", "raytracing", "dlss"}},
        {"Unknown NVMe", "PCI\\CC_010802", drivers::DeviceClass::Storage, {"nvme", "trim"}},
        {"Mystery card", "PCI\\VEN_DEAD&DEV_0001", drivers::DeviceClass::Display, {"d3d12"}},
        {"Radeon RX 7900", "PCI\\VEN_1002&DEV_744C", drivers::DeviceClass::Display,
         {"d3d12", "raytracing"}},
    };
    for (const auto& dev : sample) {
        drivers::MatchResult m = reg.match(dev);
        std::printf("  %s\n", drivers::format_match(dev, m).c_str());
    }

    section("5 load planner");
    drivers::DriverLoadPlanner planner(drivers::DriverLoadPlanner::profile_a_requests());
    std::vector<drivers::Conflict> conflicts = planner.detect_conflicts();
    std::printf("conflicts:\n%s", drivers::format_conflicts(conflicts).c_str());
    drivers::PlannerOptions popts;
    popts.resolve_conflicts = true;
    drivers::LoadPlan lplan = planner.plan(popts);
    std::printf("%s", drivers::format_plan(lplan).c_str());

    section("6 profile coverage");
    drivers::CoverageAnalyzer analyzer(reg);
    std::vector<drivers::ProfileReport> reports = analyzer.analyze_all();
    std::printf("%s", drivers::format_coverage_table(reports).c_str());

    section("7 one profile in detail");
    std::printf("%s", drivers::format_profile_report(reports.front()).c_str());

    section("8 shared resources and a cycle");
    std::vector<drivers::LoadRequest> bad = {
        {"alpha", "alpha.sys", 10.0, {"beta"}, {"vmd:0"}, {}, 5, true},
        {"beta", "beta.sys", 10.0, {"alpha"}, {}, {}, 5, true},
    };
    drivers::DriverLoadPlanner cyc(bad);
    drivers::LoadPlan cplan = cyc.plan();
    std::printf("%s", drivers::format_conflicts(cyc.detect_conflicts()).c_str());
    std::printf("%s", drivers::format_plan(cplan).c_str());

    std::printf("\ndemo complete\n");
    return 0;
}

// MIT License - (c) Anthony Matarazzo 2026
//
// boot_sequence.cpp - implementation of the NEBULA boot phase state machine.

#include "boot_sequence.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <set>
#include <sstream>

namespace nebula::boot {
namespace {

// Deterministic 64 bit FNV-1a. Used for the PCR style measurement chain so the
// same run always produces the same digest on every platform.
std::uint64_t fnv1a(const std::string& text, std::uint64_t seed) {
    std::uint64_t h = seed;
    for (unsigned char c : text) {
        h ^= static_cast<std::uint64_t>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

std::string hex16(std::uint64_t v) {
    char buf[17];
    std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(v));
    return std::string(buf);
}

std::string fmt_ms(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.3f", v);
    return std::string(buf);
}

} // namespace

const char* to_string(PhaseId id) {
    switch (id) {
        case PhaseId::Firmware:        return "Firmware";
        case PhaseId::Bootloader:      return "Bootloader";
        case PhaseId::KernelCore:      return "KernelCore";
        case PhaseId::Subsystems:      return "Subsystems";
        case PhaseId::DeviceDiscovery: return "DeviceDiscovery";
        case PhaseId::SystemServices:  return "SystemServices";
        case PhaseId::UserSpace:       return "UserSpace";
    }
    return "Unknown";
}

const char* to_string(PhaseStatus status) {
    switch (status) {
        case PhaseStatus::Pending: return "Pending";
        case PhaseStatus::Running: return "Running";
        case PhaseStatus::Done:    return "Done";
        case PhaseStatus::Skipped: return "Skipped";
        case PhaseStatus::Failed:  return "Failed";
        case PhaseStatus::Blocked: return "Blocked";
    }
    return "Unknown";
}

bool BootPlan::add(Phase phase, std::string* error) {
    if (phase.name.empty()) {
        if (error) *error = "phase " + std::string(to_string(phase.id)) + " has an empty name";
        return false;
    }
    if (phase.budget_ms < 0.0) {
        if (error) *error = "phase " + phase.name + " has a negative budget";
        return false;
    }
    for (const auto& existing : phases_) {
        if (existing.id == phase.id) {
            if (error) *error = "duplicate phase id " + std::string(to_string(phase.id));
            return false;
        }
    }
    phases_.push_back(std::move(phase));
    return true;
}

const Phase* BootPlan::find(PhaseId id) const {
    for (const auto& p : phases_) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

Validation BootPlan::validate() const {
    Validation v;
    std::set<PhaseId> known;
    for (const auto& p : phases_) known.insert(p.id);

    for (const auto& p : phases_) {
        for (PhaseId dep : p.requires) {
            if (dep == p.id) {
                v.errors.push_back(p.name + " requires itself");
            } else if (known.count(dep) == 0) {
                v.errors.push_back(p.name + " requires unknown phase " + to_string(dep));
            }
        }
    }

    // Cycle detection with the same deterministic order used by order().
    std::map<PhaseId, int> state; // 0 unvisited, 1 on stack, 2 done
    std::function<bool(PhaseId)> visit = [&](PhaseId id) -> bool {
        int& s = state[id];
        if (s == 1) return false; // back edge
        if (s == 2) return true;
        s = 1;
        if (const Phase* p = find(id)) {
            for (PhaseId dep : p->requires) {
                if (known.count(dep) == 0) continue;
                if (!visit(dep)) return false;
            }
        }
        s = 2;
        return true;
    };
    for (const auto& p : phases_) {
        if (!visit(p.id)) {
            v.errors.push_back("dependency cycle involving " + p.name);
        }
    }
    v.ok = v.errors.empty();
    return v;
}

std::optional<std::vector<PhaseId>> BootPlan::order() const {
    std::set<PhaseId> known;
    for (const auto& p : phases_) known.insert(p.id);
    for (const auto& p : phases_) {
        for (PhaseId dep : p.requires) {
            if (dep != p.id && known.count(dep) == 0) return std::nullopt;
        }
    }

    std::set<PhaseId> done;
    std::vector<PhaseId> result;
    std::vector<bool> taken(phases_.size(), false);

    while (result.size() < phases_.size()) {
        bool progressed = false;
        for (std::size_t i = 0; i < phases_.size(); ++i) {
            if (taken[i]) continue;
            const Phase& p = phases_[i];
            bool ready = true;
            for (PhaseId dep : p.requires) {
                if (done.count(dep) == 0) { ready = false; break; }
            }
            if (!ready) continue;
            taken[i] = true;
            done.insert(p.id);
            result.push_back(p.id);
            progressed = true;
            break; // declaration order wins; restart the scan
        }
        if (!progressed) return std::nullopt; // cycle
    }
    return result;
}

double BootPlan::critical_path_ms() const {
    std::map<PhaseId, double> memo;
    std::map<PhaseId, int> guard;

    std::function<double(PhaseId)> longest = [&](PhaseId id) -> double {
        auto hit = memo.find(id);
        if (hit != memo.end()) return hit->second;
        if (guard[id] == 1) return 0.0; // cycle guard, validate() reports it
        guard[id] = 1;
        const Phase* p = find(id);
        double best_dep = 0.0;
        if (p) {
            for (PhaseId dep : p->requires) {
                best_dep = std::max(best_dep, longest(dep));
            }
        }
        double total = best_dep + (p ? p->budget_ms : 0.0);
        guard[id] = 2;
        memo[id] = total;
        return total;
    };

    double best = 0.0;
    for (const auto& p : phases_) best = std::max(best, longest(p.id));
    return best;
}

BootPlan BootPlan::nebula_default() {
    BootPlan plan;
    plan.add(Phase{PhaseId::Firmware, "Firmware", {}, 0.20, false,
                   {"reset vector 0xFFFFFFF0", "firmware self test", "enumerate boot devices",
                    "load nebula-boot.efi from ESP", "verify bootloader signature"}});
    plan.add(Phase{PhaseId::Bootloader, "Bootloader", {PhaseId::Firmware}, 0.15, false,
                   {"enter long mode", "identity map first 2GB", "load nebula.kernel",
                    "load initrd with driver shim", "jump to kernel entry"}});
    plan.add(Phase{PhaseId::KernelCore, "Kernel core", {PhaseId::Bootloader}, 0.20, false,
                   {"install IDT and GDT", "enable paging", "start APs", "create ObjectManager",
                    "create root ObjectDirectory", "init MemoryManager"}});
    plan.add(Phase{PhaseId::Subsystems, "Subsystems", {PhaseId::KernelCore}, 0.15, false,
                   {"scheduler and idle task", "interrupt controller", "timer and clock",
                    "DeviceTree from ACPI", "DriverManager probe queue",
                    "WindowsDriverRegistryMapDB", "SecureDriverStorage seal check"}});
    plan.add(Phase{PhaseId::DeviceDiscovery, "Device discovery",
                   {PhaseId::Subsystems}, 0.50, false,
                   {"PCI enumeration", "ACPI namespace walk", "match driver by hardware ID",
                    "verify SHA3-384 integrity", "check TPM seal", "decrypt and load driver",
                    "register with DeviceTree", "mount root filesystem"}});
    plan.add(Phase{PhaseId::SystemServices, "System services", {PhaseId::DeviceDiscovery}, 0.10, false,
                   {"WindowsBinaryShim", "AutomatedDriverResolver", "NetworkManager",
                    "DesktopManager", "IntegrityMonitor", "TelemetryService", "AuditLog"}});
    plan.add(Phase{PhaseId::UserSpace, "User space", {PhaseId::SystemServices}, 0.10, false,
                   {"create init task PID 1", "run /system/init.nebula", "start login manager",
                    "launch services", "present desktop"}});
    return plan;
}

BootMachine::BootMachine(BootPlan plan) : plan_(std::move(plan)) {}

RunReport BootMachine::run(const Executor& exec, const RunOptions& opts) const {
    RunReport report;

    Validation v = plan_.validate();
    if (!v.ok) {
        report.error = v.errors.front();
        return report;
    }
    auto order = plan_.order();
    if (!order) {
        report.error = "plan cannot be ordered (cycle)";
        return report;
    }

    std::map<PhaseId, PhaseStatus> status;
    for (const auto& p : plan_.phases()) status[p.id] = PhaseStatus::Pending;

    for (PhaseId id : *order) {
        const Phase* p = plan_.find(id);
        if (!p) continue;

        PhaseResult r;
        r.id = p->id;
        r.name = p->name;
        r.budget_ms = p->budget_ms * opts.budget_scale;

        // Dependency gate.
        bool blocked = false;
        for (PhaseId dep : p->requires) {
            PhaseStatus ds = status[dep];
            if (ds != PhaseStatus::Done && ds != PhaseStatus::Skipped) blocked = true;
        }
        if (blocked) {
            if (p->optional && opts.skip_optional_on_block) {
                r.status = PhaseStatus::Skipped;
                r.detail = "optional, dependency unavailable";
                ++report.skipped;
            } else {
                r.status = PhaseStatus::Blocked;
                r.detail = "dependency not satisfied";
                ++report.blocked;
            }
            status[id] = r.status;
            report.phases.push_back(r);
            continue;
        }

        r.status = PhaseStatus::Running;
        Outcome o = exec ? exec(*p) : Outcome{true, r.budget_ms, "no executor"};
        r.elapsed_ms = o.elapsed_ms < 0.0 ? 0.0 : o.elapsed_ms;
        r.detail = o.detail;
        r.over_budget = r.budget_ms >= 0.0 && r.elapsed_ms > r.budget_ms;
        if (r.over_budget) report.within_budget = false;

        if (!o.ok || (opts.fail_on_over_budget && r.over_budget)) {
            r.status = PhaseStatus::Failed;
            ++report.failed;
            status[id] = r.status;
            report.phases.push_back(r);
            if (opts.halt_on_failure) break;
            continue;
        }

        r.status = PhaseStatus::Done;
        ++report.done;
        status[id] = r.status;
        report.phases.push_back(r);
    }

    for (auto& pr : report.phases) {
        report.total_ms += pr.elapsed_ms;
        report.budget_ms += pr.budget_ms;
    }
    report.critical_path_ms = plan_.critical_path_ms();

    std::uint64_t h = 1469598103934665603ULL; // FNV offset basis
    for (const auto& pr : report.phases) {
        std::string rec = std::string(to_string(pr.id)) + "|" + to_string(pr.status) + "|" +
                          fmt_ms(pr.elapsed_ms) + "|" + pr.detail;
        h = fnv1a(rec, h);
    }
    report.pcr = hex16(h);
    report.completed = (report.failed == 0 && report.blocked == 0 &&
                        report.phases.size() == plan_.size());
    return report;
}

std::string format_plan(const BootPlan& plan) {
    std::ostringstream os;
    os << "phase             budget_ms  requires         steps\n";
    for (const auto& p : plan.phases()) {
        std::string req;
        for (std::size_t i = 0; i < p.requires.size(); ++i) {
            if (i) req += ",";
            req += to_string(p.requires[i]);
        }
        if (req.empty()) req = "-";
        char line[160];
        std::snprintf(line, sizeof(line), "%-17s %9.2f  %-15s %zu\n",
                      p.name.c_str(), p.budget_ms, req.c_str(), p.steps.size());
        os << line;
    }
    return os.str();
}

std::string format_report(const RunReport& report) {
    std::ostringstream os;
    if (!report.error.empty()) {
        os << "plan error: " << report.error << "\n";
        return os.str();
    }
    os << "phase             status   elapsed_ms  budget_ms  flag\n";
    for (const auto& r : report.phases) {
        char line[160];
        std::snprintf(line, sizeof(line), "%-17s %-8s %10.3f  %9.2f  %s\n",
                      r.name.c_str(), to_string(r.status), r.elapsed_ms, r.budget_ms,
                      r.over_budget ? "OVER" : "");
        os << line;
    }
    char tail[256];
    std::snprintf(tail, sizeof(tail),
                  "done=%d skipped=%d failed=%d blocked=%d total=%.3f ms budget=%.2f ms "
                  "critical_path=%.2f ms within_budget=%s pcr=%s\n",
                  report.done, report.skipped, report.failed, report.blocked,
                  report.total_ms, report.budget_ms, report.critical_path_ms,
                  report.within_budget ? "yes" : "no", report.pcr.c_str());
    os << tail;
    return os.str();
}

} // namespace nebula::boot

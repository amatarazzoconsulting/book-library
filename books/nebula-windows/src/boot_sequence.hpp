// MIT License - (c) Anthony Matarazzo 2026
//
// boot_sequence.hpp - NEBULA boot sequence as an ordered phase state machine.
//
// The book describes seven boot phases (firmware through user space), each with
// a time budget and a fixed list of steps. Phases may not start until every
// phase they require has completed. This header models that contract:
//
//   BootPlan     declares phases, checks dependencies and cycles
//   BootMachine  executes phases through a caller supplied callback
//   RunReport    per phase status, budget accounting, PCR style measurement
//
// No real firmware, kernel or driver is touched. The executor is a plain
// std::function, so a test can simulate a slow or failing phase deterministically.

#ifndef NEBULA_BOOT_SEQUENCE_HPP
#define NEBULA_BOOT_SEQUENCE_HPP

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace nebula::boot {

enum class PhaseId {
    Firmware = 0,
    Bootloader,
    KernelCore,
    Subsystems,
    DeviceDiscovery,
    SystemServices,
    UserSpace
};

const char* to_string(PhaseId id);

enum class PhaseStatus { Pending, Running, Done, Skipped, Failed, Blocked };

const char* to_string(PhaseStatus status);

// One boot phase. "requires" lists phases that must be Done before this one runs.
struct Phase {
    PhaseId id{PhaseId::Firmware};
    std::string name;
    std::vector<PhaseId> requires;
    double budget_ms{0.0};
    bool optional{false};
    std::vector<std::string> steps;
};

struct Validation {
    bool ok{true};
    std::vector<std::string> errors;
};

// What the executor reports back for one phase.
struct Outcome {
    bool ok{true};
    double elapsed_ms{0.0};
    std::string detail;
};

struct PhaseResult {
    PhaseId id{PhaseId::Firmware};
    std::string name;
    PhaseStatus status{PhaseStatus::Pending};
    double elapsed_ms{0.0};
    double budget_ms{0.0};
    bool over_budget{false};
    std::string detail;
};

struct RunOptions {
    bool halt_on_failure{true};        // stop the machine at the first Failed phase
    bool skip_optional_on_block{true}; // optional phase with a dead dependency -> Skipped
    bool fail_on_over_budget{false};   // treat a budget overrun as a phase failure
    double budget_scale{1.0};          // stretch or shrink every budget (1.0 = as declared)
};

struct RunReport {
    std::vector<PhaseResult> phases;
    bool completed{false};
    double total_ms{0.0};
    double budget_ms{0.0};
    double critical_path_ms{0.0};
    std::string pcr;       // chained measurement of every phase outcome (FNV-1a)
    std::string error;     // set when the plan itself could not be ordered
    int done{0};
    int skipped{0};
    int failed{0};
    int blocked{0};
    bool within_budget{true};
};

class BootPlan {
public:
    // Returns false and fills *error for a duplicate id, empty name or negative budget.
    bool add(Phase phase, std::string* error = nullptr);

    // Duplicate ids, unknown dependencies, self dependencies, cycles.
    Validation validate() const;

    // Deterministic topological order: among ready phases the one declared first wins.
    // Returns std::nullopt when the plan has a cycle or an unknown dependency.
    std::optional<std::vector<PhaseId>> order() const;

    // Longest chain of declared budgets through the dependency graph.
    double critical_path_ms() const;

    const std::vector<Phase>& phases() const { return phases_; }
    const Phase* find(PhaseId id) const;
    std::size_t size() const { return phases_.size(); }

    // The seven phases and budgets printed in the book (0.2 / 0.15 / 0.2 / 0.15 / 0.5 / 0.1 / 0.1).
    static BootPlan nebula_default();

private:
    std::vector<Phase> phases_;
};

using Executor = std::function<Outcome(const Phase&)>;

class BootMachine {
public:
    explicit BootMachine(BootPlan plan);

    RunReport run(const Executor& exec, const RunOptions& opts = RunOptions()) const;

    const BootPlan& plan() const { return plan_; }

private:
    BootPlan plan_;
};

std::string format_plan(const BootPlan& plan);
std::string format_report(const RunReport& report);

} // namespace nebula::boot

#endif // NEBULA_BOOT_SEQUENCE_HPP

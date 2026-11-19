// MIT License - (c) Anthony Matarazzo 2026
//
// driver_load_planner.hpp - ordering the driver load queue.
//
// Phase 5 of the boot sequence loads matched drivers. Real drivers depend on
// each other (the NVIDIA display driver needs the chipset PCI driver; audio
// needs the HDA bus) and they compete for resources (one VMD controller cannot
// be owned by both iaStorVD and iaStorAC). This header turns a flat request
// list into an ordered load plan:
//
//   - dependencies are ordered by deterministic topological sort
//   - cycles are reported, not silently dropped
//   - two drivers claiming the same exclusive resource are a conflict
//   - explicit "conflicts_with" pairs are reported
//   - the plan is split into waves that could be loaded concurrently
//
// Load cost is modelled as a fixed per driver overhead plus size / throughput,
// so the planner can estimate wall time for each wave.

#ifndef NEBULA_LOAD_PLANNER_HPP
#define NEBULA_LOAD_PLANNER_HPP

#include <cstddef>
#include <string>
#include <vector>

namespace nebula::drivers {

struct LoadRequest {
    std::string driver_id;
    std::string binary;
    double size_mb{0.0};
    std::vector<std::string> requires;      // driver ids that must load first
    std::vector<std::string> claims;        // exclusive resources, e.g. "vmd:0"
    std::vector<std::string> conflicts_with;
    int priority{0};                        // higher priority wins a resource tie
    bool critical{true};                    // non critical drivers may be dropped
};

struct Conflict {
    enum class Kind { DuplicateId, MissingDependency, Cycle, Resource, Explicit, SelfDependency };
    Kind kind{Kind::Resource};
    std::string a;
    std::string b;
    std::string detail;
};

const char* to_string(Conflict::Kind k);

struct LoadStep {
    std::string driver_id;
    std::string binary;
    int wave{0};
    double est_ms{0.0};
    double size_mb{0.0};
    int priority{0};
    std::vector<std::string> requires;
};

struct LoadPlan {
    std::vector<LoadStep> steps;              // ordered, wave ascending
    std::vector<Conflict> conflicts;
    std::vector<std::string> dropped;         // non critical drivers removed by policy
    int waves{0};
    double est_total_ms{0.0};
    double est_critical_path_ms{0.0};
    double total_mb{0.0};
    bool loadable{false};
    std::string error;
};

struct PlannerOptions {
    double fixed_overhead_ms{2.0};   // per driver open, verify and create device object
    double throughput_mb_per_ms{3.3};  // decrypt and map speed, about 3.3 GB/s
    bool resolve_conflicts{false};   // keep the higher priority claimant, drop the other
    bool drop_non_critical{false};   // drop non critical drivers that cannot be ordered
    bool allow_parallel_waves{true}; // wave width affects wall time estimate
};

class DriverLoadPlanner {
public:
    explicit DriverLoadPlanner(std::vector<LoadRequest> requests);

    const std::vector<LoadRequest>& requests() const { return requests_; }

    // Static checks: duplicate ids, self dependencies, missing dependencies,
    // explicit conflicts and shared exclusive resources.
    std::vector<Conflict> detect_conflicts() const;

    LoadPlan plan(const PlannerOptions& opts = PlannerOptions()) const;

    // Estimated cost of a single driver under the given options.
    static double estimate_ms(const LoadRequest& r, const PlannerOptions& opts);

    // The eight driver requests of the book's Profile A.
    static std::vector<LoadRequest> profile_a_requests();

private:
    std::vector<LoadRequest> requests_;
};

std::string format_plan(const LoadPlan& plan);
std::string format_conflicts(const std::vector<Conflict>& conflicts);

} // namespace nebula::drivers

#endif // NEBULA_LOAD_PLANNER_HPP

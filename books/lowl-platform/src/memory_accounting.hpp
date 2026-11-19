// MIT License - (c) Anthony Matarazzo 2026
#ifndef MEMORY_ACCOUNTING_HPP
#define MEMORY_ACCOUNTING_HPP

#include <string>
#include <vector>

// Resident memory accounting for the numbers in "LOWL Platform" parts 3 and 5.
// Two separate questions are modelled: what an idle desktop costs, broken down by
// component, and how many applications fit in a RAM budget once per-process
// overhead is taken into account.
namespace lowl {

struct Component {
    std::string name;
    double mb = 0.0;
};

struct Footprint {
    std::string os;
    std::vector<Component> components;
};

// Sum of components. Used to check the book's own arithmetic.
double total_mb(const Footprint& f);

// Share of the total held by one component, 0.0 when the total is zero.
double component_share(const Footprint& f, const std::string& name);

// Idle desktop breakdown from part 3.1, one entry per operating system.
std::vector<Footprint> idle_footprints();

// Application RSS table from part 3.2.
struct AppMemory {
    std::string app;
    double windows_mb = 0.0;
    double macos_mb = 0.0;
    double ubuntu_mb = 0.0;
    double lowl_mb = 0.0;
};

std::vector<AppMemory> application_table();

// Mean RSS across the applications both operating systems report.
double mean_app_mb(const std::vector<AppMemory>& apps, const std::string& os);

// One point of the part 3.3 scaling table: N applications open, total RSS.
struct ScalingPoint {
    int apps = 0;
    double mb = 0.0;
};

std::vector<ScalingPoint> scaling_table(const std::string& os);

// Least squares fit of total_mb = intercept + slope * apps. A large positive
// intercept with a small slope means fixed system cost dominates; a slope near the
// measured per-application RSS means each process carries its own weight.
struct Fit {
    double intercept_mb = 0.0;
    double slope_mb = 0.0;
    double max_residual_mb = 0.0;
    double rms_residual_mb = 0.0;
};

Fit fit_scaling(const std::vector<ScalingPoint>& points);

// Capacity model. usable_mb is RAM after firmware and page cache reserve.
// Each running process costs its private working set plus a fixed per-process
// overhead: page tables, kernel task struct, and for dynamically linked systems
// the private copies of shared library data.
struct CapacityModel {
    std::string os;
    double system_mb = 0.0;
    double app_mb = 0.0;
    double per_process_overhead_mb = 0.0;
};

int capacity_apps(const CapacityModel& model, double ram_mb);

double ram_for_apps(const CapacityModel& model, int apps);

// Compressed heap: the Organic Codec keeps cold heap objects compressed at the
// stated ratio, so the resident cost of a heap of heap_mb is
// heap_mb * (1 - ratio) while the object is untouched.
double compressed_heap_mb(double heap_mb, double ratio);

// Blend a plain working set with a compressed portion.
struct HeapSplit {
    double hot_mb = 0.0;
    double cold_mb = 0.0;
    double ratio = 0.0;
};

double effective_rss_mb(double base_mb, const HeapSplit& split);

}  // namespace lowl

#endif  // MEMORY_ACCOUNTING_HPP

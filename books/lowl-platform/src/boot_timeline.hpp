// MIT License - (c) Anthony Matarazzo 2026
#ifndef BOOT_TIMELINE_HPP
#define BOOT_TIMELINE_HPP

#include <string>
#include <vector>

// Boot phase model from "LOWL Platform" part 2 and 6.1. Phases are a small DAG:
// power-on to desktop is the longest path through the phase graph, not the sum of
// the phases, because driver init and session startup overlap.
namespace lowl {

struct Phase {
    std::string name;
    double ms = 0.0;
    std::vector<int> after;  // indices of phases that must complete first
};

struct Plan {
    std::string name;
    std::vector<Phase> phases;
};

struct Timeline {
    std::vector<double> start_ms;
    std::vector<double> finish_ms;
    std::vector<int> critical_path;
    double total_ms = 0.0;
    double serial_ms = 0.0;
};

// Longest path through the phase DAG. Phase i may only depend on phases j < i,
// which lets one forward pass compute every start and finish time.
Timeline run_plan(const Plan& plan);

// LOWL in full detail, plus the competitor totals the book states. The book only
// splits competitor boots into POST, bootloader and everything else.
std::vector<Plan> reference_plans();

// Fraction of the total boot time that this phase alone accounts for. Only
// meaningful for phases on the critical path; parallel phases hide under it.
double phase_share(const Timeline& t, std::size_t index);

// How far a phase may slip before it moves the total. Zero on the critical path.
double slack_ms(const Timeline& t, std::size_t index);

bool on_critical_path(const Timeline& t, std::size_t index);

// Storage model: total = fixed compute + image bytes / bandwidth. The fixed part
// is calibrated from the NVMe measurement, so predictions for slower media show
// how much of the book's "storage agnostic" claim is bandwidth and how much is
// device enumeration that the model does not describe.
struct StoragePoint {
    std::string device;
    double bandwidth_mbs = 0.0;
    double measured_s = 0.0;
};

std::vector<StoragePoint> lowl_storage_points();
std::vector<StoragePoint> competitor_storage_points();

struct StoragePrediction {
    std::string device;
    double measured_ms = 0.0;
    double predicted_ms = 0.0;
    double residual_ms = 0.0;
    double bandwidth_ms = 0.0;  // pure transfer time for the image
};

// image_mb is the AIR kernel plus initramfs size, under 50 MB per the book.
std::vector<StoragePrediction> predict_storage(const std::vector<StoragePoint>& points,
                                               double image_mb);

double speedup(double baseline_ms, double candidate_ms);

}  // namespace lowl

#endif  // BOOT_TIMELINE_HPP

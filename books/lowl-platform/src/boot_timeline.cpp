// MIT License - (c) Anthony Matarazzo 2026
#include "boot_timeline.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace lowl {

Timeline run_plan(const Plan& plan) {
    Timeline t;
    const std::size_t n = plan.phases.size();
    t.start_ms.assign(n, 0.0);
    t.finish_ms.assign(n, 0.0);

    for (std::size_t i = 0; i < n; ++i) {
        const Phase& p = plan.phases[i];
        if (p.ms < 0.0) {
            throw std::invalid_argument("phase duration must not be negative");
        }
        double start = 0.0;
        for (int dep : p.after) {
            if (dep < 0 || static_cast<std::size_t>(dep) >= i) {
                // Forward only edges keep the single pass valid and catch cycles.
                throw std::invalid_argument("dependency must refer to an earlier phase");
            }
            start = std::max(start, t.finish_ms[static_cast<std::size_t>(dep)]);
        }
        t.start_ms[i] = start;
        t.finish_ms[i] = start + p.ms;
        t.serial_ms += p.ms;
    }

    t.total_ms = 0.0;
    for (double f : t.finish_ms) {
        t.total_ms = std::max(t.total_ms, f);
    }

    // Walk back from the phase that ends last, always stepping to the dependency
    // that finished at the current phase's start time.
    std::size_t cur = 0;
    bool found = false;
    for (std::size_t i = 0; i < n; ++i) {
        if (t.finish_ms[i] == t.total_ms) {
            cur = i;
            found = true;
        }
    }
    if (found) {
        for (;;) {
            t.critical_path.push_back(static_cast<int>(cur));
            int next = -1;
            for (int dep : plan.phases[cur].after) {
                if (t.finish_ms[static_cast<std::size_t>(dep)] == t.start_ms[cur]) {
                    next = dep;
                }
            }
            if (next < 0) {
                break;
            }
            cur = static_cast<std::size_t>(next);
        }
        std::reverse(t.critical_path.begin(), t.critical_path.end());
    }
    return t;
}

std::vector<Plan> reference_plans() {
    // LOWL phase split. POST 300 ms and bootloader 150 ms are stated in part 2;
    // the remaining 460 ms is distributed across kernel load, kernel init, driver
    // init, session and desktop load so the critical path lands on the stated
    // 0.91 s total. Driver init runs in parallel with kernel init plus session.
    Plan lowl;
    lowl.name = "LOWL 1.0";
    lowl.phases = {
        {"BIOS/UEFI POST", 300.0, {}},
        {"Bootloader", 150.0, {0}},
        {"Kernel loading", 110.0, {1}},
        {"Kernel init", 80.0, {2}},
        {"Driver init", 150.0, {2}},
        {"User session startup", 130.0, {3}},
        {"Desktop environment load", 140.0, {4, 5}},
    };

    // Competitors: the book states POST, bootloader and the total only.
    auto simple = [](const std::string& name, double post, double loader, double total) {
        Plan p;
        p.name = name;
        p.phases = {
            {"BIOS/UEFI POST", post, {}},
            {"Bootloader", loader, {0}},
            {"Kernel, drivers, session, desktop", total - post - loader, {1}},
        };
        return p;
    };

    return {
        lowl,
        simple("Windows 11", 3200.0, 1500.0, 8300.0),
        simple("macOS Sonoma", 4500.0, 2800.0, 12100.0),
        simple("Ubuntu 24.04", 3200.0, 800.0, 5100.0),
        simple("ChromeOS Flex", 3200.0, 500.0, 4500.0),
    };
}

double phase_share(const Timeline& t, std::size_t index) {
    if (index >= t.finish_ms.size() || t.total_ms <= 0.0) {
        return 0.0;
    }
    if (!on_critical_path(t, index)) {
        return 0.0;
    }
    return (t.finish_ms[index] - t.start_ms[index]) / t.total_ms;
}

double slack_ms(const Timeline& t, std::size_t index) {
    if (index >= t.finish_ms.size()) {
        return 0.0;
    }
    return t.total_ms - t.finish_ms[index];
}

bool on_critical_path(const Timeline& t, std::size_t index) {
    for (int i : t.critical_path) {
        if (static_cast<std::size_t>(i) == index) {
            return true;
        }
    }
    return false;
}

std::vector<StoragePoint> lowl_storage_points() {
    // Part 6.1, LOWL column, seconds from power-on to desktop.
    return {
        {"NVMe SSD", 3500.0, 0.91},
        {"SATA SSD", 550.0, 1.20},
        {"HDD 7200 RPM", 150.0, 2.10},
        {"USB 3.0 flash", 100.0, 2.80},
    };
}

std::vector<StoragePoint> competitor_storage_points() {
    // Part 6.1, Ubuntu column, used as the comparison baseline.
    return {
        {"NVMe SSD", 3500.0, 5.10},
        {"SATA SSD", 550.0, 7.80},
        {"HDD 7200 RPM", 150.0, 28.00},
        {"USB 3.0 flash", 100.0, 35.00},
    };
}

std::vector<StoragePrediction> predict_storage(const std::vector<StoragePoint>& points,
                                               double image_mb) {
    std::vector<StoragePrediction> out;
    if (points.empty() || image_mb < 0.0) {
        return out;
    }
    // Calibrate the fixed portion against the fastest device in the list.
    const StoragePoint* fast = &points.front();
    for (const StoragePoint& p : points) {
        if (p.bandwidth_mbs > fast->bandwidth_mbs) {
            fast = &p;
        }
    }
    const double fast_transfer_ms = image_mb / fast->bandwidth_mbs * 1000.0;
    const double fixed_ms = fast->measured_s * 1000.0 - fast_transfer_ms;

    for (const StoragePoint& p : points) {
        StoragePrediction sp;
        sp.device = p.device;
        sp.measured_ms = p.measured_s * 1000.0;
        sp.bandwidth_ms = p.bandwidth_mbs > 0.0 ? image_mb / p.bandwidth_mbs * 1000.0 : 0.0;
        sp.predicted_ms = fixed_ms + sp.bandwidth_ms;
        sp.residual_ms = sp.measured_ms - sp.predicted_ms;
        out.push_back(sp);
    }
    return out;
}

double speedup(double baseline_ms, double candidate_ms) {
    if (candidate_ms <= 0.0) {
        return 0.0;
    }
    return baseline_ms / candidate_ms;
}

}  // namespace lowl

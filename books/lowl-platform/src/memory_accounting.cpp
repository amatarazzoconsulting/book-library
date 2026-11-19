// MIT License - (c) Anthony Matarazzo 2026
#include "memory_accounting.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace lowl {

double total_mb(const Footprint& f) {
    double sum = 0.0;
    for (const Component& c : f.components) {
        sum += c.mb;
    }
    return sum;
}

double component_share(const Footprint& f, const std::string& name) {
    const double total = total_mb(f);
    if (total <= 0.0) {
        return 0.0;
    }
    for (const Component& c : f.components) {
        if (c.name == name) {
            return c.mb / total;
        }
    }
    return 0.0;
}

std::vector<Footprint> idle_footprints() {
    // Part 3.1. LOWL folds the compositor into the desktop shell, so it has no
    // display server line at all rather than a zero line.
    return {
        {"Windows 11",
         {{"Kernel", 380.0}, {"Display server", 185.0}, {"Desktop shell", 95.0},
          {"Background services", 210.0}, {"Caches", 0.0}}},
        {"macOS Sonoma",
         {{"Kernel", 420.0}, {"Display server", 95.0}, {"Desktop shell", 120.0},
          {"Background services", 565.0}, {"Caches", 0.0}}},
        {"Ubuntu 24.04",
         {{"Kernel", 220.0}, {"Display server", 210.0}, {"Desktop shell", 105.0},
          {"Background services", 125.0}, {"Caches", 0.0}}},
        {"ChromeOS Flex",
         {{"Kernel", 310.0}, {"Display server", 0.0}, {"Desktop shell", 70.0},
          {"Background services", 140.0}, {"Caches", 0.0}}},
        {"LOWL 1.0",
         {{"Kernel", 28.0}, {"Desktop shell", 8.0}, {"Caches", 12.0}}},
    };
}

std::vector<AppMemory> application_table() {
    // Part 3.2. The book prints "N/A" for ChromeOS on several rows and does not
    // give a macOS or ChromeOS column we need, so only the four used here are kept.
    return {
        {"Terminal emulator", 45.0, 52.0, 35.0, 6.0},
        {"File manager", 78.0, 85.0, 54.0, 12.0},
        {"Text editor", 185.0, 210.0, 140.0, 28.0},
        {"Web browser (1 tab)", 543.0, 597.0, 503.0, 189.0},
        {"Spreadsheet", 210.0, 245.0, 155.0, 32.0},
        {"PDF viewer", 67.0, 76.0, 59.0, 18.0},
        {"Media player", 85.0, 96.0, 74.0, 22.0},
    };
}

double mean_app_mb(const std::vector<AppMemory>& apps, const std::string& os) {
    if (apps.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (const AppMemory& a : apps) {
        if (os == "windows") {
            sum += a.windows_mb;
        } else if (os == "macos") {
            sum += a.macos_mb;
        } else if (os == "ubuntu") {
            sum += a.ubuntu_mb;
        } else if (os == "lowl") {
            sum += a.lowl_mb;
        } else {
            return 0.0;
        }
    }
    return sum / static_cast<double>(apps.size());
}

std::vector<ScalingPoint> scaling_table(const std::string& os) {
    // Part 3.3. LOWL is not in the table; the prose states about 30 MB per
    // additional application and over 500 applications in 16 GB.
    if (os == "windows") {
        return {{5, 1800.0}, {10, 3200.0}, {20, 5800.0}, {50, 13500.0}, {100, 26000.0}};
    }
    if (os == "macos") {
        return {{5, 2400.0}, {10, 4100.0}, {20, 7200.0}, {50, 16800.0}, {100, 32000.0}};
    }
    if (os == "ubuntu") {
        return {{5, 1500.0}, {10, 2600.0}, {20, 4800.0}, {50, 11200.0}, {100, 22000.0}};
    }
    // Derived from the stated 30 MB marginal cost on top of the 48 MB idle figure.
    std::vector<ScalingPoint> lowl;
    for (int n : {5, 10, 20, 50, 100}) {
        lowl.push_back({n, 48.0 + 30.0 * n});
    }
    return lowl;
}

Fit fit_scaling(const std::vector<ScalingPoint>& points) {
    Fit fit;
    const double n = static_cast<double>(points.size());
    if (n < 2.0) {
        return fit;
    }
    double sx = 0.0;
    double sy = 0.0;
    double sxx = 0.0;
    double sxy = 0.0;
    for (const ScalingPoint& p : points) {
        const double x = p.apps;
        sx += x;
        sy += p.mb;
        sxx += x * x;
        sxy += x * p.mb;
    }
    const double denom = n * sxx - sx * sx;
    if (denom == 0.0) {
        return fit;
    }
    fit.slope_mb = (n * sxy - sx * sy) / denom;
    fit.intercept_mb = (sy - fit.slope_mb * sx) / n;

    double worst = 0.0;
    double sumsq = 0.0;
    for (const ScalingPoint& p : points) {
        const double err = p.mb - (fit.intercept_mb + fit.slope_mb * p.apps);
        worst = std::max(worst, std::fabs(err));
        sumsq += err * err;
    }
    fit.max_residual_mb = worst;
    fit.rms_residual_mb = std::sqrt(sumsq / n);
    return fit;
}

int capacity_apps(const CapacityModel& model, double ram_mb) {
    const double per_app = model.app_mb + model.per_process_overhead_mb;
    if (per_app <= 0.0) {
        return 0;
    }
    const double spare = ram_mb - model.system_mb;
    if (spare <= 0.0) {
        return 0;
    }
    return static_cast<int>(spare / per_app);
}

double ram_for_apps(const CapacityModel& model, int apps) {
    if (apps <= 0) {
        return model.system_mb;
    }
    return model.system_mb +
           static_cast<double>(apps) * (model.app_mb + model.per_process_overhead_mb);
}

double compressed_heap_mb(double heap_mb, double ratio) {
    if (ratio < 0.0 || ratio > 1.0) {
        return heap_mb;
    }
    return heap_mb * (1.0 - ratio);
}

double effective_rss_mb(double base_mb, const HeapSplit& split) {
    return base_mb + split.hot_mb + compressed_heap_mb(split.cold_mb, split.ratio);
}

}  // namespace lowl

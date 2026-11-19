// MIT License - (c) Anthony Matarazzo 2026
//
// profile_coverage.cpp - weighted coverage measurement per hardware profile.

#include "profile_coverage.hpp"

#include <algorithm>
#include <cstdio>
#include <set>
#include <sstream>

namespace nebula::drivers {

ProfileReport CoverageAnalyzer::analyze(const std::string& profile_name,
                                        const std::vector<HardwareDevice>& devices,
                                        const CoverageOptions& opts) const {
    ProfileReport report;
    report.profile = profile_name;

    double weighted_required = 0.0;
    double weighted_supported = 0.0;
    int fully_supported = 0;
    std::map<std::string, CapabilityStat> cap_stats;

    for (const auto& dev : devices) {
        DeviceCoverage dc;
        dc.name = dev.name;
        dc.hardware_id = dev.hardware_id;
        dc.device_class = dev.device_class;
        dc.required_caps = dev.required_capabilities.size();

        MatchResult m = registry_.match(dev, opts.min_match_score, false);
        if (m.found && opts.require_whql) {
            const DriverEntry* d = registry_.find_by_id(m.driver_id);
            if (d && !d->whql) {
                m.found = false;
                m.reason = "driver " + m.driver_id + " is not WHQL signed";
            }
        }
        if (m.found && !opts.count_fallback_as_match &&
            (m.kind == MatchKind::Wildcard || m.kind == MatchKind::ClassFallback)) {
            m.found = false;
            m.reason = "only a fallback driver matched";
        }

        dc.matched = m.found;
        dc.driver_id = m.driver_id;
        dc.kind = m.kind;

        if (m.found) {
            const DriverEntry* d = registry_.find_by_id(m.driver_id);
            for (const auto& cap : dev.required_capabilities) {
                bool ok = d && std::find(d->capabilities.begin(), d->capabilities.end(), cap) !=
                                   d->capabilities.end();
                if (ok) ++dc.supported_caps;
                else dc.missing_caps.push_back(cap);
            }
            dc.degraded = !dc.missing_caps.empty();
            ++report.matched;
            if (!dc.degraded) ++fully_supported;
            else ++report.degraded;
        } else {
            dc.missing_caps = dev.required_capabilities;
            ++report.unmatched;
        }

        dc.cap_percent = dc.required_caps == 0
                             ? 100.0
                             : 100.0 * static_cast<double>(dc.supported_caps) /
                                   static_cast<double>(dc.required_caps);

        auto wit = opts.class_weight.find(dev.device_class);
        double weight = wit != opts.class_weight.end() ? wit->second : opts.default_weight;
        if (weight < 0.0) weight = 0.0;
        weighted_required += weight * static_cast<double>(dc.required_caps);
        weighted_supported += weight * static_cast<double>(dc.supported_caps);

        for (const auto& cap : dev.required_capabilities) {
            CapabilityStat& st = cap_stats[cap];
            st.name = cap;
            ++st.devices_requiring;
            if (std::find(dc.missing_caps.begin(), dc.missing_caps.end(), cap) ==
                dc.missing_caps.end()) {
                ++st.devices_supported;
            } else {
                report.gaps.push_back(dev.name + " lacks " + cap +
                                      (dc.matched ? " (driver " + dc.driver_id + ")"
                                                  : " (no driver)"));
            }
        }

        report.devices.push_back(dc);
    }

    for (auto& kv : cap_stats) {
        CapabilityStat& st = kv.second;
        st.percent = st.devices_requiring == 0
                         ? 100.0
                         : 100.0 * static_cast<double>(st.devices_supported) /
                               static_cast<double>(st.devices_requiring);
        report.capabilities.push_back(st);
    }
    std::sort(report.capabilities.begin(), report.capabilities.end(),
              [](const CapabilityStat& a, const CapabilityStat& b) {
                  if (a.percent != b.percent) return a.percent < b.percent;
                  return a.name < b.name;
              });

    int total = static_cast<int>(devices.size());
    report.device_coverage = total == 0 ? 100.0 : 100.0 * report.matched / total;
    report.strict_coverage = total == 0 ? 100.0 : 100.0 * fully_supported / total;
    report.capability_coverage =
        weighted_required <= 0.0 ? 100.0 : 100.0 * weighted_supported / weighted_required;
    report.grade = grade_for(report.strict_coverage);
    return report;
}

std::vector<ProfileReport> CoverageAnalyzer::analyze_all(const CoverageOptions& opts) const {
    std::vector<ProfileReport> out;
    for (const auto& p : DriverRegistry::book_profiles()) {
        out.push_back(analyze(p.first, p.second, opts));
    }
    return out;
}

double CoverageAnalyzer::aggregate_capability_coverage(const std::vector<ProfileReport>& reports) {
    double sum = 0.0;
    int n = 0;
    for (const auto& r : reports) {
        if (r.devices.empty()) continue;
        sum += r.capability_coverage;
        ++n;
    }
    return n == 0 ? 0.0 : sum / static_cast<double>(n);
}

std::string CoverageAnalyzer::grade_for(double strict_percent) {
    if (strict_percent >= 99.5) return "A";
    if (strict_percent >= 90.0) return "B";
    if (strict_percent >= 75.0) return "C";
    if (strict_percent >= 60.0) return "D";
    return "F";
}

std::string format_profile_report(const ProfileReport& r) {
    std::ostringstream os;
    os << "profile: " << r.profile << "\n";
    os << "  device                          class        driver        match        caps\n";
    for (const auto& d : r.devices) {
        char line[220];
        std::snprintf(line, sizeof(line), "  %-30s %-11s %-13s %-12s %zu/%zu (%.0f%%)%s\n",
                      d.name.c_str(), to_string(d.device_class),
                      d.matched ? d.driver_id.c_str() : "-", to_string(d.kind),
                      d.supported_caps, d.required_caps, d.cap_percent,
                      d.degraded ? " DEGRADED" : (d.matched ? "" : " UNMATCHED"));
        os << line;
    }
    char tail[320];
    std::snprintf(tail, sizeof(tail),
                  "  matched=%d unmatched=%d degraded=%d device_coverage=%.1f%% "
                  "capability_coverage=%.1f%% strict_coverage=%.1f%% grade=%s\n",
                  r.matched, r.unmatched, r.degraded, r.device_coverage,
                  r.capability_coverage, r.strict_coverage, r.grade.c_str());
    os << tail;
    if (!r.gaps.empty()) {
        os << "  gaps:\n";
        for (const auto& g : r.gaps) os << "    - " << g << "\n";
    }
    return os.str();
}

std::string format_coverage_table(const std::vector<ProfileReport>& reports) {
    std::ostringstream os;
    os << "profile                            devices  matched  degraded  device%  cap%   strict%  grade\n";
    for (const auto& r : reports) {
        char line[220];
        std::snprintf(line, sizeof(line), "%-34s %7zu  %7d  %8d  %6.1f  %6.1f  %7.1f  %s\n",
                      r.profile.c_str(), r.devices.size(), r.matched, r.degraded,
                      r.device_coverage, r.capability_coverage, r.strict_coverage,
                      r.grade.c_str());
        os << line;
    }
    char tail[128];
    std::snprintf(tail, sizeof(tail), "mean capability coverage across profiles: %.1f%%\n",
                  CoverageAnalyzer::aggregate_capability_coverage(reports));
    os << tail;
    return os.str();
}

} // namespace nebula::drivers

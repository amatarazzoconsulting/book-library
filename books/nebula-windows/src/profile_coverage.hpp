// MIT License - (c) Anthony Matarazzo 2026
//
// profile_coverage.hpp - hardware profile coverage analysis.
//
// The book claims the driver library "enables NEBULA OS to run on virtually any
// x86_64 desktop or laptop". This header turns that claim into a measurement:
//
//   CoverageAnalyzer::analyze(registry, profile, weights)
//       -> per device match, per capability support, weighted coverage percent
//
// Coverage is reported two ways, because they answer different questions:
//   device_coverage     fraction of devices that get a driver (can it boot?)
//   capability_coverage weighted fraction of required capabilities that are
//                       actually provided (is it fully functional?)
// A device can match a driver and still lose features, for example a fallback
// display driver that has no ray tracing. The report keeps those apart.

#ifndef NEBULA_PROFILE_COVERAGE_HPP
#define NEBULA_PROFILE_COVERAGE_HPP

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "driver_inventory.hpp"

namespace nebula::drivers {

struct DeviceCoverage {
    std::string name;
    std::string hardware_id;
    DeviceClass device_class{DeviceClass::Display};
    bool matched{false};
    std::string driver_id;
    MatchKind kind{MatchKind::None};
    std::size_t required_caps{0};
    std::size_t supported_caps{0};
    std::vector<std::string> missing_caps;
    double cap_percent{0.0};
    bool degraded{false};   // matched but some capabilities are missing
};

struct CapabilityStat {
    std::string name;
    int devices_requiring{0};
    int devices_supported{0};
    double percent{0.0};
};

struct ProfileReport {
    std::string profile;
    std::vector<DeviceCoverage> devices;
    std::vector<CapabilityStat> capabilities;
    int matched{0};
    int unmatched{0};
    int degraded{0};
    double device_coverage{0.0};      // percent of devices with a driver
    double capability_coverage{0.0};  // weighted percent of required capabilities met
    double strict_coverage{0.0};      // percent of devices fully supported
    std::string grade;                // A..F from strict coverage
    std::vector<std::string> gaps;    // human readable list of missing pieces
};

struct CoverageOptions {
    bool require_whql{true};              // reject non WHQL drivers
    int min_match_score{0};               // reject weak (class fallback) matches
    std::map<DeviceClass, double> class_weight; // defaults to 1.0 per class
    double default_weight{1.0};
    bool count_fallback_as_match{true};   // wildcard/class fallback counts as matched
};

class CoverageAnalyzer {
public:
    explicit CoverageAnalyzer(const DriverRegistry& registry) : registry_(registry) {}

    ProfileReport analyze(const std::string& profile_name,
                          const std::vector<HardwareDevice>& devices,
                          const CoverageOptions& opts = CoverageOptions()) const;

    // Analyzes every profile in the registry's book profile list.
    std::vector<ProfileReport> analyze_all(const CoverageOptions& opts = CoverageOptions()) const;

    // Weighted capability coverage across several profiles.
    static double aggregate_capability_coverage(const std::vector<ProfileReport>& reports);

    static std::string grade_for(double strict_percent);

private:
    const DriverRegistry& registry_;
};

std::string format_profile_report(const ProfileReport& r);
std::string format_coverage_table(const std::vector<ProfileReport>& reports);

} // namespace nebula::drivers

#endif // NEBULA_PROFILE_COVERAGE_HPP

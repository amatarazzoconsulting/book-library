// MIT License - (c) Anthony Matarazzo 2026
//
// test_driver_inventory.cpp - catalog lookup and capability matching.

#include <cstdio>
#include <string>
#include <vector>

#include "driver_inventory.hpp"

using namespace nebula::drivers;

namespace {

int failures = 0;

void check(bool cond, const char* what) {
    if (cond) {
        std::printf("PASS %s\n", what);
    } else {
        std::printf("FAIL %s\n", what);
        ++failures;
    }
}

void check_eq(long long got, long long want, const char* what) {
    if (got == want) {
        std::printf("PASS %s (%lld)\n", what, got);
    } else {
        std::printf("FAIL %s: got %lld want %lld\n", what, got, want);
        ++failures;
    }
}

DriverEntry simple(const std::string& id, DeviceClass cls, std::vector<std::string> ids,
                   std::vector<std::string> caps, bool whql = true) {
    DriverEntry d;
    d.id = id;
    d.binary = id + ".sys";
    d.vendor = "Test";
    d.device_class = cls;
    d.hardware_ids = std::move(ids);
    d.capabilities = std::move(caps);
    d.whql = whql;
    d.size_mb = 1.0;
    d.version = "1.0";
    return d;
}

HardwareDevice device(const std::string& hw, DeviceClass cls, std::vector<std::string> caps) {
    HardwareDevice d;
    d.name = "device";
    d.hardware_id = hw;
    d.device_class = cls;
    d.required_capabilities = std::move(caps);
    return d;
}

} // namespace

int main() {
    std::printf("== test_driver_inventory\n");

    // ---- the book catalog ------------------------------------------------
    DriverRegistry reg = DriverRegistry::book_catalog();
    check_eq(static_cast<long long>(reg.size()), 38, "book catalog holds 38 drivers");
    check(reg.find_by_id("nvlddmkm") != nullptr, "nvlddmkm is in the catalog");
    check(reg.find_by_id("nope") == nullptr, "unknown id returns null");
    check_eq(static_cast<long long>(reg.by_class(DeviceClass::Display).size()), 4,
             "four display drivers");
    check_eq(static_cast<long long>(reg.by_class(DeviceClass::Security).size()), 2,
             "two security drivers");
    check_eq(static_cast<long long>(reg.by_class(DeviceClass::Chipset).size()), 2,
             "two chipset drivers");

    // Every driver is WHQL in the transcribed catalog, as the book claims.
    bool all_whql = true;
    bool all_sized = true;
    for (const auto& d : reg.all()) {
        all_whql = all_whql && d.whql;
        all_sized = all_sized && d.size_mb > 0.0;
    }
    check(all_whql, "every catalog driver is WHQL");
    check(all_sized, "every catalog driver has a size");

    // ---- add() validation ------------------------------------------------
    std::string err;
    DriverRegistry bad;
    check(!bad.add(simple("", DeviceClass::Display, {"A"}, {"b"}), &err), "empty id rejected");
    err.clear();
    DriverEntry no_binary = simple("x", DeviceClass::Display, {"A"}, {"b"});
    no_binary.binary.clear();
    check(!bad.add(no_binary, &err), "missing binary rejected");
    check(err.find("binary") != std::string::npos, "binary error is explicit");
    err.clear();
    check(!bad.add(simple("x", DeviceClass::Display, {}, {"b"}), &err),
          "driver with no hardware ids rejected");
    check(err.find("hardware ids") != std::string::npos, "hardware id error is explicit");
    check(bad.add(simple("x", DeviceClass::Display, {"A"}, {"b"}), &err), "valid driver accepted");
    err.clear();
    check(!bad.add(simple("x", DeviceClass::Display, {"A"}, {"b"}), &err), "duplicate rejected");
    check(err.find("duplicate") != std::string::npos, "duplicate error is explicit");

    // ---- exact, prefix, class code, wildcard, fallback -------------------
    DriverRegistry kinds;
    kinds.add(simple("exact", DeviceClass::Display, {"PCI\\VEN_0001&DEV_0001"}, {"d3d12"}));
    kinds.add(simple("prefix", DeviceClass::Network, {"PCI\\VEN_0002&DEV_"}, {"ethernet"}));
    kinds.add(simple("classcode", DeviceClass::Storage, {"PCI\\CC_010802"}, {"nvme"}));
    kinds.add(simple("wild", DeviceClass::Audio, {"HDAUDIO\\FUNC_01&VEN_0003&DEV_*"}, {"hd-audio"}));
    kinds.add(simple("fallback", DeviceClass::Input, {"CLASS:*"}, {"hid-keyboard"}));

    MatchResult m = kinds.match(device("PCI\\VEN_0001&DEV_0001", DeviceClass::Display, {"d3d12"}));
    check(m.found, "exact id matches");
    check(m.kind == MatchKind::ExactId, "exact id kind");
    check_eq(m.score, 105, "exact id scores 100 plus 5 for WHQL");
    check(m.driver_id == "exact", "exact id picks the right driver");

    m = kinds.match(device("PCI\\VEN_0002&DEV_00AA", DeviceClass::Network, {"ethernet"}));
    check(m.kind == MatchKind::PrefixId, "trailing separator is a prefix id");
    check_eq(m.score, 85, "prefix id scores 80 plus 5 for WHQL");

    m = kinds.match(device("PCI\\CC_010802", DeviceClass::Storage, {"nvme"}));
    check(m.kind == MatchKind::ExactId, "identical class code counts as exact");
    // A class code is a literal prefix, so PCI\\CC_0108 covers the 0108 subclass only.
    DriverRegistry sub;
    sub.add(simple("c8", DeviceClass::Storage, {"PCI\\CC_0108"}, {"nvme"}));
    m = sub.match(device("PCI\\CC_010801", DeviceClass::Storage, {"nvme"}));
    check(m.kind == MatchKind::ClassCode, "class code matches as a prefix");
    m = sub.match(device("PCI\\CC_010802", DeviceClass::Storage, {"nvme"}));
    check(m.kind == MatchKind::ClassCode, "class code covers its own subclass");
    m = kinds.match(device("PCI\\CC_010803", DeviceClass::Storage, {"nvme"}));
    check(!m.found, "a full class code does not cover a different subclass");

    m = kinds.match(device("HDAUDIO\\FUNC_01&VEN_0003&DEV_00FF", DeviceClass::Audio, {"hd-audio"}));
    check(m.kind == MatchKind::Wildcard, "wildcard suffix matches");
    check_eq(m.score, 45, "wildcard scores 40 plus 5 for WHQL");

    m = kinds.match(device("ACPI\\SOMETHING", DeviceClass::Input, {"hid-keyboard"}));
    check(m.kind == MatchKind::ClassFallback, "CLASS:* serves its own class");
    check_eq(m.score, 15, "class fallback scores 10 plus 5 for WHQL");

    m = kinds.match(device("ACPI\\SOMETHING", DeviceClass::Display, {"d3d12"}));
    check(!m.found, "CLASS:* does not serve another class");
    check(m.reason.find("no driver") != std::string::npos, "no match explains itself");

    // ---- capability gating ----------------------------------------------
    DriverRegistry caps;
    caps.add(simple("rich", DeviceClass::Display, {"PCI\\VEN_0009&DEV_0001"},
                    {"d3d12", "vulkan", "raytracing"}));
    HardwareDevice needs_ray = device("PCI\\VEN_0009&DEV_0001", DeviceClass::Display,
                                      {"d3d12", "raytracing"});
    m = caps.match(needs_ray);
    check(m.found, "driver with every capability matches");
    check(m.missing_capabilities.empty(), "nothing missing when fully supported");

    HardwareDevice needs_dlss = device("PCI\\VEN_0009&DEV_0001", DeviceClass::Display,
                                       {"d3d12", "dlss", "ecc"});
    m = caps.match(needs_dlss);
    check(!m.found, "missing capabilities reject the driver by default");
    check(m.reason.find("lacks required capabilities") != std::string::npos,
          "rejection reason names the capability shortfall");
    check_eq(static_cast<long long>(m.missing_capabilities.size()), 2, "two capabilities missing");

    m = caps.match(needs_dlss, 0, false);
    check(m.found, "lenient match returns the id match anyway");
    check_eq(static_cast<long long>(m.missing_capabilities.size()), 2,
             "lenient match still reports the shortfall");

    check(DriverRegistry::supports(*caps.find_by_id("rich"), {"d3d12"}, nullptr),
          "supports reports a subset as supported");
    std::vector<std::string> missing;
    check(!DriverRegistry::supports(*caps.find_by_id("rich"), {"d3d12", "ecc"}, &missing),
          "supports reports a superset as unsupported");
    check(missing.size() == 1 && missing[0] == "ecc", "supports fills the missing list");

    // ---- scoring and tie breaks ------------------------------------------
    // A specific id must beat a class fallback for the same device.
    DriverRegistry rank;
    rank.add(simple("fallback", DeviceClass::Display, {"CLASS:*"}, {"d3d12"}));
    rank.add(simple("specific", DeviceClass::Display, {"PCI\\VEN_0005&DEV_0001"}, {"d3d12"}));
    m = rank.match(device("PCI\\VEN_0005&DEV_0001", DeviceClass::Display, {"d3d12"}));
    check(m.driver_id == "specific", "specific id beats the class fallback");
    check_eq(m.score, 105, "specific score wins");

    // WHQL breaks a tie before the id ordering does.
    DriverRegistry tie;
    tie.add(simple("zzz", DeviceClass::Display, {"PCI\\VEN_0006&DEV_0001"}, {"d3d12"}, false));
    tie.add(simple("aaa", DeviceClass::Display, {"PCI\\VEN_0006&DEV_0001"}, {"d3d12"}, true));
    m = tie.match(device("PCI\\VEN_0006&DEV_0001", DeviceClass::Display, {"d3d12"}));
    check(m.driver_id == "aaa", "WHQL driver wins an equal score");

    DriverRegistry tie2;
    tie2.add(simple("zzz", DeviceClass::Display, {"PCI\\VEN_0007&DEV_0001"}, {"d3d12"}, false));
    tie2.add(simple("aaa", DeviceClass::Display, {"PCI\\VEN_0007&DEV_0001"}, {"d3d12"}, false));
    m = tie2.match(device("PCI\\VEN_0007&DEV_0001", DeviceClass::Display, {"d3d12"}));
    check(m.driver_id == "aaa", "id order breaks a remaining tie");

    // min_score filters weak matches.
    m = kinds.match(device("ACPI\\SOMETHING", DeviceClass::Input, {"hid-keyboard"}), 50);
    check(!m.found, "min_score rejects a class fallback");
    m = kinds.match(device("PCI\\VEN_0001&DEV_0001", DeviceClass::Display, {"d3d12"}), 100);
    check(m.found, "min_score keeps an exact match");

    // ---- the real catalog on real profile devices ------------------------
    m = reg.match(device("PCI\\VEN_10DE&DEV_2684", DeviceClass::Display,
                         {"d3d12", "vulkan", "raytracing", "dlss", "hdmi-audio"}));
    check(m.driver_id == "nvlddmkm", "RTX 4090 maps to nvlddmkm");
    check(m.kind == MatchKind::ExactId, "RTX 4090 is an exact id match");

    m = reg.match(device("PCI\\VEN_DEAD&DEV_BEEF", DeviceClass::Display, {"d3d12"}));
    check(m.driver_id == "basicdisplay", "unknown GPU falls back to basicdisplay");
    check(m.kind == MatchKind::ClassFallback, "basicdisplay match is a class fallback");

    m = reg.match(device("PCI\\CC_010802", DeviceClass::Storage, {"nvme", "trim", "smart"}));
    check(m.driver_id == "stornvme", "NVMe class code maps to stornvme");

    m = reg.match(device("PCI\\VEN_8086&DEV_09AB", DeviceClass::Storage, {"nvme", "raid", "vmd"}));
    check(m.driver_id == "iaStorVD", "VMD controller maps to iaStorVD");

    m = reg.match(device("HDAUDIO\\FUNC_01&VEN_10EC&DEV_1168", DeviceClass::Audio,
                         {"hd-audio", "dts", "line-out"}));
    check(m.driver_id == "RTKVHD64", "ALC4080 maps to RTKVHD64");

    // An ALC897 asks for plain hd-audio only, which both Realtek and Microsoft cover.
    m = reg.match(device("HDAUDIO\\FUNC_01&VEN_10EC", DeviceClass::Audio, {"hd-audio"}));
    check(m.found, "generic Realtek codec still matches");
    // The generic Realtek id is an exact match for HdAudio.sys, and the book's
    // Profile E driver list says exactly that for the ALC897.
    check(m.driver_id == "HdAudio", "generic Realtek codec maps to HdAudio.sys");

    m = reg.match(device("ACPI\\PNP0C31", DeviceClass::Security, {"tpm2", "bitlocker"}));
    check(m.driver_id == "tpm", "TPM 2.0 maps to tpm.sys");

    // ---- class name parsing ---------------------------------------------
    check(class_from_string("gpu").has_value() &&
              *class_from_string("gpu") == DeviceClass::Display,
          "gpu alias maps to Display");
    check(class_from_string("NET").has_value() &&
              *class_from_string("NET") == DeviceClass::Network,
          "case insensitive class alias");
    check(!class_from_string("bogus").has_value(), "unknown class name returns nullopt");
    check(std::string(to_string(DeviceClass::Thunderbolt)) == "Thunderbolt", "class text");
    check(std::string(to_string(MatchKind::ClassFallback)) == "class-fallback", "match kind text");

    // ---- profiles --------------------------------------------------------
    auto profiles = DriverRegistry::book_profiles();
    check_eq(static_cast<long long>(profiles.size()), 5, "five book profiles");
    check_eq(static_cast<long long>(profiles[0].second.size()), 8, "profile A has eight devices");
    check_eq(static_cast<long long>(profiles[2].second.size()), 10, "profile C has ten devices");
    check_eq(static_cast<long long>(profiles[4].second.size()), 7, "profile E has seven devices");
    check(profiles[0].first.find("Gaming Desktop") != std::string::npos, "profile A is named");

    // Every device in every book profile gets some driver, with capabilities.
    int total_devices = 0;
    int matched = 0;
    for (const auto& p : profiles) {
        for (const auto& d : p.second) {
            ++total_devices;
            if (reg.match(d).found) ++matched;
        }
    }
    check_eq(matched, total_devices, "every book profile device has a driver");

    // ---- formatting ------------------------------------------------------
    std::string line = format_match(device("PCI\\VEN_10DE&DEV_2684", DeviceClass::Display,
                                           {"d3d12"}),
                                    reg.match(device("PCI\\VEN_10DE&DEV_2684",
                                                     DeviceClass::Display, {"d3d12"})));
    check(line.find("nvlddmkm.sys") != std::string::npos, "match line names the binary");
    std::string catalog = format_catalog(reg);
    check(catalog.find("stornvme") != std::string::npos, "catalog table lists drivers");
    check(catalog.find("size_mb") != std::string::npos, "catalog table has a header");

    if (failures == 0) {
        std::printf("all driver_inventory checks passed\n");
        return 0;
    }
    std::printf("%d driver_inventory checks FAILED\n", failures);
    return 1;
}

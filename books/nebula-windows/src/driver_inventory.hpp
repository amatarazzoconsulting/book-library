// MIT License - (c) Anthony Matarazzo 2026
//
// driver_inventory.hpp - Windows driver catalog and capability matching.
//
// Phase 5 of the NEBULA boot sequence asks the WindowsDriverRegistryMapDB for a
// driver that matches a detected device. This header models the catalog side of
// that query without any real .sys files:
//
//   DriverEntry     one WHQL driver, its hardware IDs and capabilities
//   HardwareDevice  a detected device with the capabilities it needs
//   DriverRegistry  exact, prefix and class-code lookup with scoring
//
// A driver only matches a device when every capability the device requires is
// declared by the driver. The score ranks otherwise valid candidates, so a
// specific hardware ID always beats a wildcard fallback.
//
// Hardware ID forms understood by the matcher:
//   PCI\VEN_10DE&DEV_2684   exact id
//   PCI\VEN_8086&DEV_       prefix id (trailing separator)
//   PCI\CC_010802           class code, matched as a prefix
//   PCI\VEN_10DE&DEV_*      wildcard, any suffix after the star
//   CLASS:*                  any device of the driver's own device class

#ifndef NEBULA_DRIVER_INVENTORY_HPP
#define NEBULA_DRIVER_INVENTORY_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace nebula::drivers {

enum class DeviceClass {
    Display = 0,
    Storage,
    Network,
    Audio,
    Input,
    Chipset,
    Thunderbolt,
    Power,
    Security
};

const char* to_string(DeviceClass c);
std::optional<DeviceClass> class_from_string(const std::string& s);

struct DriverEntry {
    std::string id;          // stable key, e.g. "nvlddmkm"
    std::string binary;      // nvlddmkm.sys
    std::string vendor;      // NVIDIA
    DeviceClass device_class{DeviceClass::Display};
    std::vector<std::string> hardware_ids;
    std::vector<std::string> capabilities;
    bool whql{true};
    double size_mb{0.0};
    std::string version;
};

struct HardwareDevice {
    std::string name;
    std::string hardware_id;
    DeviceClass device_class{DeviceClass::Display};
    std::vector<std::string> required_capabilities;
};

enum class MatchKind { None, ExactId, PrefixId, ClassCode, Wildcard, ClassFallback };

const char* to_string(MatchKind k);

struct MatchResult {
    bool found{false};
    std::string driver_id;
    std::string binary;
    MatchKind kind{MatchKind::None};
    int score{0};
    std::vector<std::string> missing_capabilities;
    std::string reason;
};

class DriverRegistry {
public:
    // Returns false for a duplicate id, an empty binary name or a driver with no
    // hardware IDs. Validation errors are appended to *error.
    bool add(DriverEntry entry, std::string* error = nullptr);

    std::size_t size() const { return drivers_.size(); }
    const std::vector<DriverEntry>& all() const { return drivers_; }
    const DriverEntry* find_by_id(const std::string& id) const;

    // Devices whose class equals c, in insertion order.
    std::vector<const DriverEntry*> by_class(DeviceClass c) const;

    // Best driver for a device, or found == false with a reason.
    // min_score rejects weak candidates (ClassFallback is 10, ExactId is 100).
    // require_capabilities = true (default) only returns a driver that declares
    // every capability the device asks for. Pass false to get the ID match even
    // when it falls short, which is what the coverage analyzer needs in order to
    // report a degraded device instead of an unmatched one.
    MatchResult match(const HardwareDevice& device, int min_score = 0,
                      bool require_capabilities = true) const;

    // True when the driver declares every capability in required.
    static bool supports(const DriverEntry& d, const std::vector<std::string>& required,
                         std::vector<std::string>* missing = nullptr);

    // The catalog transcribed from the book's driver inventory tables.
    static DriverRegistry book_catalog();

    // The five hardware profiles A-E from the book, with their devices.
    static std::vector<HardwareDevice> profile_a();
    static std::vector<HardwareDevice> profile_b();
    static std::vector<HardwareDevice> profile_c();
    static std::vector<HardwareDevice> profile_d();
    static std::vector<HardwareDevice> profile_e();
    static std::vector<std::pair<std::string, std::vector<HardwareDevice>>> book_profiles();

private:
    std::vector<DriverEntry> drivers_;
};

std::string format_match(const HardwareDevice& device, const MatchResult& m);
std::string format_catalog(const DriverRegistry& reg);

} // namespace nebula::drivers

#endif // NEBULA_DRIVER_INVENTORY_HPP

// MIT License - (c) Anthony Matarazzo 2026
//
// driver_inventory.cpp - catalog lookup and capability matching.

#include "driver_inventory.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <set>
#include <sstream>

namespace nebula::drivers {
namespace {

bool starts_with(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool is_class_code(const std::string& id) { return starts_with(id, "PCI\\CC_"); }

std::string upper(std::string s) {
    for (char& c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    return s;
}

int base_score(MatchKind k) {
    switch (k) {
        case MatchKind::ExactId:       return 100;
        case MatchKind::PrefixId:      return 80;
        case MatchKind::ClassCode:     return 60;
        case MatchKind::Wildcard:      return 40;
        case MatchKind::ClassFallback: return 10;
        case MatchKind::None:          return 0;
    }
    return 0;
}

// How a single hardware ID of a driver relates to the device ID.
MatchKind id_kind(const std::string& driver_id, const std::string& device_id) {
    if (driver_id == device_id) return MatchKind::ExactId;
    if (!driver_id.empty() && driver_id.back() == '*') {
        std::string stem = driver_id.substr(0, driver_id.size() - 1);
        if (starts_with(device_id, stem)) return MatchKind::Wildcard;
    }
    if (is_class_code(driver_id)) {
        // A class code is a prefix: PCI\CC_0108 covers PCI\CC_010802.
        return starts_with(device_id, driver_id) ? MatchKind::ClassCode : MatchKind::None;
    }
    if (device_id.size() > driver_id.size() && starts_with(device_id, driver_id) &&
        (ends_with(driver_id, "&") || ends_with(driver_id, "\\") || ends_with(driver_id, "&DEV_"))) {
        return MatchKind::PrefixId;
    }
    return MatchKind::None;
}

} // namespace

const char* to_string(DeviceClass c) {
    switch (c) {
        case DeviceClass::Display:     return "Display";
        case DeviceClass::Storage:     return "Storage";
        case DeviceClass::Network:     return "Network";
        case DeviceClass::Audio:       return "Audio";
        case DeviceClass::Input:       return "Input";
        case DeviceClass::Chipset:     return "Chipset";
        case DeviceClass::Thunderbolt: return "Thunderbolt";
        case DeviceClass::Power:       return "Power";
        case DeviceClass::Security:    return "Security";
    }
    return "Unknown";
}

std::optional<DeviceClass> class_from_string(const std::string& s) {
    std::string k = upper(s);
    if (k == "DISPLAY" || k == "GPU") return DeviceClass::Display;
    if (k == "STORAGE") return DeviceClass::Storage;
    if (k == "NETWORK" || k == "NET") return DeviceClass::Network;
    if (k == "AUDIO") return DeviceClass::Audio;
    if (k == "INPUT") return DeviceClass::Input;
    if (k == "CHIPSET") return DeviceClass::Chipset;
    if (k == "THUNDERBOLT" || k == "USB") return DeviceClass::Thunderbolt;
    if (k == "POWER") return DeviceClass::Power;
    if (k == "SECURITY" || k == "TPM") return DeviceClass::Security;
    return std::nullopt;
}

const char* to_string(MatchKind k) {
    switch (k) {
        case MatchKind::None:          return "none";
        case MatchKind::ExactId:       return "exact-id";
        case MatchKind::PrefixId:      return "prefix-id";
        case MatchKind::ClassCode:     return "class-code";
        case MatchKind::Wildcard:      return "wildcard";
        case MatchKind::ClassFallback: return "class-fallback";
    }
    return "none";
}

bool DriverRegistry::add(DriverEntry entry, std::string* error) {
    if (entry.id.empty()) {
        if (error) *error = "driver with empty id";
        return false;
    }
    if (entry.binary.empty()) {
        if (error) *error = entry.id + " has no binary name";
        return false;
    }
    if (entry.hardware_ids.empty()) {
        if (error) *error = entry.id + " has no hardware ids";
        return false;
    }
    for (const auto& d : drivers_) {
        if (d.id == entry.id) {
            if (error) *error = "duplicate driver id " + entry.id;
            return false;
        }
    }
    drivers_.push_back(std::move(entry));
    return true;
}

const DriverEntry* DriverRegistry::find_by_id(const std::string& id) const {
    for (const auto& d : drivers_) {
        if (d.id == id) return &d;
    }
    return nullptr;
}

std::vector<const DriverEntry*> DriverRegistry::by_class(DeviceClass c) const {
    std::vector<const DriverEntry*> out;
    for (const auto& d : drivers_) {
        if (d.device_class == c) out.push_back(&d);
    }
    return out;
}

bool DriverRegistry::supports(const DriverEntry& d, const std::vector<std::string>& required,
                              std::vector<std::string>* missing) {
    bool ok = true;
    for (const auto& cap : required) {
        if (std::find(d.capabilities.begin(), d.capabilities.end(), cap) == d.capabilities.end()) {
            ok = false;
            if (missing) missing->push_back(cap);
        }
    }
    return ok;
}

MatchResult DriverRegistry::match(const HardwareDevice& device, int min_score,
                                  bool require_capabilities) const {
    MatchResult best;
    int best_score = -1;
    MatchResult rejected;   // closest candidate that failed on capabilities

    for (const auto& d : drivers_) {
        MatchKind kind = MatchKind::None;
        for (const auto& hid : d.hardware_ids) {
            MatchKind k = id_kind(hid, device.hardware_id);
            if (base_score(k) > base_score(kind)) kind = k;
        }
        bool class_fallback = false;
        if (kind == MatchKind::None && d.device_class == device.device_class &&
            std::find(d.hardware_ids.begin(), d.hardware_ids.end(), "CLASS:*") !=
                d.hardware_ids.end()) {
            // A driver that declares CLASS:* serves any device of its class. This is
            // the weakest match and only applies when no ID matched at all.
            kind = MatchKind::ClassFallback;
            class_fallback = true;
        }
        if (kind == MatchKind::None) continue;

        int score = base_score(kind);
        if (d.whql) score += 5;

        std::vector<std::string> missing;
        if (!supports(d, device.required_capabilities, &missing)) {
            if (require_capabilities) {
                // Keep the strongest shortfall so the caller gets a useful reason.
                if (score > rejected.score) {
                    rejected.found = false;
                    rejected.driver_id = d.id;
                    rejected.binary = d.binary;
                    rejected.kind = kind;
                    rejected.score = score;
                    rejected.missing_capabilities = missing;
                }
                continue;
            }
        }
        if (score < min_score) continue;

        bool better = score > best_score;
        if (score == best_score && !best.driver_id.empty()) {
            const DriverEntry* incumbent = find_by_id(best.driver_id);
            if (incumbent && incumbent->whql != d.whql) better = d.whql;
            else better = d.id < best.driver_id; // deterministic tie break
        } else if (score == best_score && best.driver_id.empty()) {
            better = true;
        }
        if (better) {
            best_score = score;
            best.found = true;
            best.driver_id = d.id;
            best.binary = d.binary;
            best.kind = kind;
            best.score = score;
            best.missing_capabilities = missing;
            best.reason = class_fallback ? "no hardware id matched, class fallback used" : "matched";
        }
    }

    if (!best.found) {
        if (rejected.score > 0) {
            best = rejected;
            best.reason = "driver " + rejected.driver_id +
                          " matches the hardware id but lacks required capabilities";
        } else {
            best.reason = "no driver for hardware id " + device.hardware_id;
        }
    }
    return best;
}

DriverRegistry DriverRegistry::book_catalog() {
    DriverRegistry reg;
    auto add = [&reg](DriverEntry d) {
        std::string err;
        if (!reg.add(std::move(d), &err)) {
            std::fprintf(stderr, "catalog error: %s\n", err.c_str());
        }
    };

    // Display, from the GPU tables (section 2 of the inventory).
    add({"nvlddmkm", "nvlddmkm.sys", "NVIDIA", DeviceClass::Display,
         {"PCI\\VEN_10DE&DEV_2684", "PCI\\VEN_10DE&DEV_2204", "PCI\\VEN_10DE&DEV_1E07",
          "PCI\\VEN_10DE&DEV_1B80", "PCI\\VEN_10DE&DEV_28A0", "PCI\\VEN_10DE&DEV_2520",
          "PCI\\VEN_10DE&DEV_1F91", "PCI\\VEN_10DE&DEV_1EB1"},
         {"d3d12", "vulkan", "opengl", "raytracing", "dlss", "hdmi-audio", "ecc"},
         true, 1200.0, "551.23"});
    add({"amdkmdag", "amdkmdag.sys", "AMD", DeviceClass::Display,
         {"PCI\\VEN_1002&DEV_744C", "PCI\\VEN_1002&DEV_73E4", "PCI\\VEN_1002&DEV_15BF",
          "PCI\\VEN_1002&DEV_6985", "PCI\\VEN_1002&DEV_66AF"},
         {"d3d12", "vulkan", "opengl", "raytracing", "hdmi-audio"},
         true, 950.0, "24.1.1"});
    add({"igdkmd64", "igdkmd64.sys", "Intel", DeviceClass::Display,
         {"PCI\\VEN_8086&DEV_7D55", "PCI\\VEN_8086&DEV_56A0", "PCI\\VEN_8086&DEV_9A49",
          "PCI\\VEN_8086&DEV_46A6", "PCI\\VEN_8086&DEV_9BC8", "PCI\\VEN_8086&DEV_8A51"},
         {"d3d12", "vulkan", "opengl", "hdmi-audio", "quicksync"},
         true, 620.0, "31.0.101"});
    add({"basicdisplay", "basicdisplay.sys", "Microsoft", DeviceClass::Display,
         {"CLASS:*"},
         {"d3d12", "opengl"},
         true, 2.0, "10.0.26100"});

    // Storage.
    add({"stornvme", "stornvme.sys", "Microsoft", DeviceClass::Storage,
         {"PCI\\CC_010802"},
         {"nvme", "trim", "smart"},
         true, 3.0, "10.0.26100"});
    add({"iaStorVD", "iaStorVD.sys", "Intel", DeviceClass::Storage,
         {"PCI\\VEN_8086&DEV_09AB"},
         {"nvme", "raid", "vmd", "trim"},
         true, 4.5, "20.0.0"});
    add({"iaStorAC", "iaStorAC.sys", "Intel", DeviceClass::Storage,
         {"PCI\\VEN_8086&DEV_2822"},
         {"sata", "raid", "optane", "trim"},
         true, 4.0, "18.7.6"});
    add({"storahci", "storahci.sys", "Microsoft", DeviceClass::Storage,
         {"PCI\\CC_010601"},
         {"sata", "trim"},
         true, 2.0, "10.0.26100"});
    add({"amd_sata", "amd_sata.sys", "AMD", DeviceClass::Storage,
         {"PCI\\VEN_1022&DEV_7901"},
         {"sata", "raid"},
         true, 3.0, "9.3.0"});
    add({"lsi_sas", "lsi_sas.sys", "Broadcom", DeviceClass::Storage,
         {"PCI\\VEN_1000&DEV_0072"},
         {"sas", "raid", "smart"},
         true, 5.0, "2.0.0"});

    // Network.
    add({"netwtw10", "netwtw10.sys", "Intel", DeviceClass::Network,
         {"PCI\\VEN_8086&DEV_7A70"},
         {"wifi6e", "wpa3", "bluetooth"},
         true, 45.0, "23.40.0"});
    add({"e2xw10x64", "e2xw10x64.sys", "Intel", DeviceClass::Network,
         {"PCI\\VEN_8086&DEV_15F3"},
         {"ethernet", "wol", "vlan"},
         true, 30.0, "29.3.0"});
    add({"e3dx", "e3dx.sys", "Intel", DeviceClass::Network,
         {"PCI\\VEN_8086&DEV_1889"},
         {"ethernet", "wol", "vlan", "sr-iov"},
         true, 50.0, "4.2.0"});
    add({"qcfilter", "qcfilter.sys", "Qualcomm", DeviceClass::Network,
         {"USB\\VID_05C6&PID_90DC"},
         {"wwan5g", "gnss"},
         true, 40.0, "1.0.0"});
    add({"rt640x64", "rt640x64.sys", "Realtek", DeviceClass::Network,
         {"PCI\\VEN_10EC&DEV_8168"},
         {"ethernet", "wol"},
         true, 25.0, "10.68.0"});
    add({"mtkwl6ex", "mtkwl6ex.sys", "MediaTek", DeviceClass::Network,
         {"PCI\\VEN_14C3&DEV_7927", "PCI\\VEN_14C3&DEV_0616"},
         {"wifi6e", "wpa3", "bluetooth"},
         true, 42.0, "3.3.0"});
    add({"netwtw08", "netwtw08.sys", "Intel", DeviceClass::Network,
         {"PCI\\VEN_8086&DEV_2723"},
         {"wifi6", "wpa3", "bluetooth"},
         true, 44.0, "22.220.0"});

    // Audio.
    add({"RTKVHD64", "RTKVHD64.sys", "Realtek", DeviceClass::Audio,
         {"HDAUDIO\\FUNC_01&VEN_10EC&DEV_1168"},
         {"hd-audio", "dts", "line-out"},
         true, 22.0, "6.0.9500"});
    add({"HdAudio", "HdAudio.sys", "Microsoft", DeviceClass::Audio,
         {"HDAUDIO\\FUNC_01&VEN_10EC"},
         {"hd-audio"},
         true, 2.0, "10.0.26100"});
    add({"IntcAudioBus", "IntcAudioBus.sys", "Intel", DeviceClass::Audio,
         {"INTELAUDIO\\FUNC_01&VEN_8086"},
         {"hd-audio", "voice-wake", "dsp"},
         true, 18.0, "10.29.0"});
    add({"nvhda64v", "nvhda64v.sys", "NVIDIA", DeviceClass::Audio,
         {"HDAUDIO\\FUNC_01&VEN_10DE"},
         {"hd-audio", "hdmi-audio"},
         true, 8.0, "1.4.0"});
    add({"cavkhda", "cavkhda.sys", "Conexant", DeviceClass::Audio,
         {"HDAUDIO\\FUNC_01&VEN_11D4"},
         {"hd-audio", "smart-amp"},
         true, 16.0, "8.66.0"});

    // Input.
    add({"SynTP", "SynTP.sys", "Synaptics", DeviceClass::Input,
         {"ACPI\\SYN0500"},
         {"precision-touchpad", "gestures", "palm-reject"},
         true, 12.0, "19.5.0"});
    add({"ETD", "ETD.sys", "Elan", DeviceClass::Input,
         {"ACPI\\ETD0100"},
         {"precision-touchpad", "gestures"},
         true, 10.0, "15.6.0"});
    add({"razerkbd", "razerkbd.sys", "Razer", DeviceClass::Input,
         {"USB\\VID_1532&PID_0A0D"},
         {"hid-keyboard", "rgb", "macro"},
         true, 9.0, "3.8.0"});
    add({"kbdhid", "kbdhid.sys", "Microsoft", DeviceClass::Input,
         {"USB\\Class_03&SubClass_01"},
         {"hid-keyboard"},
         true, 1.5, "10.0.26100"});
    add({"mouhid", "mouhid.sys", "Microsoft", DeviceClass::Input,
         {"USB\\Class_03&SubClass_02"},
         {"hid-mouse"},
         true, 1.5, "10.0.26100"});
    add({"i8042prt", "i8042prt.sys", "Microsoft", DeviceClass::Input,
         {"ACPI\\PNP0303", "ACPI\\PNP0F03"},
         {"ps2"},
         true, 1.0, "10.0.26100"});
    add({"goodix", "goodix.sys", "Goodix", DeviceClass::Input,
         {"USB\\VID_0416&PID_2345"},
         {"fingerprint", "secure-bio"},
         true, 14.0, "3.0.0"});

    // Chipset.
    add({"amdgpio2", "amdgpio2.sys", "AMD", DeviceClass::Chipset,
         {"ACPI\\AMDIF030"},
         {"gpio", "pin-ctrl"},
         true, 6.0, "2.2.0"});
    add({"amdpsp", "amdpsp.sys", "AMD", DeviceClass::Chipset,
         {"PCI\\VEN_1022&DEV_1456"},
         {"psp", "attestation"},
         true, 7.0, "5.17.0"});
    add({"intelpep", "intelpep.sys", "Intel", DeviceClass::Power,
         {"ACPI\\INT33A1"},
         {"power-engine", "c-states", "p-states"},
         true, 5.0, "10.0.0"});
    add({"amdppm", "amdppm.sys", "AMD", DeviceClass::Power,
         {"ACPI\\Processor"},
         {"power-engine", "c-states", "p-states"},
         true, 5.0, "10.0.0"});
    add({"cmbatt", "cmbatt.sys", "Microsoft", DeviceClass::Power,
         {"ACPI\\PNP0C0A"},
         {"battery", "charge-threshold"},
         true, 2.0, "10.0.26100"});
    add({"TbtBusDrv", "TbtBusDrv.sys", "Intel", DeviceClass::Thunderbolt,
         {"PCI\\VEN_8086&DEV_1137", "PCI\\VEN_8086&DEV_09D2"},
         {"thunderbolt4", "pcie-tunneling", "display-tunneling"},
         true, 11.0, "1.41.0"});
    add({"usbxhci", "usbxhci.sys", "Microsoft", DeviceClass::Thunderbolt,
         {"PCI\\CC_0C0330"},
         {"usb3", "xhci"},
         true, 3.0, "10.0.26100"});

    // Security.
    add({"tpm", "tpm.sys", "Microsoft", DeviceClass::Security,
         {"ACPI\\PNP0C31", "ACPI\\MSFT0101"},
         {"tpm2", "bitlocker", "measured-boot"},
         true, 3.0, "10.0.26100"});
    add({"iclsClient", "iclsClient.sys", "Intel", DeviceClass::Security,
         {"ACPI\\INT3400"},
         {"txe", "attestation"},
         true, 4.0, "4.0.0"});
    return reg;
}

std::vector<HardwareDevice> DriverRegistry::profile_a() {
    return {
        {"GeForce RTX 4090", "PCI\\VEN_10DE&DEV_2684", DeviceClass::Display,
         {"d3d12", "vulkan", "raytracing", "dlss", "hdmi-audio"}},
        {"Intel VMD controller", "PCI\\VEN_8086&DEV_09AB", DeviceClass::Storage,
         {"nvme", "raid", "vmd"}},
        {"Intel I226-V 2.5GbE", "PCI\\VEN_8086&DEV_15F3", DeviceClass::Network,
         {"ethernet", "wol"}},
        {"Killer WiFi 6E", "PCI\\VEN_8086&DEV_7A70", DeviceClass::Network,
         {"wifi6e", "wpa3"}},
        {"Realtek ALC4080", "HDAUDIO\\FUNC_01&VEN_10EC&DEV_1168", DeviceClass::Audio,
         {"hd-audio", "dts"}},
        {"Razer keyboard", "USB\\VID_1532&PID_0A0D", DeviceClass::Input,
         {"hid-keyboard", "rgb"}},
        {"Thunderbolt 4 host", "PCI\\VEN_8086&DEV_1137", DeviceClass::Thunderbolt,
         {"thunderbolt4", "display-tunneling"}},
        {"Intel PEP", "ACPI\\INT33A1", DeviceClass::Power, {"power-engine"}},
    };
}

std::vector<HardwareDevice> DriverRegistry::profile_b() {
    return {
        {"GeForce RTX 4070 Laptop", "PCI\\VEN_10DE&DEV_28A0", DeviceClass::Display,
         {"d3d12", "vulkan", "dlss"}},
        {"Radeon 780M iGPU", "PCI\\VEN_1002&DEV_15BF", DeviceClass::Display,
         {"d3d12", "vulkan"}},
        {"AMD SATA controller", "PCI\\VEN_1022&DEV_7901", DeviceClass::Storage,
         {"sata", "raid"}},
        {"MediaTek WiFi 6E", "PCI\\VEN_14C3&DEV_7927", DeviceClass::Network,
         {"wifi6e", "bluetooth"}},
        {"Conexant audio", "HDAUDIO\\FUNC_01&VEN_11D4", DeviceClass::Audio,
         {"hd-audio", "smart-amp"}},
        {"Elan touchpad", "ACPI\\ETD0100", DeviceClass::Input,
         {"precision-touchpad", "gestures"}},
        {"AMD GPIO", "ACPI\\AMDIF030", DeviceClass::Chipset, {"gpio"}},
        {"AMD PSP", "PCI\\VEN_1022&DEV_1456", DeviceClass::Chipset, {"psp"}},
    };
}

std::vector<HardwareDevice> DriverRegistry::profile_c() {
    return {
        {"Intel Iris Xe", "PCI\\VEN_8086&DEV_9A49", DeviceClass::Display,
         {"d3d12", "quicksync"}},
        {"Intel RST controller", "PCI\\VEN_8086&DEV_2822", DeviceClass::Storage,
         {"sata", "optane"}},
        {"Intel AX211 WiFi", "PCI\\VEN_8086&DEV_7A70", DeviceClass::Network,
         {"wifi6e", "wpa3"}},
        {"Snapdragon X65 5G", "USB\\VID_05C6&PID_90DC", DeviceClass::Network,
         {"wwan5g", "gnss"}},
        {"Intel Smart Sound", "INTELAUDIO\\FUNC_01&VEN_8086", DeviceClass::Audio,
         {"hd-audio", "dsp"}},
        {"Precision touchpad", "ACPI\\SYN0500", DeviceClass::Input,
         {"precision-touchpad", "palm-reject"}},
        {"TPM 2.0", "ACPI\\MSFT0101", DeviceClass::Security, {"tpm2", "bitlocker"}},
        {"Thunderbolt 4 host", "PCI\\VEN_8086&DEV_1137", DeviceClass::Thunderbolt,
         {"thunderbolt4", "pcie-tunneling"}},
        {"Control Method Battery", "ACPI\\PNP0C0A", DeviceClass::Power, {"battery"}},
        {"Fingerprint reader", "USB\\VID_0416&PID_2345", DeviceClass::Input,
         {"fingerprint"}},
    };
}

std::vector<HardwareDevice> DriverRegistry::profile_d() {
    return {
        {"RTX A5500", "PCI\\VEN_10DE&DEV_1EB1", DeviceClass::Display,
         {"d3d12", "vulkan", "ecc"}},
        {"Intel X710 10GbE", "PCI\\VEN_8086&DEV_1889", DeviceClass::Network,
         {"ethernet", "sr-iov"}},
        {"Intel RST RAID 1", "PCI\\VEN_8086&DEV_2822", DeviceClass::Storage,
         {"raid", "trim"}},
        {"Snapdragon X65 5G", "USB\\VID_05C6&PID_90DC", DeviceClass::Network,
         {"wwan5g"}},
        {"TPM 2.0", "ACPI\\PNP0C31", DeviceClass::Security, {"tpm2", "bitlocker"}},
        {"Intel TXE", "ACPI\\INT3400", DeviceClass::Security, {"txe", "attestation"}},
        {"LSI SAS HBA", "PCI\\VEN_1000&DEV_0072", DeviceClass::Storage,
         {"sas", "smart"}},
        {"Thunderbolt 4 host", "PCI\\VEN_8086&DEV_1137", DeviceClass::Thunderbolt,
         {"thunderbolt4", "display-tunneling"}},
    };
}

std::vector<HardwareDevice> DriverRegistry::profile_e() {
    return {
        {"UHD Graphics 730", "PCI\\VEN_8086&DEV_46A6", DeviceClass::Display,
         {"d3d12", "quicksync"}},
        {"SATA AHCI controller", "PCI\\CC_010601", DeviceClass::Storage,
         {"sata", "trim"}},
        {"Realtek 8111H 1GbE", "PCI\\VEN_10EC&DEV_8168", DeviceClass::Network,
         {"ethernet", "wol"}},
        {"Realtek ALC897", "HDAUDIO\\FUNC_01&VEN_10EC", DeviceClass::Audio,
         {"hd-audio"}},
        {"PS/2 keyboard", "ACPI\\PNP0303", DeviceClass::Input, {"ps2"}},
        {"USB keyboard", "USB\\Class_03&SubClass_01", DeviceClass::Input,
         {"hid-keyboard"}},
        {"USB mouse", "USB\\Class_03&SubClass_02", DeviceClass::Input,
         {"hid-mouse"}},
    };
}

std::vector<std::pair<std::string, std::vector<HardwareDevice>>> DriverRegistry::book_profiles() {
    return {
        {"A High-Performance Gaming Desktop", profile_a()},
        {"B AMD Gaming Laptop", profile_b()},
        {"C Ultrabook / Business Laptop", profile_c()},
        {"D Workstation", profile_d()},
        {"E Budget / Entry Desktop", profile_e()},
    };
}

std::string format_match(const HardwareDevice& device, const MatchResult& m) {
    std::ostringstream os;
    os << device.name << " [" << device.hardware_id << "] -> ";
    if (m.found) {
        os << m.binary << " via " << to_string(m.kind) << " score " << m.score;
    } else {
        os << "NO MATCH (" << m.reason << ")";
    }
    if (!m.missing_capabilities.empty()) {
        os << " missing:";
        for (const auto& c : m.missing_capabilities) os << " " << c;
    }
    return os.str();
}

std::string format_catalog(const DriverRegistry& reg) {
    std::ostringstream os;
    os << "id            class        whql  size_mb  ids  caps  version\n";
    for (const auto& d : reg.all()) {
        char line[160];
        std::snprintf(line, sizeof(line), "%-13s %-12s %-5s %7.1f  %3zu  %4zu  %s\n",
                      d.id.c_str(), to_string(d.device_class), d.whql ? "yes" : "no",
                      d.size_mb, d.hardware_ids.size(), d.capabilities.size(),
                      d.version.c_str());
        os << line;
    }
    return os.str();
}

} // namespace nebula::drivers

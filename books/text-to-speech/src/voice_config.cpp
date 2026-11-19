// MIT License - (c) Anthony Matarazzo 2026
#include "voice_config.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace tts {

namespace {

constexpr float kPi = 3.14159265358979323846f;

// Range table: keep in sync with VoiceConfig field order below.
struct Range {
    float lo;
    float hi;
};

Range range_for(const std::string& n) {
    if (n == "vocal_tract_length_cm") return {14.0f, 19.0f};
    if (n == "vocal_fold_mass") return {0.4f, 1.6f};
    if (n == "subglottal_pressure") return {0.3f, 1.4f};
    if (n == "breathiness") return {0.0f, 1.0f};
    if (n == "nasality") return {0.0f, 1.0f};
    if (n == "harmonic_richness") return {0.2f, 1.6f};
    if (n == "articulation_precision") return {0.0f, 1.0f};
    if (n == "vibrato_rate") return {0.0f, 8.0f};
    if (n == "vibrato_depth") return {0.0f, 2.0f};
    if (n == "jitter") return {0.0f, 0.1f};
    if (n == "shimmer") return {0.0f, 0.1f};
    if (n == "pitch_hz") return {0.0f, 600.0f};
    if (n == "speaking_rate") return {0.25f, 4.0f};
    return {0.0f, 1.0f};  // formants and gender axis
}

float clamp_to(const std::string& n, float v) {
    const Range r = range_for(n);
    return std::min(r.hi, std::max(r.lo, v));
}

VoiceConfig make(const std::string& name, float tract, float mass, float pressure,
                 float breath, float nasal, float rich, float artic,
                 float f1, float f2, float f3, float vib_rate, float vib_depth,
                 float jit, float shim) {
    VoiceConfig v;
    v.name = name;
    v.vocal_tract_length_cm = tract;
    v.vocal_fold_mass = mass;
    v.subglottal_pressure = pressure;
    v.breathiness = breath;
    v.nasality = nasal;
    v.harmonic_richness = rich;
    v.articulation_precision = artic;
    v.formant_scaling = {f1, f2, f3};
    v.vibrato_rate = vib_rate;
    v.vibrato_depth = vib_depth;
    v.jitter = jit;
    v.shimmer = shim;
    return v;
}

}  // namespace

const std::vector<ParamInfo>& VoiceConfig::parameter_info() {
    static const std::vector<ParamInfo> info = {
        {"vocal_tract_length_cm", 14.0f, 19.0f, "cm",
         "Length of the tube from glottis to lips; shorter is brighter"},
        {"vocal_fold_mass", 0.4f, 1.6f, "relative",
         "Fold mass; heavier folds vibrate slower and lower"},
        {"subglottal_pressure", 0.3f, 1.4f, "relative",
         "Driving pressure below the folds; sets loudness and effort"},
        {"breathiness", 0.0f, 1.0f, "0..1",
         "Aspiration noise mixed with the periodic source"},
        {"nasality", 0.0f, 1.0f, "0..1",
         "Coupling into the nasal branch, adds a pole near 250 Hz"},
        {"harmonic_richness", 0.2f, 1.6f, "relative",
         "Spectral tilt control; above 1.0 tilts energy up"},
        {"articulation_precision", 0.0f, 1.0f, "0..1",
         "How sharply formants reach their targets"},
        {"formant_f1", 0.5f, 1.6f, "scale",
         "Multiplier on the first formant target"},
        {"formant_f2", 0.5f, 1.6f, "scale",
         "Multiplier on the second formant target"},
        {"formant_f3", 0.5f, 1.6f, "scale",
         "Multiplier on the third formant target"},
        {"vibrato_rate", 0.0f, 8.0f, "Hz",
         "Frequency of periodic pitch modulation"},
        {"vibrato_depth", 0.0f, 2.0f, "semitones",
         "Peak deviation of the vibrato"},
        {"jitter", 0.0f, 0.1f, "fraction",
         "Random cycle to cycle period change"},
        {"shimmer", 0.0f, 0.1f, "fraction",
         "Random cycle to cycle amplitude change"},
        {"pitch_hz", 0.0f, 600.0f, "Hz",
         "Explicit fundamental; 0 means derive it from mass and length"},
        {"speaking_rate", 0.25f, 4.0f, "multiplier",
         "Duration scale applied to every phoneme"},
        {"gender_axis_0", 0.0f, 1.0f, "0..1", "First latent gender coordinate"},
        {"gender_axis_1", 0.0f, 1.0f, "0..1", "Second latent gender coordinate"},
        {"gender_axis_2", 0.0f, 1.0f, "0..1", "Third latent gender coordinate"},
        {"gender_axis_3", 0.0f, 1.0f, "0..1", "Fourth latent gender coordinate"},
        {"gender_axis_4", 0.0f, 1.0f, "0..1", "Fifth latent gender coordinate"},
        {"gender_axis_5", 0.0f, 1.0f, "0..1", "Sixth latent gender coordinate"},
        {"gender_axis_6", 0.0f, 1.0f, "0..1", "Seventh latent gender coordinate"},
        {"gender_axis_7", 0.0f, 1.0f, "0..1", "Eighth latent gender coordinate"},
    };
    return info;
}

float VoiceConfig::get_parameter(const std::string& n) const {
    if (n == "vocal_tract_length_cm") return vocal_tract_length_cm;
    if (n == "vocal_fold_mass") return vocal_fold_mass;
    if (n == "subglottal_pressure") return subglottal_pressure;
    if (n == "breathiness") return breathiness;
    if (n == "nasality") return nasality;
    if (n == "harmonic_richness") return harmonic_richness;
    if (n == "articulation_precision") return articulation_precision;
    if (n == "formant_f1") return formant_scaling[0];
    if (n == "formant_f2") return formant_scaling[1];
    if (n == "formant_f3") return formant_scaling[2];
    if (n == "vibrato_rate") return vibrato_rate;
    if (n == "vibrato_depth") return vibrato_depth;
    if (n == "jitter") return jitter;
    if (n == "shimmer") return shimmer;
    if (n == "pitch_hz") return pitch_hz;
    if (n == "speaking_rate") return speaking_rate;
    for (int i = 0; i < 8; ++i) {
        if (n == "gender_axis_" + std::to_string(i)) return gender_projection[static_cast<size_t>(i)];
    }
    throw std::invalid_argument("unknown voice parameter: " + n);
}

bool VoiceConfig::set_parameter(const std::string& n, float value) {
    if (n == "vocal_tract_length_cm") vocal_tract_length_cm = value;
    else if (n == "vocal_fold_mass") vocal_fold_mass = value;
    else if (n == "subglottal_pressure") subglottal_pressure = value;
    else if (n == "breathiness") breathiness = value;
    else if (n == "nasality") nasality = value;
    else if (n == "harmonic_richness") harmonic_richness = value;
    else if (n == "articulation_precision") articulation_precision = value;
    else if (n == "formant_f1") formant_scaling[0] = value;
    else if (n == "formant_f2") formant_scaling[1] = value;
    else if (n == "formant_f3") formant_scaling[2] = value;
    else if (n == "vibrato_rate") vibrato_rate = value;
    else if (n == "vibrato_depth") vibrato_depth = value;
    else if (n == "jitter") jitter = value;
    else if (n == "shimmer") shimmer = value;
    else if (n == "pitch_hz") pitch_hz = value;
    else if (n == "speaking_rate") speaking_rate = value;
    else {
        bool matched = false;
        for (int i = 0; i < 8; ++i) {
            if (n == "gender_axis_" + std::to_string(i)) {
                gender_projection[static_cast<size_t>(i)] = value;
                matched = true;
            }
        }
        if (!matched) return false;
    }
    return true;
}

float VoiceConfig::fundamental_hz() const {
    if (pitch_hz > 0.0f) return pitch_hz;
    // Reference adult: 165 Hz at 16.5 cm tract and unit fold mass.
    // Pitch falls with the square root of mass and rises as the tract shortens,
    // which is the same relationship the book uses for its derived pitch.
    const float mass = std::max(0.2f, vocal_fold_mass);
    const float tract = std::max(8.0f, vocal_tract_length_cm);
    float f0 = 165.0f * (16.5f / tract) / std::sqrt(mass);
    f0 *= 1.0f + 0.08f * (harmonic_richness - 1.0f);
    (void)kPi;
    return std::min(600.0f, std::max(60.0f, f0));
}

VoiceConfig VoiceConfig::clamped() const {
    VoiceConfig v = *this;
    for (const ParamInfo& p : parameter_info()) {
        v.set_parameter(p.name, clamp_to(p.name, get_parameter(p.name)));
    }
    return v;
}

bool VoiceConfig::validate(std::string* reason) const {
    for (const ParamInfo& p : parameter_info()) {
        const float v = get_parameter(p.name);
        if (!(v >= p.min_value && v <= p.max_value)) {
            if (reason) {
                std::ostringstream os;
                os << p.name << "=" << v << " outside [" << p.min_value << ", "
                   << p.max_value << "]";
                *reason = os.str();
            }
            return false;
        }
    }
    if (vocal_tract_length_cm <= 0.0f) {
        if (reason) *reason = "vocal_tract_length_cm must be positive";
        return false;
    }
    return true;
}

VoiceConfig VoiceConfig::morph(const VoiceConfig& target, float t) const {
    t = std::min(1.0f, std::max(0.0f, t));
    VoiceConfig out;
    for (const ParamInfo& p : parameter_info()) {
        const float a = get_parameter(p.name);
        const float b = target.get_parameter(p.name);
        out.set_parameter(p.name, a + (b - a) * t);
    }
    out.name = (t < 0.5f ? name : target.name) + "+morph";
    return out;
}

std::vector<std::string> VoiceConfig::preset_ids() {
    return {"neutral", "bright", "deep", "breathy", "nasal", "elderly",
            "child",   "singer", "whisper", "robotic", "french", "aged"};
}

VoiceConfig VoiceConfig::preset(const std::string& id) {
    // Values transcribed from the voice catalog, chapter 8.1.
    if (id == "neutral") {
        VoiceConfig v = make("Neutral", 16.5f, 1.00f, 1.00f, 0.02f, 0.03f, 1.00f, 0.95f,
                             1.00f, 1.00f, 1.00f, 0.00f, 0.0f, 0.010f, 0.010f);
        return v;
    }
    if (id == "bright") {
        return make("Bright", 14.8f, 0.85f, 1.05f, 0.05f, 0.02f, 1.20f, 0.98f,
                    1.12f, 1.08f, 1.04f, 0.00f, 0.0f, 0.008f, 0.007f);
    }
    if (id == "deep" || id == "dark") {
        return make("Deep", 18.2f, 1.25f, 1.10f, 0.01f, 0.01f, 0.85f, 0.92f,
                    0.82f, 0.88f, 0.94f, 0.08f, 0.15f, 0.012f, 0.015f);
    }
    if (id == "breathy") {
        return make("Breathy", 16.0f, 0.95f, 0.85f, 0.65f, 0.08f, 0.75f, 0.88f,
                    0.95f, 0.98f, 1.00f, 0.05f, 0.12f, 0.020f, 0.025f);
    }
    if (id == "nasal") {
        return make("Nasal", 16.5f, 1.00f, 1.00f, 0.03f, 0.55f, 0.90f, 0.93f,
                    0.92f, 1.02f, 1.05f, 0.00f, 0.0f, 0.010f, 0.010f);
    }
    if (id == "elderly" || id == "aged") {
        return make("Elderly", 16.8f, 1.15f, 1.05f, 0.35f, 0.12f, 0.70f, 0.75f,
                    0.88f, 0.92f, 0.96f, 0.12f, 0.30f, 0.035f, 0.040f);
    }
    if (id == "child") {
        return make("Child", 12.5f, 0.55f, 1.15f, 0.12f, 0.06f, 1.35f, 0.85f,
                    1.35f, 1.28f, 1.20f, 0.00f, 0.0f, 0.025f, 0.020f);
    }
    if (id == "singer") {
        return make("Singer", 16.2f, 1.05f, 1.25f, 0.01f, 0.02f, 1.15f, 0.97f,
                    1.05f, 1.03f, 1.02f, 0.22f, 0.45f, 0.005f, 0.008f);
    }
    if (id == "whisper") {
        return make("Whisper", 16.5f, 1.00f, 0.40f, 0.95f, 0.05f, 0.30f, 0.85f,
                    0.90f, 0.95f, 1.00f, 0.00f, 0.0f, 0.050f, 0.060f);
    }
    if (id == "robotic") {
        VoiceConfig v = make("Robotic", 16.5f, 1.00f, 1.00f, 0.00f, 0.00f, 1.20f, 1.00f,
                             1.00f, 1.00f, 1.00f, 0.00f, 0.0f, 0.000f, 0.000f);
        v.gender_projection[0] = 1.0f;
        return v;
    }
    if (id == "french") {
        return make("FrenchAccent", 16.3f, 1.00f, 1.02f, 0.08f, 0.25f, 1.05f, 0.96f,
                    0.98f, 1.05f, 1.02f, 0.03f, 0.06f, 0.010f, 0.010f);
    }
    throw std::invalid_argument("unknown voice preset: " + id);
}

VoiceConfig VoiceConfig::neutral() { return preset("neutral"); }
VoiceConfig VoiceConfig::bright_voice() { return preset("bright"); }
VoiceConfig VoiceConfig::dark_voice() { return preset("deep"); }
VoiceConfig VoiceConfig::breathy_voice() { return preset("breathy"); }

std::string VoiceConfig::describe() const {
    std::ostringstream os;
    os.setf(std::ios::fixed);
    os.precision(2);
    os << name << " [tract " << vocal_tract_length_cm << " cm, mass " << vocal_fold_mass
       << ", pressure " << subglottal_pressure << ", breath " << breathiness << ", nasal "
       << nasality << ", rich " << harmonic_richness << ", artic "
       << articulation_precision << ", F " << formant_scaling[0] << "/"
       << formant_scaling[1] << "/" << formant_scaling[2] << ", vib " << vibrato_rate
       << " Hz x " << vibrato_depth << " st, jit " << jitter << ", shim " << shimmer
       << ", f0 " << fundamental_hz() << " Hz]";
    return os.str();
}

bool VoiceConfig::operator==(const VoiceConfig& other) const {
    if (name != other.name) return false;
    for (const ParamInfo& p : parameter_info()) {
        if (get_parameter(p.name) != other.get_parameter(p.name)) return false;
    }
    return true;
}

}  // namespace tts

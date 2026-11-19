// MIT License - (c) Anthony Matarazzo 2026
//
// voice_config.hpp - continuous, gender-indirect voice description.
//
// Implements the VoiceConfig structure documented in chapter 9.1 of
// "Text to Speech with Adaptive Voices": physical parameters, acoustic
// parameters, an eight-number gender axis, per-formant biases, temporal
// micro-variation and the twelve voice presets from chapter 8.1.
//
// Everything here is plain C++17 + STL and fully deterministic.
#pragma once

#include <array>
#include <string>
#include <vector>

namespace tts {

// Description of one tunable parameter, used by the teaching program and by
// validate()/clamped().
struct ParamInfo {
    std::string name;
    float min_value;
    float max_value;
    std::string unit;
    std::string meaning;
};

// The complete parameter set of a voice. Every field is a continuous axis:
// there is no male/female switch, "gender" is a region of this space.
struct VoiceConfig {
    std::string name = "Neutral";

    // ---- Physical parameters (anatomically plausible) ---------------------
    float vocal_tract_length_cm = 16.5f;   // 14.0 - 19.0 cm
    float vocal_fold_mass = 1.0f;          // relative, 1.0 = average adult
    float subglottal_pressure = 1.0f;      // relative, 1.0 = comfortable

    // ---- Acoustic parameters ---------------------------------------------
    float breathiness = 0.02f;             // 0 = clean, 1 = all breath
    float nasality = 0.03f;                // 0 = oral, 1 = full nasal
    float harmonic_richness = 1.0f;        // 0.2 dark .. 1.6 bright
    float articulation_precision = 0.95f;  // 0 = slurred, 1 = crisp

    // ---- Gender-continuous latent axis (8 numbers, chapter 9.1) ----------
    std::array<float, 8> gender_projection{};

    // ---- Formant biases, one per formant ---------------------------------
    std::array<float, 3> formant_scaling{1.0f, 1.0f, 1.0f};

    // ---- Temporal characteristics ----------------------------------------
    float vibrato_rate = 0.0f;    // Hz, 0 = off
    float vibrato_depth = 0.0f;   // semitones of peak deviation
    float jitter = 0.01f;         // cycle to cycle period variation (fraction)
    float shimmer = 0.01f;        // cycle to cycle amplitude variation

    // ---- Derived or overridden controls ----------------------------------
    float pitch_hz = 0.0f;        // 0 = derive from fold mass and tract length
    float speaking_rate = 1.0f;   // 1.0 = normal, 2.0 = twice as fast

    // ---- Parameter introspection -----------------------------------------
    // One entry per tunable parameter, including formant_f1..f3 and
    // gender_axis_0..7.
    static const std::vector<ParamInfo>& parameter_info();
    float get_parameter(const std::string& name) const;
    bool set_parameter(const std::string& name, float value);

    // ---- Derived values ---------------------------------------------------
    // Fundamental frequency implied by fold mass and tract length, unless
    // pitch_hz carries an explicit override.
    float fundamental_hz() const;

    // Copy with every field clamped into its documented range.
    VoiceConfig clamped() const;
    // True when every field is inside its documented range; on failure the
    // offending parameter is written to *reason when reason is not null.
    bool validate(std::string* reason = nullptr) const;

    // ---- Morphing ---------------------------------------------------------
    // Linear blend from *this (t = 0) to target (t = 1). t is clamped.
    VoiceConfig morph(const VoiceConfig& target, float t) const;

    // ---- Presets ----------------------------------------------------------
    static std::vector<std::string> preset_ids();
    // Throws std::invalid_argument for an unknown id.
    static VoiceConfig preset(const std::string& id);

    // Names used by the book's API reference (chapter 9.1).
    static VoiceConfig neutral();
    static VoiceConfig bright_voice();
    static VoiceConfig dark_voice();
    static VoiceConfig breathy_voice();

    // ---- Reporting --------------------------------------------------------
    std::string describe() const;

    bool operator==(const VoiceConfig& other) const;
    bool operator!=(const VoiceConfig& other) const { return !(*this == other); }
};

}  // namespace tts

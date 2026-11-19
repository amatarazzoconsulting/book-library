// MIT License - (c) Anthony Matarazzo 2026
//
// test_voice_config.cpp - parameter table, presets, validation, morphing.
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

#include "voice_config.hpp"

using namespace tts;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}

bool near(float a, float b, float tol) { return std::fabs(a - b) <= tol; }

}  // namespace

int main() {
    // ---- Parameter table ------------------------------------------------
    const auto& info = VoiceConfig::parameter_info();
    check(info.size() == 24, "parameter table lists 24 tunable parameters");
    check(info.front().name == "vocal_tract_length_cm", "table starts with tract length");
    check(info.back().name == "gender_axis_7", "table ends with the eighth gender axis");

    bool all_ranged = true;
    for (const ParamInfo& p : info) {
        if (!(p.min_value < p.max_value)) all_ranged = false;
        if (p.unit.empty() || p.meaning.empty()) all_ranged = false;
    }
    check(all_ranged, "every parameter has a range, a unit and a meaning");

    // ---- Presets --------------------------------------------------------
    const std::vector<std::string> ids = VoiceConfig::preset_ids();
    check(ids.size() == 12, "twelve presets are catalogued");

    bool all_load = true;
    for (const std::string& id : ids) {
        try {
            const VoiceConfig v = VoiceConfig::preset(id);
            if (v.name.empty()) all_load = false;
        } catch (const std::exception&) {
            all_load = false;
        }
    }
    check(all_load, "every catalogued preset loads");

    const VoiceConfig neutral = VoiceConfig::preset("neutral");
    check(near(neutral.vocal_tract_length_cm, 16.5f, 1e-6f), "neutral tract is 16.5 cm");
    check(near(neutral.fundamental_hz(), 165.0f, 0.5f), "neutral f0 is 165 Hz");

    // A child's shorter tract and lighter folds must raise the pitch above the
    // adult reference; a deep voice must sit below it.
    const VoiceConfig child = VoiceConfig::preset("child");
    const VoiceConfig deep = VoiceConfig::preset("deep");
    check(child.fundamental_hz() > neutral.fundamental_hz(), "child pitch above neutral");
    check(deep.fundamental_hz() < neutral.fundamental_hz(), "deep pitch below neutral");
    check(child.vocal_tract_length_cm < neutral.vocal_tract_length_cm,
          "child tract is shorter than the adult reference");
    check(deep.vocal_tract_length_cm > neutral.vocal_tract_length_cm,
          "deep tract is longer than the adult reference");

    // Pitch override wins over the derived value.
    VoiceConfig pinned = neutral;
    pinned.pitch_hz = 220.0f;
    check(near(pinned.fundamental_hz(), 220.0f, 1e-4f), "pitch_hz overrides the derived f0");

    // Aliases documented in the API reference.
    check(VoiceConfig::dark_voice() == VoiceConfig::preset("deep"), "dark_voice is the deep preset");
    check(VoiceConfig::breathy_voice() == VoiceConfig::preset("breathy"),
          "breathy_voice is the breathy preset");

    // ---- Unknown ids ----------------------------------------------------
    bool threw = false;
    try {
        (void)VoiceConfig::preset("martian");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "an unknown preset id throws invalid_argument");

    // ---- get / set ------------------------------------------------------
    VoiceConfig v = neutral;
    check(near(v.get_parameter("breathiness"), 0.02f, 1e-6f), "get_parameter reads breathiness");
    check(v.set_parameter("breathiness", 0.4f), "set_parameter accepts a known name");
    check(near(v.breathiness, 0.4f, 1e-6f), "set_parameter writes the field");
    check(v.set_parameter("formant_f2", 1.1f), "set_parameter accepts formant_f2");
    check(near(v.formant_scaling[1], 1.1f, 1e-6f), "formant_f2 maps to the second bias");
    check(v.set_parameter("gender_axis_7", 0.75f), "set_parameter accepts the last gender axis");
    check(near(v.gender_projection[7], 0.75f, 1e-6f), "gender_axis_7 maps to the last slot");
    check(!v.set_parameter("warmth", 0.5f), "set_parameter rejects an unknown name");

    threw = false;
    try {
        (void)v.get_parameter("warmth");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "get_parameter throws on an unknown name");

    // Every field must be reachable through the generic accessors, or morph and
    // clamped would silently skip it.
    bool round_trip = true;
    for (const ParamInfo& p : info) {
        VoiceConfig w = neutral;
        w.set_parameter(p.name, p.min_value);
        if (!near(w.get_parameter(p.name), p.min_value, 1e-6f)) round_trip = false;
    }
    check(round_trip, "every table entry round trips through set_parameter");

    // ---- Validation -----------------------------------------------------
    std::string reason;
    check(neutral.validate(&reason), "neutral validates");
    check(reason.empty(), "a passing validation writes no reason");

    VoiceConfig bad = neutral;
    bad.breathiness = 2.0f;
    reason.clear();
    check(!bad.validate(&reason), "breathiness above 1.0 fails validation");
    check(reason.find("breathiness") != std::string::npos, "the reason names the field");

    check(!bad.validate(nullptr), "validate tolerates a null reason pointer");

    VoiceConfig bad_tract = neutral;
    bad_tract.vocal_tract_length_cm = 30.0f;
    check(!bad_tract.validate(nullptr), "an out of range tract fails validation");

    // The child preset is deliberately outside the adult range, so it must be
    // reported and repaired by clamped().
    check(!child.validate(nullptr), "the child preset sits outside the adult range");
    const VoiceConfig fixed = child.clamped();
    check(fixed.validate(nullptr), "clamped() repairs an out of range voice");
    check(near(fixed.vocal_tract_length_cm, 14.0f, 1e-5f), "clamped() lifts the tract to 14 cm");
    check(near(fixed.vocal_fold_mass, child.vocal_fold_mass, 1e-6f),
          "clamped() leaves in range fields untouched");

    VoiceConfig extreme = neutral;
    extreme.harmonic_richness = 9.0f;
    extreme.jitter = -1.0f;
    extreme.speaking_rate = 0.01f;
    const VoiceConfig repaired = extreme.clamped();
    check(near(repaired.harmonic_richness, 1.6f, 1e-5f), "clamped() caps richness at 1.6");
    check(near(repaired.jitter, 0.0f, 1e-6f), "clamped() lifts negative jitter to 0");
    check(near(repaired.speaking_rate, 0.25f, 1e-5f), "clamped() lifts rate to 0.25");
    check(repaired.validate(nullptr), "the repaired voice validates");

    // ---- Morphing -------------------------------------------------------
    const VoiceConfig a = VoiceConfig::preset("child");
    const VoiceConfig b = VoiceConfig::preset("elderly");
    const VoiceConfig mid = a.morph(b, 0.5f);
    check(near(mid.vocal_tract_length_cm, (a.vocal_tract_length_cm + b.vocal_tract_length_cm) * 0.5f,
               1e-4f),
          "morph at 0.5 lands halfway on tract length");
    check(near(mid.breathiness, (a.breathiness + b.breathiness) * 0.5f, 1e-5f),
          "morph at 0.5 lands halfway on breathiness");
    check(mid.fundamental_hz() > b.fundamental_hz() && mid.fundamental_hz() < a.fundamental_hz(),
          "the morphed pitch lies between the two parents");

    const VoiceConfig at_zero = a.morph(b, 0.0f);
    check(near(at_zero.vocal_tract_length_cm, a.vocal_tract_length_cm, 1e-5f),
          "morph at 0 reproduces the source");
    const VoiceConfig at_one = a.morph(b, 1.0f);
    check(near(at_one.breathiness, b.breathiness, 1e-5f), "morph at 1 reproduces the target");

    // t is clamped, so these must not extrapolate.
    const VoiceConfig under = a.morph(b, -5.0f);
    const VoiceConfig over = a.morph(b, 5.0f);
    check(near(under.vocal_tract_length_cm, a.vocal_tract_length_cm, 1e-5f),
          "morph clamps t below 0");
    check(near(over.vocal_tract_length_cm, b.vocal_tract_length_cm, 1e-5f),
          "morph clamps t above 1");

    // Morphing a voice with itself is the identity.
    const VoiceConfig same = neutral.morph(neutral, 0.37f);
    check(near(same.fundamental_hz(), neutral.fundamental_hz(), 1e-3f),
          "morphing a voice with itself keeps its pitch");
    check(same.validate(nullptr), "the identity morph still validates");

    // ---- Equality and reporting -----------------------------------------
    check(VoiceConfig::preset("neutral") == VoiceConfig::preset("neutral"),
          "two neutral presets compare equal");
    check(VoiceConfig::preset("neutral") != VoiceConfig::preset("bright"),
          "neutral and bright compare unequal");
    check(neutral.describe().find("Neutral") != std::string::npos,
          "describe() starts from the voice name");

    std::cout << (failures == 0 ? "\nvoice_config: all checks passed\n"
                                : "\nvoice_config: FAILURES\n");
    return failures == 0 ? 0 : 1;
}

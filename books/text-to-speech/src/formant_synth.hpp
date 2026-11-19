// MIT License - (c) Anthony Matarazzo 2026
//
// formant_synth.hpp - source-filter formant synthesizer.
//
// The physical model of chapters 2.1 and 4.1, reduced to something that runs in
// real time on one core with no dependencies:
//
//   glottal source -> cascade of formant resonators -> nasal branch -> lip radiation
//
// The source is a periodic glottal flow pulse with jitter, shimmer and
// aspiration noise. The filter is four time varying two-pole resonators whose
// centre frequencies come from the phoneme timeline, scaled by the voice's
// formant biases and by vocal tract length. Output is mono float PCM in the
// range -1..1.
#pragma once

#include <cstdint>
#include <vector>

#include "prosody.hpp"
#include "voice_config.hpp"

namespace tts {

struct SynthOptions {
    int sample_rate = 48000;
    float gain = 0.8f;          // linear output gain before normalisation
    bool normalize = true;      // scale the peak to peak_target
    float peak_target = 0.9f;   // peak amplitude after normalisation
    float noise_floor = 0.0f;   // constant aspiration mixed in, 0..1
    float attack_ms = 8.0f;     // envelope attack per phoneme
    float release_ms = 12.0f;   // envelope release per phoneme
    bool nasal_branch = true;   // enable the nasal pole and zero
    uint32_t seed = 20260101u;  // seed for jitter, shimmer and noise
};

// Simple deterministic LCG. Not a quality generator: it is here so that every
// run of the synthesizer produces identical samples.
class Lcg {
public:
    explicit Lcg(uint32_t seed = 1u) : state_(seed ? seed : 1u) {}
    uint32_t next_u32() {
        state_ = state_ * 1664525u + 1013904223u;
        return state_;
    }
    // Uniform in [0, 1).
    float next_float() { return static_cast<float>(next_u32() >> 8) * (1.0f / 16777216.0f); }
    // Approximately normal, mean 0, standard deviation 1 (sum of 4 uniforms).
    float next_gauss() {
        const float s = next_float() + next_float() + next_float() + next_float();
        return (s - 2.0f) * 1.7320508f;
    }
    void reseed(uint32_t seed) { state_ = seed ? seed : 1u; }

private:
    uint32_t state_;
};

struct SynthStats {
    size_t samples = 0;
    double seconds = 0.0;
    float peak = 0.0f;
    float rms = 0.0f;
    size_t periods = 0;   // glottal cycles produced
    size_t phonemes = 0;
};

class FormantSynth {
public:
    FormantSynth() = default;
    explicit FormantSynth(const SynthOptions& options) : opt_(options) {}

    SynthOptions& options() { return opt_; }
    const SynthOptions& options() const { return opt_; }

    // Render a scheduled phoneme timeline. The sample rate of the timeline is
    // independent: durations are in seconds.
    std::vector<float> render(const std::vector<Phoneme>& timeline, const VoiceConfig& voice);

    // Render and fill statistics. Same samples as render().
    std::vector<float> render(const std::vector<Phoneme>& timeline, const VoiceConfig& voice,
                              SynthStats* stats);

    // Render a single phoneme of the given duration; handy for tests and for
    // sweeping one parameter at a time.
    std::vector<float> render_phoneme(const Phoneme& phoneme, const VoiceConfig& voice);

    // Fundamental frequency actually used, after vibrato and jitter.
    static float effective_f0(const VoiceConfig& voice, float time_s);

    // Reset filter and generator state (a new utterance starts clean).
    void reset();

private:
    struct Resonator {
        float a1 = 0.0f, a2 = 0.0f, gain = 1.0f;
        float y1 = 0.0f, y2 = 0.0f;
        void set(float freq, float bandwidth, float sample_rate);
        float process(float x) {
            const float y = gain * x + a1 * y1 + a2 * y2;
            y2 = y1;
            y1 = y;
            return y;
        }
        void clear() { y1 = y2 = 0.0f; }
    };

    SynthOptions opt_{};
    Lcg rng_{20260101u};
    Resonator r_[4];
    Resonator nasal_pole_;
    Resonator nasal_zero_;   // used as a one-pole shelf for the antiresonance
    float f_[4] = {500.0f, 1500.0f, 2500.0f, 3500.0f};
    float phase_ = 0.0f;
    float period_ = 1.0f / 165.0f;
    float last_flow_ = 0.0f;
    float lip_prev_ = 0.0f;
    float tilt_lp_ = 0.0f;
    float jitter_mul_ = 1.0f;
    float shimmer_mul_ = 1.0f;
    size_t periods_ = 0;

    void update_filters(const VoiceConfig& voice, float sample_rate);
    float source(float t, const VoiceConfig& voice, float target_f0, bool voiced,
                 float noise_amount, float amplitude);
    float tract(float x, const VoiceConfig& voice);
};

}  // namespace tts

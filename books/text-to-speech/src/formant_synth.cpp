// MIT License - (c) Anthony Matarazzo 2026
#include "formant_synth.hpp"

#include <algorithm>
#include <cmath>

namespace tts {

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;
constexpr int kControlBlock = 32;  // samples between filter coefficient updates

// Bandwidth of each formant at articulation_precision = 1.
constexpr float kBaseBandwidth[4] = {70.0f, 100.0f, 140.0f, 180.0f};
}  // namespace

void FormantSynth::Resonator::set(float freq, float bandwidth, float sample_rate) {
    const float nyquist = sample_rate * 0.5f;
    freq = std::min(nyquist * 0.95f, std::max(40.0f, freq));
    bandwidth = std::min(nyquist * 0.5f, std::max(20.0f, bandwidth));
    const float r = std::exp(-kPi * bandwidth / sample_rate);
    const float theta = kTwoPi * freq / sample_rate;
    a1 = 2.0f * r * std::cos(theta);
    a2 = -r * r;
    // Unity gain at DC, Klatt style: the resonance peak is then boosted by the
    // Q of the pole pair, which is exactly the formant emphasis we want. The
    // source spectrum supplies the matching high frequency roll off.
    gain = 1.0f - a1 - a2;
}

float FormantSynth::effective_f0(const VoiceConfig& voice, float time_s) {
    const float base = voice.fundamental_hz();
    if (voice.vibrato_rate <= 0.0f || voice.vibrato_depth <= 0.0f) return base;
    const float lfo = std::sin(kTwoPi * voice.vibrato_rate * time_s);
    // Depth is in semitones of peak deviation.
    return base * std::pow(2.0f, voice.vibrato_depth * lfo / 12.0f);
}

void FormantSynth::reset() {
    for (Resonator& r : r_) r.clear();
    nasal_pole_.clear();
    nasal_zero_.clear();
    phase_ = 0.0f;
    period_ = 1.0f / 165.0f;
    last_flow_ = 0.0f;
    lip_prev_ = 0.0f;
    tilt_lp_ = 0.0f;
    jitter_mul_ = 1.0f;
    shimmer_mul_ = 1.0f;
    periods_ = 0;
    rng_.reseed(opt_.seed);
}

void FormantSynth::update_filters(const VoiceConfig& voice, float sample_rate) {
    // Vocal tract length scales every formant inversely: a longer tube has
    // lower resonances. 16.5 cm is the reference.
    const float length_scale = 16.5f / std::max(8.0f, voice.vocal_tract_length_cm);

    // Low articulation precision widens the resonators, which smears the
    // spectral contrast between neighbouring phonemes.
    const float precision = std::min(1.0f, std::max(0.0f, voice.articulation_precision));
    const float bw_scale = 1.0f + (1.0f - precision) * 1.2f;

    for (int i = 0; i < 4; ++i) {
        const float bw = kBaseBandwidth[i] * bw_scale;
        r_[i].set(f_[i], bw, sample_rate);
    }

    if (opt_.nasal_branch && voice.nasality > 0.0f) {
        // The nasal cavity adds a pole near 250 Hz and an antiresonance near
        // 1000 Hz; both scale with the coupling amount.
        const float pole_hz = 250.0f * length_scale;
        const float zero_hz = 1000.0f * length_scale;
        nasal_pole_.set(pole_hz, 120.0f, sample_rate);
        // A one pole low pass stands in for the antiresonance shelf.
        const float r = std::exp(-kTwoPi * zero_hz / sample_rate);
        nasal_zero_.a1 = r;
        nasal_zero_.a2 = 0.0f;
        nasal_zero_.gain = 1.0f - r;
    }
}

float FormantSynth::source(float t, const VoiceConfig& voice, float target_f0, bool voiced,
                           float noise_amount, float amplitude) {
    const float sample_rate = static_cast<float>(opt_.sample_rate);
    float flow = 0.0f;

    if (voiced && target_f0 > 0.0f) {
        // Fractional phase advance keeps the pitch smooth while it glides.
        const float f0 = std::max(30.0f, target_f0);
        period_ = 1.0f / f0;
        phase_ += f0 / sample_rate * jitter_mul_;
        if (phase_ >= 1.0f) {
            phase_ -= 1.0f;
            ++periods_;
            // New cycle: new jitter and shimmer multipliers. Jitter is
            // symmetric in the log domain so the mean period is unchanged.
            if (voice.jitter > 0.0f) {
                jitter_mul_ = std::exp(voice.jitter * rng_.next_gauss() * 0.5f);
                jitter_mul_ = std::min(1.35f, std::max(0.75f, jitter_mul_));
            }
            if (voice.shimmer > 0.0f) {
                shimmer_mul_ = 1.0f + voice.shimmer * rng_.next_gauss() * 0.5f;
                shimmer_mul_ = std::min(2.0f, std::max(0.2f, shimmer_mul_));
            }
        }

        // Asymmetric glottal flow pulse: quick rise, slower fall, closed
        // phase with no flow. Open quotient 0.6.
        const float oq = 0.6f;
        const float p = phase_ / oq;
        if (p < 1.0f) {
            if (p < 0.35f) {
                flow = 0.5f * (1.0f - std::cos(kPi * p / 0.35f));
            } else {
                flow = 0.5f * (1.0f + std::cos(kPi * (p - 0.35f) / 0.65f));
            }
            flow *= shimmer_mul_;
        }
    }

    // Aspiration: breathiness plus the phoneme's own noise amount. Both are
    // injected at the glottis, so they pass through the tract as well.
    const float breath = std::min(1.0f, std::max(0.0f, voice.breathiness + noise_amount));
    if (breath > 0.0f) {
        flow += rng_.next_gauss() * breath * 0.35f;
    }
    if (opt_.noise_floor > 0.0f) {
        flow += rng_.next_gauss() * opt_.noise_floor * 0.2f;
    }

    // Subglottal pressure sets the driving amplitude.
    flow *= amplitude * voice.subglottal_pressure;

    // Harmonic richness tilts the source spectrum: a one pole split keeps the
    // low band and scales the high band.
    const float r = std::exp(-kTwoPi * 1200.0f / sample_rate);
    tilt_lp_ = (1.0f - r) * flow + r * tilt_lp_;
    const float hp = flow - tilt_lp_;
    flow = tilt_lp_ + hp * std::max(0.2f, voice.harmonic_richness);

    (void)t;
    return flow;
}

float FormantSynth::tract(float x, const VoiceConfig& voice) {
    // Cascade of four formants.
    float y = x;
    for (Resonator& res : r_) y = res.process(y);

    // Nasal branch: mix in a low pole and a shelf that mimics the
    // antiresonance, scaled by the coupling.
    if (opt_.nasal_branch && voice.nasality > 0.0f) {
        const float nasal = nasal_pole_.process(x);
        const float shaped = nasal - nasal_zero_.process(nasal);
        y += shaped * voice.nasality * 1.4f;
    }
    return y;
}

std::vector<float> FormantSynth::render(const std::vector<Phoneme>& timeline,
                                        const VoiceConfig& voice) {
    return render(timeline, voice, nullptr);
}

std::vector<float> FormantSynth::render(const std::vector<Phoneme>& timeline,
                                        const VoiceConfig& voice, SynthStats* stats) {
    reset();
    const float sr = static_cast<float>(opt_.sample_rate);
    const float pitch_ratio = voice.fundamental_hz() / 165.0f;
    const float rate = std::min(4.0f, std::max(0.25f, voice.speaking_rate));

    size_t total = 0;
    for (const Phoneme& p : timeline) {
        total += static_cast<size_t>(std::max(0.0, p.duration_s / rate) * sr);
    }
    std::vector<float> out;
    out.reserve(total + static_cast<size_t>(sr * 0.05f));

    double global_t = 0.0;
    size_t phoneme_count = 0;

    for (const Phoneme& ph : timeline) {
        const double dur = std::max(0.0, ph.duration_s / rate);
        const size_t n = static_cast<size_t>(dur * sr);
        if (n == 0) continue;
        ++phoneme_count;

        // Envelope: short attack and release so phoneme edges do not click.
        const size_t attack = std::min(n / 2, static_cast<size_t>(opt_.attack_ms * 0.001f * sr));
        const size_t release = std::min(n / 2, static_cast<size_t>(opt_.release_ms * 0.001f * sr));

        const float length_scale = 16.5f / std::max(8.0f, voice.vocal_tract_length_cm);
        const float f1_target = ph.f1 * voice.formant_scaling[0] * length_scale;
        const float f2_target = ph.f2 * voice.formant_scaling[1] * length_scale;
        const float f3_target = ph.f3 * voice.formant_scaling[2] * length_scale;
        const float f4_target = 3500.0f * length_scale;

        // Glide time constant: precise articulation moves faster.
        const float precision = std::min(1.0f, std::max(0.0f, voice.articulation_precision));
        const float tau = 0.032f - 0.020f * precision;
        const float glide = 1.0f - std::exp(-static_cast<float>(kControlBlock) / (tau * sr));

        for (size_t i = 0; i < n; ++i) {
            const float local = static_cast<float>(i) / static_cast<float>(n);

            if (i % kControlBlock == 0) {
                f_[0] += (f1_target - f_[0]) * glide;
                f_[1] += (f2_target - f_[1]) * glide;
                f_[2] += (f3_target - f_[2]) * glide;
                f_[3] += (f4_target - f_[3]) * glide;
                update_filters(voice, sr);
            }

            float f0 = (ph.f0_start + (ph.f0_end - ph.f0_start) * local) * pitch_ratio;
            if (voice.vibrato_rate > 0.0f && voice.vibrato_depth > 0.0f) {
                const float lfo = std::sin(kTwoPi * voice.vibrato_rate *
                                           static_cast<float>(global_t));
                f0 *= std::pow(2.0f, voice.vibrato_depth * lfo / 12.0f);
            }

            const float amp = ph.amplitude;
            float s = source(static_cast<float>(global_t), voice, f0, ph.voiced, ph.noise, amp);
            s = tract(s, voice);

            // Lip radiation: a first difference, then a gentle de-emphasis so
            // the high end does not dominate.
            const float radiated = s - lip_prev_;
            lip_prev_ = s;
            float y = radiated * 0.35f + s * 0.65f;

            // Phoneme envelope.
            float env = 1.0f;
            if (attack > 0 && i < attack) env = static_cast<float>(i) / static_cast<float>(attack);
            if (release > 0 && i + release >= n) {
                env = std::min(env, static_cast<float>(n - 1 - i) / static_cast<float>(release));
            }
            y *= env * opt_.gain;

            out.push_back(y);
            global_t += 1.0 / static_cast<double>(opt_.sample_rate);
        }
    }

    // Statistics and normalisation.
    float peak = 0.0f;
    double sum_sq = 0.0;
    for (float s : out) {
        peak = std::max(peak, std::fabs(s));
        sum_sq += static_cast<double>(s) * static_cast<double>(s);
    }
    if (opt_.normalize && peak > 1e-9f) {
        const float g = opt_.peak_target / peak;
        for (float& s : out) s *= g;
        sum_sq *= static_cast<double>(g) * static_cast<double>(g);
        peak *= g;
    }
    for (float& s : out) s = std::min(1.0f, std::max(-1.0f, s));

    if (stats) {
        stats->samples = out.size();
        stats->seconds = static_cast<double>(out.size()) / static_cast<double>(opt_.sample_rate);
        stats->peak = peak;
        stats->rms = out.empty() ? 0.0f
                                 : static_cast<float>(std::sqrt(sum_sq / static_cast<double>(out.size())));
        stats->periods = periods_;
        stats->phonemes = phoneme_count;
    }
    return out;
}

std::vector<float> FormantSynth::render_phoneme(const Phoneme& phoneme, const VoiceConfig& voice) {
    std::vector<Phoneme> one{phoneme};
    return render(one, voice);
}

}  // namespace tts

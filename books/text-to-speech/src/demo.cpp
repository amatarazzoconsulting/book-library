// MIT License - (c) Anthony Matarazzo 2026
//
// demo.cpp - exercises all four algorithms and writes playable WAV files.
//
//   ./bin/demo            (run from the book directory)
//
// Output files are written next to this README: demo-*.wav.
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "formant_synth.hpp"
#include "prosody.hpp"
#include "voice_config.hpp"
#include "wav_writer.hpp"

using namespace tts;

namespace {

void banner(const char* title) {
    std::printf("\n=== %s ===\n", title);
}

void render_to_file(const std::string& path, const std::string& text,
                    const VoiceConfig& voice, float base_f0, int sample_rate) {
    ProsodyOptions po;
    po.base_f0 = base_f0;
    po.rate = voice.speaking_rate;
    po.articulation_precision = voice.articulation_precision;
    Prosody prosody(po);

    const std::vector<Phoneme> timeline = prosody.convert(text);

    SynthOptions so;
    so.sample_rate = sample_rate;
    FormantSynth synth(so);
    SynthStats stats;
    const auto t0 = std::chrono::steady_clock::now();
    const std::vector<float> pcm = synth.render(timeline, voice, &stats);
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    const bool ok = WavWriter::write(path, pcm, sample_rate);
    std::printf("  %-22s %5zu phonemes %6.2f s audio %7.1f ms  rtf %6.1fx  %s\n",
                voice.name.c_str(), stats.phonemes, stats.seconds, ms,
                stats.seconds / (ms / 1000.0), ok ? "written" : "WRITE FAILED");
    std::printf("  %-22s -> %s\n", "", path.c_str());
}

}  // namespace

int main() {
    std::printf("Text to Speech with Adaptive Voices - demo\n");
    std::printf("Algorithms: voice_config, prosody, formant_synth, wav_writer\n");

    // ---------------------------------------------------------------- 1 ----
    banner("1. voice_config: presets and a morph");
    for (const char* id : {"neutral", "bright", "deep", "child", "whisper"}) {
        const VoiceConfig v = VoiceConfig::preset(id);
        std::printf("  %-8s %s\n", id, v.describe().c_str());
    }
    const VoiceConfig a = VoiceConfig::preset("child");
    const VoiceConfig b = VoiceConfig::preset("elderly");
    const VoiceConfig mid = a.morph(b, 0.5f);
    std::printf("  morph child->elderly at t=0.5: f0 %.1f Hz, tract %.2f cm\n",
                mid.fundamental_hz(), mid.vocal_tract_length_cm);

    // ---------------------------------------------------------------- 2 ----
    banner("2. prosody: text to phonemes with pitch, duration, stress");
    ProsodyOptions po;
    po.base_f0 = 165.0f;
    Prosody prosody(po);
    const std::string sentence = "The quick brown fox jumps over the lazy dog.";
    const std::vector<Phoneme> timeline = prosody.convert(sentence);
    std::printf("  text: %s\n", sentence.c_str());
    std::printf("  %zu phonemes, %.2f s planned\n", timeline.size(),
                Prosody::total_duration(timeline));
    std::printf("  %-6s %-10s %7s %7s %8s %8s %s\n", "sym", "word", "start", "dur",
                "f0 in", "f0 out", "stress");
    for (size_t i = 0; i < timeline.size() && i < 14; ++i) {
        const Phoneme& p = timeline[i];
        std::printf("  %-6s %-10s %7.3f %7.3f %8.1f %8.1f %d\n", p.symbol.c_str(),
                    p.word.c_str(), p.start_s, p.duration_s, p.f0_start, p.f0_end, p.stress);
    }
    if (timeline.size() > 14) std::printf("  ... %zu more\n", timeline.size() - 14);

    // ---------------------------------------------------------------- 3 ----
    banner("3. formant_synth: source filter synthesis");
    for (const char* id : {"neutral", "deep", "breathy"}) {
        const VoiceConfig v = VoiceConfig::preset(id);
        SynthOptions so;
        FormantSynth synth(so);
        SynthStats stats;
        const std::vector<float> pcm = synth.render(timeline, v, &stats);
        std::printf("  %-9s %7zu samples  peak %.2f  rms %.3f  %zu glottal periods\n",
                    id, stats.samples, stats.peak, stats.rms, stats.periods);
        (void)pcm;
    }

    // ---------------------------------------------------------------- 4 ----
    banner("4. wav_writer: playable files in the book directory");
    render_to_file("demo-neutral.wav", "Text to speech with adaptive voices.", VoiceConfig::preset("neutral"), 165.0f, 48000);
    render_to_file("demo-deep.wav", "Text to speech with adaptive voices.", VoiceConfig::preset("deep"), 110.0f, 48000);
    render_to_file("demo-child.wav", "Text to speech with adaptive voices.", VoiceConfig::preset("child"), 260.0f, 48000);
    render_to_file("demo-whisper.wav", "Text to speech with adaptive voices.", VoiceConfig::preset("whisper"), 165.0f, 48000);
    render_to_file("demo-question.wav", "Is this a question?", VoiceConfig::preset("bright"), 200.0f, 48000);

    // ---------------------------------------------------------------- 5 ----
    banner("5. round trip check on the last file");
    WavInfo info;
    if (WavWriter::read("demo-question.wav", &info)) {
        std::printf("  %d Hz, %d channel, %d bit, %u frames, %.3f s, peak %.3f\n",
                    info.sample_rate, info.channels, info.bits_per_sample, info.frames,
                    info.duration_s, info.peak);
    } else {
        std::printf("  read back FAILED\n");
        return 1;
    }

    std::printf("\ndone. play the files with: aplay demo-neutral.wav\n");
    return 0;
}

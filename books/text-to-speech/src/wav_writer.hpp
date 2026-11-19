// MIT License - (c) Anthony Matarazzo 2026
//
// wav_writer.hpp - RIFF/WAVE output for float PCM.
//
// The book's AudioBuffer::write_wav and ProductionArticulatoryEngine::saveToWAV
// both end here: 16 bit mono PCM, little endian, canonical 44 byte header. The
// reader is included so tests and teaching programs can prove the file they
// just wrote is well formed.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tts {

struct WavInfo {
    int sample_rate = 0;
    int channels = 0;
    int bits_per_sample = 0;
    uint32_t frames = 0;      // samples per channel
    double duration_s = 0.0;
    float peak = 0.0f;
    float rms = 0.0f;
};

class WavWriter {
public:
    // 16 bit PCM, mono. Samples outside -1..1 are clamped. A true peak above
    // 1.0 with clip_report enabled is reported through WavInfo::peak, which
    // then exceeds 1.0 so callers can see the clipping happened.
    static bool write(const std::string& path, const std::vector<float>& samples,
                      int sample_rate);

    // Read back a file written by write(). Returns false on any malformed
    // header. When out_samples is not null it receives the decoded floats.
    static bool read(const std::string& path, WavInfo* info,
                     std::vector<float>* out_samples = nullptr);

    // Peak and RMS of a float buffer, in linear units.
    static void measure(const std::vector<float>& samples, float* peak, float* rms);
};

}  // namespace tts

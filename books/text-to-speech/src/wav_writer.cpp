// MIT License - (c) Anthony Matarazzo 2026
#include "wav_writer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

namespace tts {

namespace {

void put_u16(std::vector<unsigned char>& b, uint16_t v) {
    b.push_back(static_cast<unsigned char>(v & 0xFFu));
    b.push_back(static_cast<unsigned char>((v >> 8) & 0xFFu));
}

void put_u32(std::vector<unsigned char>& b, uint32_t v) {
    b.push_back(static_cast<unsigned char>(v & 0xFFu));
    b.push_back(static_cast<unsigned char>((v >> 8) & 0xFFu));
    b.push_back(static_cast<unsigned char>((v >> 16) & 0xFFu));
    b.push_back(static_cast<unsigned char>((v >> 24) & 0xFFu));
}

void put_tag(std::vector<unsigned char>& b, const char* tag) {
    for (int i = 0; i < 4; ++i) b.push_back(static_cast<unsigned char>(tag[i]));
}

uint16_t get_u16(const unsigned char* p) {
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

uint32_t get_u32(const unsigned char* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

float clamp1(float v) { return std::min(1.0f, std::max(-1.0f, v)); }

}  // namespace

void WavWriter::measure(const std::vector<float>& samples, float* peak, float* rms) {
    float p = 0.0f;
    double sum_sq = 0.0;
    for (float s : samples) {
        p = std::max(p, std::fabs(s));
        sum_sq += static_cast<double>(s) * static_cast<double>(s);
    }
    if (peak) *peak = p;
    if (rms) {
        *rms = samples.empty()
                   ? 0.0f
                   : static_cast<float>(std::sqrt(sum_sq / static_cast<double>(samples.size())));
    }
}

bool WavWriter::write(const std::string& path, const std::vector<float>& samples,
                      int sample_rate) {
    if (sample_rate <= 0) return false;
    if (samples.size() > 0x7FFFFFFFu / 2u) return false;

    const uint32_t data_bytes = static_cast<uint32_t>(samples.size() * 2u);
    const uint32_t riff_size = 36u + data_bytes;

    std::vector<unsigned char> buf;
    buf.reserve(44u + data_bytes);

    put_tag(buf, "RIFF");
    put_u32(buf, riff_size);
    put_tag(buf, "WAVE");

    put_tag(buf, "fmt ");
    put_u32(buf, 16u);                              // PCM chunk size
    put_u16(buf, 1u);                               // format: PCM
    put_u16(buf, 1u);                               // channels
    put_u32(buf, static_cast<uint32_t>(sample_rate));
    put_u32(buf, static_cast<uint32_t>(sample_rate) * 2u);  // byte rate
    put_u16(buf, 2u);                               // block align
    put_u16(buf, 16u);                              // bits per sample

    put_tag(buf, "data");
    put_u32(buf, data_bytes);

    for (float s : samples) {
        // Round to nearest, symmetric about zero.
        const float v = clamp1(s) * 32767.0f;
        const int32_t q = static_cast<int32_t>(v >= 0.0f ? v + 0.5f : v - 0.5f);
        put_u16(buf, static_cast<uint16_t>(static_cast<int16_t>(q)));
    }

    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    return static_cast<bool>(f);
}

bool WavWriter::read(const std::string& path, WavInfo* info,
                     std::vector<float>* out_samples) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<unsigned char> buf((std::istreambuf_iterator<char>(f)),
                                   std::istreambuf_iterator<char>());
    if (buf.size() < 44u) return false;
    if (std::memcmp(buf.data(), "RIFF", 4) != 0) return false;
    if (std::memcmp(buf.data() + 8, "WAVE", 4) != 0) return false;
    if (std::memcmp(buf.data() + 12, "fmt ", 4) != 0) return false;

    const uint16_t format = get_u16(&buf[20]);
    const uint16_t channels = get_u16(&buf[22]);
    const uint32_t sample_rate = get_u32(&buf[24]);
    const uint16_t bits = get_u16(&buf[34]);
    if (format != 1u || channels != 1u || bits != 16u) return false;
    if (std::memcmp(buf.data() + 36, "data", 4) != 0) return false;

    const uint32_t data_bytes = get_u32(&buf[40]);
    if (data_bytes > buf.size() - 44u) return false;
    if (data_bytes % 2u != 0u) return false;

    std::vector<float> samples;
    samples.reserve(data_bytes / 2u);
    for (uint32_t i = 0; i < data_bytes; i += 2u) {
        const int16_t v = static_cast<int16_t>(get_u16(&buf[44u + i]));
        samples.push_back(static_cast<float>(v) / 32767.0f);
    }

    if (info) {
        info->sample_rate = static_cast<int>(sample_rate);
        info->channels = static_cast<int>(channels);
        info->bits_per_sample = static_cast<int>(bits);
        info->frames = data_bytes / 2u;
        info->duration_s = sample_rate ? static_cast<double>(info->frames) / sample_rate : 0.0;
        measure(samples, &info->peak, &info->rms);
    }
    if (out_samples) *out_samples = std::move(samples);
    return true;
}

}  // namespace tts

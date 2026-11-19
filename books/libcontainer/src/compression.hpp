// MIT License - (c) Anthony Matarazzo 2026
// Run-length and LZ77 style compression used by the container storage layer.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace libcontainer {

enum class Codec { kNone = 0, kRle = 1, kLz77 = 2, kAuto = 3 };

struct CompressionStats {
    std::size_t input_bytes = 0;
    std::size_t output_bytes = 0;
    double ratio() const {
        if (input_bytes == 0) return 0.0;
        return static_cast<double>(output_bytes) / static_cast<double>(input_bytes);
    }
};

std::vector<std::uint8_t> rle_encode(const std::vector<std::uint8_t>& in);
std::vector<std::uint8_t> rle_decode(const std::vector<std::uint8_t>& in);

std::vector<std::uint8_t> lz77_encode(const std::vector<std::uint8_t>& in,
                                      std::size_t window = 4096,
                                      std::size_t max_match = 18);
std::vector<std::uint8_t> lz77_decode(const std::vector<std::uint8_t>& in);

std::vector<std::uint8_t> compress(const std::vector<std::uint8_t>& in, Codec codec);
std::vector<std::uint8_t> decompress(const std::vector<std::uint8_t>& in, Codec codec);

// Picks the codec that produced the smallest payload and reports the choice.
CompressionStats choose_codec(const std::vector<std::uint8_t>& in, Codec* chosen);

}  // namespace libcontainer

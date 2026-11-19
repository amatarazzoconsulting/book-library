// MIT License - (c) Anthony Matarazzo 2026
//
// compress.hpp - the compression toolkit from LOWL Class Examples: RLE, LZ77,
// XOR delta and an adaptive Huffman coder, plus the header framing that makes a
// stream self describing so any decoder can be picked from the payload.
#ifndef LOWL_COMPRESS_HPP
#define LOWL_COMPRESS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace lowl {

// CompressionAlgorithm from crypto_types.lowl
enum class Algorithm : std::uint16_t {
    NONE = 0,
    LZ77 = 1,
    HUFFMAN = 2,
    RLE = 3,
    XOR = 4,
    DELTA = 5,
    AUTO = 999
};

// CompressionLevel from crypto_types.lowl
enum class Level : std::uint8_t {
    FASTEST = 1,
    DEFAULT = 5,
    MAXIMUM = 9,
    ULTRA = 10
};

const char* algorithm_name(Algorithm a);
const char* level_name(Level l);

struct CompressStats {
    std::size_t input_bytes = 0;
    std::size_t output_bytes = 0;
    std::size_t literal_bytes = 0;   // bytes emitted verbatim
    std::size_t match_count = 0;     // back references / runs / symbols
    double ratio() const {
        return output_bytes == 0 ? 0.0 : static_cast<double>(input_bytes) / static_cast<double>(output_bytes);
    }
    double saved_percent() const {
        if (input_bytes == 0) return 0.0;
        return 100.0 * (1.0 - static_cast<double>(output_bytes) / static_cast<double>(input_bytes));
    }
};

// --- RLE: runs of 3 or more bytes become one marker plus value plus count.
std::vector<std::uint8_t> rle_compress(const std::vector<std::uint8_t>& in, CompressStats* st = nullptr);
std::vector<std::uint8_t> rle_decompress(const std::vector<std::uint8_t>& in);

// --- LZ77: greedy longest match inside a sliding window.
struct Lz77Config {
    std::size_t window = 4096;
    std::size_t min_match = 3;
    std::size_t max_match = 258;
    // How many earlier positions sharing a 3 byte hash are tried before the
    // search gives up. This is the only knob that trades time for ratio.
    std::size_t max_chain = 64;
};

// Maps a book CompressionLevel onto window and chain depth.
Lz77Config lz77_config_for(Level level);
std::vector<std::uint8_t> lz77_compress(const std::vector<std::uint8_t>& in,
                                        const Lz77Config& cfg = Lz77Config(),
                                        CompressStats* st = nullptr);
std::vector<std::uint8_t> lz77_decompress(const std::vector<std::uint8_t>& in);

// --- XOR delta: out[i] = in[i] ^ in[i-1]
std::vector<std::uint8_t> xor_compress(const std::vector<std::uint8_t>& in, CompressStats* st = nullptr);
std::vector<std::uint8_t> xor_decompress(const std::vector<std::uint8_t>& in);

// --- Block adaptive Huffman. Frequencies are rebuilt every block_size bytes
// and both sides derive the same canonical code from them.
struct HuffmanConfig {
    std::size_t block_size = 4096;
};
std::vector<std::uint8_t> huffman_compress(const std::vector<std::uint8_t>& in,
                                           CompressStats* st = nullptr,
                                           const HuffmanConfig& cfg = HuffmanConfig());
std::vector<std::uint8_t> huffman_decompress(const std::vector<std::uint8_t>& in);

// --- Framed container: "LC" + version + algorithm + original length + payload.
std::vector<std::uint8_t> compress(const std::vector<std::uint8_t>& in, Algorithm algo,
                                   CompressStats* st = nullptr);
std::vector<std::uint8_t> decompress(const std::vector<std::uint8_t>& in);
// Picks the smallest of RLE, LZ77, XOR and Huffman for this input.
std::vector<std::uint8_t> compress_auto(const std::vector<std::uint8_t>& in,
                                        Algorithm* chosen = nullptr,
                                        CompressStats* st = nullptr);

// Rolling 64 bit integrity hash used by DirectoryManager (FNV style, rotl mix).
std::uint64_t directory_hash(const std::uint8_t* data, std::size_t len);
std::uint64_t directory_hash(const std::vector<std::uint8_t>& data);
std::string hex_u64(std::uint64_t v);

} // namespace lowl

#endif // LOWL_COMPRESS_HPP

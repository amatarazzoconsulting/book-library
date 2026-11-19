// MIT License - (c) Anthony Matarazzo 2026
#include "cipher.hpp"

#include <algorithm>
#include <array>

#include "genetic_cipher.hpp"
#include "sha256.hpp"

namespace srsp {
namespace cipher {
namespace {

// The header's class is a template over the operation pipeline, and the bare
// instantiation GeneticCipher<> runs zero operations: it returns the plaintext
// with padding and nothing else. The pipeline has to be named explicitly, so it
// is named once here and every frame in the system uses the same one.
using pipeline = genetic_cipher::GeneticCipher<
    genetic_cipher::Scatter, genetic_cipher::Shuffle, genetic_cipher::Diffuse,
    genetic_cipher::Mix, genetic_cipher::Permute, genetic_cipher::Rotate,
    genetic_cipher::Flip, genetic_cipher::Swap, genetic_cipher::BitwiseNot,
    genetic_cipher::BitwiseRotL, genetic_cipher::ByteReverse, genetic_cipher::WordMix,
    genetic_cipher::TransformXorShift>;

constexpr std::size_t k_header = 10;  // magic + plain length + chunk size log2
constexpr std::size_t k_min_chunk = 256;
constexpr std::size_t k_tag = 32;
const char k_magic[4] = {'S', 'R', 'C', '1'};

genetic_cipher::EncryptionSettings make_settings(const config& cfg) {
    genetic_cipher::EncryptionSettings s;
    s.parallel_threads = cfg.parallel_threads == 0 ? 1 : cfg.parallel_threads;
    s.key_derivation_rounds = cfg.key_derivation_rounds;
    s.container_chunk_size = cfg.chunk_size;
    s.minimum_encrypted_size = cfg.chunk_size;
    s.bit_mode = genetic_cipher::BitMode::BYTE;
    return s;
}

bytes mac_key(const bytes& key) {
    static const std::string label = "srsp-genetic-cipher-mac";
    const auto tag = hash::hmac(key, std::vector<std::uint8_t>(label.begin(), label.end()));
    return bytes(tag.begin(), tag.end());
}

bytes compute_tag(const bytes& key, const bytes& header, const bytes& body, const bytes& aad) {
    bytes material;
    material.reserve(header.size() + body.size() + aad.size());
    material.insert(material.end(), header.begin(), header.end());
    material.insert(material.end(), body.begin(), body.end());
    material.insert(material.end(), aad.begin(), aad.end());
    const auto tag = hash::hmac(mac_key(key), material);
    return bytes(tag.begin(), tag.end());
}

// Small frames should not pay a 4 KiB padding bill, so the chunk size travels in
// the header as a power of two. Large frames still use the configured ceiling.
std::size_t chunk_for(std::size_t plain_len, const config& cfg) {
    std::size_t chunk = k_min_chunk;
    while (chunk < plain_len && chunk < cfg.chunk_size) chunk <<= 1;
    return chunk;
}

void put_u32(bytes& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
}

std::uint32_t get_u32(const bytes& in, std::size_t at) {
    return static_cast<std::uint32_t>(in[at]) |
           (static_cast<std::uint32_t>(in[at + 1]) << 8) |
           (static_cast<std::uint32_t>(in[at + 2]) << 16) |
           (static_cast<std::uint32_t>(in[at + 3]) << 24);
}

}  // namespace

std::size_t overhead() { return k_header + k_tag; }

const char* backend_name() { return "genetic_cipher.hpp pipeline of 13 operations"; }

std::size_t operation_count() { return pipeline::operation_count; }

bytes seal(const bytes& key, const bytes& plaintext, const bytes& aad, const config& cfg) {
    const std::size_t chunk = chunk_for(plaintext.size(), cfg);
    bytes header(k_magic, k_magic + 4);
    put_u32(header, static_cast<std::uint32_t>(plaintext.size()));
    header.push_back(static_cast<std::uint8_t>(chunk & 0xFF));
    header.push_back(static_cast<std::uint8_t>((chunk >> 8) & 0xFF));

    config frame_cfg = cfg;
    frame_cfg.chunk_size = chunk;

    bytes body;
    if (!plaintext.empty()) {
        pipeline engine(make_settings(frame_cfg));
        engine.set_password({key.data(), key.size()});
        engine.data().append_bytes(plaintext.data(), plaintext.size());
        body = engine.encrypt().to_bytes();
    }
    bytes out = header;
    out.insert(out.end(), body.begin(), body.end());
    if (cfg.authenticate) {
        const bytes tag = compute_tag(key, header, body, aad);
        out.insert(out.end(), tag.begin(), tag.end());
    }
    return out;
}

bool open(const bytes& key, const bytes& sealed, const bytes& aad, bytes& out, const config& cfg) {
    const std::size_t tag_len = cfg.authenticate ? k_tag : 0;
    if (sealed.size() < k_header + tag_len) return false;
    if (!std::equal(k_magic, k_magic + 4, sealed.begin())) return false;

    const std::size_t body_len = sealed.size() - k_header - tag_len;
    const bytes header(sealed.begin(), sealed.begin() + k_header);
    const bytes body(sealed.begin() + k_header, sealed.begin() + k_header + body_len);

    if (cfg.authenticate) {
        const bytes expect = compute_tag(key, header, body, aad);
        const bytes given(sealed.end() - k_tag, sealed.end());
        if (!hash::constant_time_equal(expect, given)) return false;
    }

    const std::uint32_t plain_len = get_u32(sealed, 4);
    if (body.empty()) {
        if (plain_len != 0) return false;
        out.clear();
        return true;
    }
    config frame_cfg = cfg;
    const std::size_t chunk = static_cast<std::size_t>(sealed[8]) |
                              (static_cast<std::size_t>(sealed[9]) << 8);
    if (chunk < k_min_chunk || chunk > cfg.chunk_size) return false;
    frame_cfg.chunk_size = chunk;

    pipeline engine(make_settings(frame_cfg));
    engine.set_password({key.data(), key.size()});
    engine.data().append_bytes(body.data(), body.size());
    const bytes plain = engine.decrypt().to_bytes();
    if (plain_len > plain.size()) return false;
    out.assign(plain.begin(), plain.begin() + plain_len);
    return true;
}

}  // namespace cipher
}  // namespace srsp

// MIT License - (c) Anthony Matarazzo 2026
//
// genetic_cipher.cpp - CryptoPrng, bit packing and the reversible operations.
#include "genetic_cipher.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>

namespace lowl {

namespace {

inline std::uint32_t rotl32(std::uint32_t v, unsigned n) {
    n &= 31u;
    return n == 0 ? v : static_cast<std::uint32_t>((v << n) | (v >> (32 - n)));
}
inline std::uint8_t rotl8(std::uint8_t v, unsigned n) {
    n &= 7u;
    return n == 0 ? v : static_cast<std::uint8_t>((v << n) | (v >> (8 - n)));
}
inline std::uint8_t rotr8(std::uint8_t v, unsigned n) {
    n &= 7u;
    return n == 0 ? v : static_cast<std::uint8_t>((v >> n) | (v << (8 - n)));
}

inline void quarter_round(std::uint32_t& a, std::uint32_t& b, std::uint32_t& c,
                          std::uint32_t& d) {
    a += b; d ^= a; d = rotl32(d, 16);
    c += d; b ^= c; b = rotl32(b, 12);
    a += b; d ^= a; d = rotl32(d, 8);
    c += d; b ^= c; b = rotl32(b, 7);
}

// Splits an element stream into bits and re-groups them at a new width.
std::vector<unsigned __int128> pack_bits(const std::uint8_t* data, std::size_t bit_count,
                                         std::size_t width) {
    std::vector<unsigned __int128> out;
    if (width == 0 || bit_count == 0) return out;
    const std::size_t count = (bit_count + width - 1) / width;
    out.assign(count, 0);
    if (width == 8) {
        for (std::size_t i = 0; i < count; ++i) out[i] = data[i];
        return out;
    }
    for (std::size_t bit = 0; bit < bit_count; ++bit) {
        const unsigned b = (data[bit >> 3] >> (bit & 7u)) & 1u;
        if (b) out[bit / width] |= (static_cast<unsigned __int128>(1) << (bit % width));
    }
    return out;
}

std::vector<std::uint8_t> unpack_bits(const std::vector<unsigned __int128>& elems,
                                      std::size_t width, std::size_t bit_count) {
    std::vector<std::uint8_t> out((bit_count + 7) / 8, 0);
    if (width == 0) return out;
    for (std::size_t bit = 0; bit < bit_count; ++bit) {
        const unsigned __int128 e = elems[bit / width];
        if ((e >> (bit % width)) & 1u) out[bit >> 3] |= static_cast<std::uint8_t>(1u << (bit & 7u));
    }
    return out;
}

CryptoPrng op_prng(const CipherState& s, unsigned index, const char* tag) {
    std::string user(reinterpret_cast<const char*>(s.master_key.data()), s.master_key.size());
    std::string elec(tag);
    elec.push_back(static_cast<char>(index & 0xFF));
    elec.push_back(static_cast<char>((index >> 8) & 0xFF));
    CryptoPrng p;
    p.seed(user, elec);
    return p;
}

} // namespace

const char* bit_mode_name(BitMode mode) {
    switch (mode) {
        case BitMode::BIT_1: return "BIT_1";
        case BitMode::BIT_2: return "BIT_2";
        case BitMode::BIT_3: return "BIT_3";
        case BitMode::NIBBLE: return "NIBBLE";
        case BitMode::BIT_5: return "BIT_5";
        case BitMode::BIT_6: return "BIT_6";
        case BitMode::BYTE: return "BYTE";
        case BitMode::WORD16: return "WORD16";
        case BitMode::WORD32: return "WORD32";
        case BitMode::WORD64: return "WORD64";
        case BitMode::BLOCK: return "BLOCK";
    }
    return "UNKNOWN";
}

std::size_t bit_mode_width(BitMode mode) { return static_cast<std::size_t>(mode); }

bool bit_mode_valid(std::uint8_t raw) {
    switch (raw) {
        case 1: case 2: case 3: case 4: case 5: case 6:
        case 8: case 16: case 32: case 64: case 128: return true;
        default: return false;
    }
}

unsigned __int128 width_mask(std::size_t width) {
    if (width == 0) return 0;
    if (width >= 128) return ~static_cast<unsigned __int128>(0);
    return (static_cast<unsigned __int128>(1) << width) - 1;
}

unsigned __int128 rotl128(unsigned __int128 v, unsigned n, std::size_t width) {
    if (width == 0) return 0;
    if (width >= 128) {
        n &= 127u;
        return n == 0 ? v : static_cast<unsigned __int128>((v << n) | (v >> (128 - n)));
    }
    n %= static_cast<unsigned>(width);
    if (n == 0) return v & width_mask(width);
    const unsigned __int128 m = width_mask(width);
    v &= m;
    return ((v << n) | (v >> (width - n))) & m;
}

unsigned __int128 rotr128(unsigned __int128 v, unsigned n, std::size_t width) {
    if (width == 0) return 0;
    if (width >= 128) {
        n &= 127u;
        return n == 0 ? v : static_cast<unsigned __int128>((v >> n) | (v << (128 - n)));
    }
    n %= static_cast<unsigned>(width);
    if (n == 0) return v & width_mask(width);
    const unsigned __int128 m = width_mask(width);
    v &= m;
    return ((v >> n) | (v << (width - n))) & m;
}

std::vector<unsigned __int128> bytes_to_elements(const std::uint8_t* data, std::size_t len,
                                                 std::size_t width, std::size_t* bit_count) {
    if (bit_count) *bit_count = len * 8;
    return pack_bits(data, len * 8, width);
}

std::vector<std::uint8_t> elements_to_bytes(const std::vector<unsigned __int128>& elems,
                                            std::size_t width, std::size_t bit_count) {
    return unpack_bits(elems, width, bit_count);
}

void repack(CipherState& s, std::size_t new_width) {
    if (new_width == 0 || new_width == s.width || s.elements.empty()) {
        s.width = new_width == 0 ? s.width : new_width;
        s.mask = width_mask(s.width);
        return;
    }
    const std::size_t total_bits = s.elements.size() * s.width;
    const std::size_t keep_bits = s.bit_count;
    std::vector<std::uint8_t> buf = unpack_bits(s.elements, s.width, total_bits);
    s.elements = pack_bits(buf.data(), total_bits, new_width);
    s.width = new_width;
    s.mask = width_mask(new_width);
    s.bit_count = keep_bits;
}

// ---------------------------------------------------------------- CryptoPrng

CryptoPrng::CryptoPrng() { init_state(); }

void CryptoPrng::seed(const std::string& user_password,
                      const std::string& electronic_password) {
    std::array<std::uint8_t, 32> km{};
    km.fill(0);

    for (std::size_t i = 0; i < user_password.size(); ++i) {
        const std::uint8_t c = static_cast<std::uint8_t>(user_password[i]);
        km[i % 32] ^= c;
        km[(i + 7) % 32] = static_cast<std::uint8_t>(km[(i + 7) % 32] + c);
        km[(i + 13) % 32] = static_cast<std::uint8_t>(rotl8(km[(i + 13) % 32], 3) ^ c);
    }
    for (std::size_t i = 0; i < electronic_password.size(); ++i) {
        const std::uint8_t c = static_cast<std::uint8_t>(electronic_password[i]);
        km[i % 32] ^= c;
        km[(i + 11) % 32] = static_cast<std::uint8_t>(km[(i + 11) % 32] - c);
        km[(i + 19) % 32] = static_cast<std::uint8_t>(rotr8(km[(i + 19) % 32], 5) ^ c);
    }
    for (int round = 0; round < 10; ++round) {
        for (std::size_t i = 0; i < 32; ++i) {
            km[i] = static_cast<std::uint8_t>(km[i] ^ km[(i + 1) % 32]);
            km[i] = rotl8(km[i], 3);
            km[i] = static_cast<std::uint8_t>(km[i] + km[(i + 15) % 32]);
        }
    }
    key_ = km;
    for (std::size_t i = 0; i < 12; ++i) nonce_[i] = km[i] ^ km[i + 20];

    counter_ = 0;
    blocks_ = 0;
    buffer_pos_ = 16;
    init_state();
}

void CryptoPrng::reset() {
    counter_ = 0;
    blocks_ = 0;
    buffer_pos_ = 16;
    init_state();
}

void CryptoPrng::init_state() {
    state_[0] = 0x61707865ULL;
    state_[1] = 0x3320646eULL;
    state_[2] = 0x79622d32ULL;
    state_[3] = 0x6b206574ULL;
    for (int i = 0; i < 8; ++i) {
        std::uint64_t w = 0;
        for (int b = 0; b < 4; ++b) w |= static_cast<std::uint64_t>(key_[i * 4 + b]) << (b * 8);
        state_[4 + i] = w;
    }
    state_[12] = counter_ & 0xFFFFFFFFULL;
    state_[13] = (counter_ >> 32) & 0xFFFFFFFFULL;
    std::uint64_t n0 = 0, n1 = 0;
    for (int b = 0; b < 4; ++b) n0 |= static_cast<std::uint64_t>(nonce_[b]) << (b * 8);
    for (int b = 0; b < 4; ++b) n1 |= static_cast<std::uint64_t>(nonce_[4 + b]) << (b * 8);
    state_[14] = n0;
    state_[15] = n1;
}

void CryptoPrng::generate_block() {
    std::array<std::uint32_t, 16> x{};
    for (int i = 0; i < 16; ++i) x[i] = static_cast<std::uint32_t>(state_[i]);
    for (int r = 0; r < 10; ++r) {
        quarter_round(x[0], x[4], x[8], x[12]);
        quarter_round(x[1], x[5], x[9], x[13]);
        quarter_round(x[2], x[6], x[10], x[14]);
        quarter_round(x[3], x[7], x[11], x[15]);
        quarter_round(x[0], x[5], x[10], x[15]);
        quarter_round(x[1], x[6], x[11], x[12]);
        quarter_round(x[2], x[7], x[8], x[13]);
        quarter_round(x[3], x[4], x[9], x[14]);
    }
    for (int i = 0; i < 16; ++i) {
        x[i] = static_cast<std::uint32_t>(x[i] + static_cast<std::uint32_t>(state_[i]));
        buffer_[i] = x[i];
    }
    // Advance the 64 bit block counter.
    counter_ = (counter_ + 1) & 0xFFFFFFFFFFFFFFFFULL;
    state_[12] = counter_ & 0xFFFFFFFFULL;
    state_[13] = (counter_ >> 32) & 0xFFFFFFFFULL;
    ++blocks_;
    buffer_pos_ = 0;
}

std::uint64_t CryptoPrng::next_u64() {
    if (buffer_pos_ >= 16) generate_block();
    return buffer_[buffer_pos_++];
}

std::uint64_t CryptoPrng::range(std::uint64_t lo, std::uint64_t hi) {
    if (hi <= lo) return lo;
    const std::uint64_t span = hi - lo + 1;
    if (span == 0) return next_u64();
    // Rejection sampling keeps the distribution uniform.
    const std::uint64_t limit = UINT64_MAX - (UINT64_MAX % span);
    std::uint64_t v = next_u64();
    while (v >= limit) v = next_u64();
    return lo + (v % span);
}

std::vector<std::uint32_t> CryptoPrng::permutation(std::size_t n) {
    std::vector<std::uint32_t> perm(n);
    std::iota(perm.begin(), perm.end(), 0u);
    if (n < 2) return perm;
    for (std::size_t i = n - 1; i > 0; --i) {
        const std::uint64_t j = range(0, static_cast<std::uint64_t>(i));
        std::swap(perm[i], perm[static_cast<std::size_t>(j)]);
    }
    return perm;
}

// ---------------------------------------------------------------- operations

void OpScatter::forward(CipherState& s) {
    const std::size_t n = s.elements.size();
    if (n < 2) return;
    CryptoPrng p = op_prng(s, 1, "scatter");
    const std::vector<std::uint32_t> perm = p.permutation(n);
    std::vector<unsigned __int128> tmp(n);
    for (std::size_t i = 0; i < n; ++i) tmp[perm[i]] = s.elements[i];
    s.elements.swap(tmp);
}

void OpScatter::inverse(CipherState& s) {
    const std::size_t n = s.elements.size();
    if (n < 2) return;
    CryptoPrng p = op_prng(s, 1, "scatter");
    const std::vector<std::uint32_t> perm = p.permutation(n);
    std::vector<unsigned __int128> tmp(n);
    for (std::size_t i = 0; i < n; ++i) tmp[i] = s.elements[perm[i]];
    s.elements.swap(tmp);
}

void OpSubstitute::forward(CipherState& s) {
    CryptoPrng p = op_prng(s, 2, "substitute");
    const std::size_t w = s.width;
    for (std::size_t i = 0; i < s.elements.size(); ++i) {
        const unsigned __int128 k1 = static_cast<unsigned __int128>(p.next_u64()) << 64 |
                                     p.next_u64();
        const unsigned r = w > 1 ? static_cast<unsigned>(p.range(1, w - 1)) : 0u;
        const unsigned __int128 k2 = static_cast<unsigned __int128>(p.next_u64()) << 64 |
                                     p.next_u64();
        unsigned __int128 v = rotl128(s.elements[i] ^ k1, r, w);
        v = (v + k2) & s.mask;
        s.elements[i] = v & s.mask;
    }
}

void OpSubstitute::inverse(CipherState& s) {
    CryptoPrng p = op_prng(s, 2, "substitute");
    const std::size_t w = s.width;
    for (std::size_t i = 0; i < s.elements.size(); ++i) {
        const unsigned __int128 k1 = static_cast<unsigned __int128>(p.next_u64()) << 64 |
                                     p.next_u64();
        const unsigned r = w > 1 ? static_cast<unsigned>(p.range(1, w - 1)) : 0u;
        const unsigned __int128 k2 = static_cast<unsigned __int128>(p.next_u64()) << 64 |
                                     p.next_u64();
        unsigned __int128 v = (s.elements[i] - k2) & s.mask;
        v = rotr128(v, r, w) ^ k1;
        s.elements[i] = v & s.mask;
    }
}

void OpDiffuse::forward(CipherState& s) {
    const std::size_t n = s.elements.size();
    if (n < 2) return;
    const std::size_t w = s.width;
    const unsigned step = w > 1 ? static_cast<unsigned>(w) : 1u;
    for (std::size_t i = 1; i < n; ++i) {
        s.elements[i] ^= rotl128(s.elements[i - 1], static_cast<unsigned>(i % step), w);
        s.elements[i] &= s.mask;
    }
}

void OpDiffuse::inverse(CipherState& s) {
    const std::size_t n = s.elements.size();
    if (n < 2) return;
    const std::size_t w = s.width;
    const unsigned step = w > 1 ? static_cast<unsigned>(w) : 1u;
    for (std::size_t i = n - 1; i > 0; --i) {
        s.elements[i] ^= rotl128(s.elements[i - 1], static_cast<unsigned>(i % step), w);
        s.elements[i] &= s.mask;
    }
}

void OpMix::forward(CipherState& s) {
    CryptoPrng p = op_prng(s, 4, "mix");
    for (std::size_t i = 0; i < s.elements.size(); ++i) {
        const unsigned __int128 k = static_cast<unsigned __int128>(p.next_u64()) << 64 |
                                    p.next_u64();
        s.elements[i] = (s.elements[i] ^ k) & s.mask;
    }
}

void OpBitwiseXor::forward(CipherState& s) {
    for (std::size_t i = 0; i < s.elements.size(); ++i) {
        unsigned __int128 k = 0;
        for (std::size_t b = 0; b < 16; ++b)
            k |= static_cast<unsigned __int128>(s.master_key[(i + b) % 32]) << (b * 8);
        s.elements[i] = (s.elements[i] ^ k) & s.mask;
    }
}

void OpBitwiseNot::forward(CipherState& s) {
    for (std::size_t i = 0; i < s.elements.size(); ++i)
        s.elements[i] = (~s.elements[i]) & s.mask;
}

void OpBitwiseRotL::forward(CipherState& s) {
    CryptoPrng p = op_prng(s, 7, "rotl");
    const std::size_t w = s.width;
    for (std::size_t i = 0; i < s.elements.size(); ++i) {
        const unsigned r = w > 1 ? static_cast<unsigned>(p.range(1, w - 1)) : 0u;
        s.elements[i] = rotl128(s.elements[i], r, w);
    }
}

void OpBitwiseRotL::inverse(CipherState& s) {
    CryptoPrng p = op_prng(s, 7, "rotl");
    const std::size_t w = s.width;
    for (std::size_t i = 0; i < s.elements.size(); ++i) {
        const unsigned r = w > 1 ? static_cast<unsigned>(p.range(1, w - 1)) : 0u;
        s.elements[i] = rotr128(s.elements[i], r, w);
    }
}

void OpFlip::forward(CipherState& s) { std::reverse(s.elements.begin(), s.elements.end()); }

void OpRotate::forward(CipherState& s) {
    const std::size_t n = s.elements.size();
    if (n < 2 || (s.settings && !s.settings->enable_rotation)) return;
    CryptoPrng p = op_prng(s, 9, "rotate");
    const std::size_t k = static_cast<std::size_t>(p.range(0, n - 1));
    std::vector<unsigned __int128> tmp(n);
    for (std::size_t i = 0; i < n; ++i) tmp[(i + k) % n] = s.elements[i];
    s.elements.swap(tmp);
}

void OpRotate::inverse(CipherState& s) {
    const std::size_t n = s.elements.size();
    if (n < 2 || (s.settings && !s.settings->enable_rotation)) return;
    CryptoPrng p = op_prng(s, 9, "rotate");
    const std::size_t k = static_cast<std::size_t>(p.range(0, n - 1));
    if (k == 0) return;
    std::vector<unsigned __int128> tmp(n);
    for (std::size_t i = 0; i < n; ++i) tmp[i] = s.elements[(i + k) % n];
    s.elements.swap(tmp);
}

void OpModeCycle::forward(CipherState& s) {
    const std::size_t target = s.settings ? bit_mode_width(s.settings->cycle_mode) : 4;
    repack(s, target);
}

void OpModeCycle::inverse(CipherState& s) {
    const std::size_t target = s.settings ? bit_mode_width(s.settings->bit_mode) : 8;
    repack(s, target);
}

} // namespace lowl

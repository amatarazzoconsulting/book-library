// MIT License - (c) Anthony Matarazzo 2026
//
// sha256.hpp - minimal, dependency free SHA-256 and HMAC-SHA256.
//
// This is the only support unit shared by the companion algorithms. It exists
// because both the ring handshake and the tamper evident audit log need a hash
// primitive, and the book forbids external crypto libraries. It is header only
// so every algorithm pair plus this header is a complete translation unit.
//
// Reference: FIPS 180-4 (SHA-256), RFC 2104 (HMAC).

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace srsp {
namespace hash {

inline std::uint32_t rotr(std::uint32_t x, std::uint32_t n) {
    return (x >> n) | (x << (32u - n));
}

class sha256 {
public:
    sha256() { reset(); }

    void reset() {
        h_ = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
              0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
        buf_.fill(0);
        buf_len_ = 0;
        total_ = 0;
    }

    void update(const void* data, std::size_t len) {
        const std::uint8_t* p = static_cast<const std::uint8_t*>(data);
        total_ += len;
        while (len > 0) {
            std::size_t take = 64 - buf_len_;
            if (take > len) take = len;
            for (std::size_t i = 0; i < take; ++i) buf_[buf_len_ + i] = p[i];
            buf_len_ += take;
            p += take;
            len -= take;
            if (buf_len_ == 64) {
                compress(buf_.data());
                buf_len_ = 0;
            }
        }
    }

    void update(const std::vector<std::uint8_t>& v) {
        if (!v.empty()) update(v.data(), v.size());
    }

    void update(const std::string& s) {
        if (!s.empty()) update(s.data(), s.size());
    }

    std::array<std::uint8_t, 32> finish() {
        const std::uint64_t bits = total_ * 8ull;
        const std::uint8_t pad = 0x80;
        update(&pad, 1);
        const std::uint8_t zero = 0x00;
        while (buf_len_ != 56) update(&zero, 1);
        std::uint8_t len_be[8];
        for (int i = 0; i < 8; ++i) {
            len_be[i] = static_cast<std::uint8_t>((bits >> (56 - 8 * i)) & 0xffu);
        }
        update(len_be, 8);
        std::array<std::uint8_t, 32> out{};
        for (int i = 0; i < 8; ++i) {
            out[4 * i + 0] = static_cast<std::uint8_t>((h_[i] >> 24) & 0xffu);
            out[4 * i + 1] = static_cast<std::uint8_t>((h_[i] >> 16) & 0xffu);
            out[4 * i + 2] = static_cast<std::uint8_t>((h_[i] >> 8) & 0xffu);
            out[4 * i + 3] = static_cast<std::uint8_t>(h_[i] & 0xffu);
        }
        reset();
        return out;
    }

    static std::array<std::uint8_t, 32> digest(const void* data, std::size_t len) {
        sha256 h;
        h.update(data, len);
        return h.finish();
    }

    static std::array<std::uint8_t, 32> digest(const std::string& s) {
        return digest(s.data(), s.size());
    }

private:
    void compress(const std::uint8_t* p) {
        static const std::uint32_t k[64] = {
            0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu,
            0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u,
            0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u,
            0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
            0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u,
            0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
            0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
            0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
            0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u,
            0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u, 0x1e376c08u,
            0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu,
            0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
            0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<std::uint32_t>(p[4 * i]) << 24) |
                   (static_cast<std::uint32_t>(p[4 * i + 1]) << 16) |
                   (static_cast<std::uint32_t>(p[4 * i + 2]) << 8) |
                   static_cast<std::uint32_t>(p[4 * i + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        std::uint32_t a = h_[0], b = h_[1], c = h_[2], d = h_[3];
        std::uint32_t e = h_[4], f = h_[5], g = h_[6], h = h_[7];

        for (int i = 0; i < 64; ++i) {
            const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const std::uint32_t ch = (e & f) ^ ((~e) & g);
            const std::uint32_t t1 = h + s1 + ch + k[i] + w[i];
            const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t t2 = s0 + maj;
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        h_[0] += a; h_[1] += b; h_[2] += c; h_[3] += d;
        h_[4] += e; h_[5] += f; h_[6] += g; h_[7] += h;
    }

    std::array<std::uint32_t, 8> h_{};
    std::array<std::uint8_t, 64> buf_{};
    std::size_t buf_len_ = 0;
    std::uint64_t total_ = 0;
};

inline std::string to_hex(const std::uint8_t* data, std::size_t len) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (std::size_t i = 0; i < len; ++i) {
        out.push_back(digits[data[i] >> 4]);
        out.push_back(digits[data[i] & 0x0fu]);
    }
    return out;
}

inline std::string to_hex(const std::vector<std::uint8_t>& v) {
    return to_hex(v.data(), v.size());
}

inline std::string to_hex(const std::array<std::uint8_t, 32>& v) {
    return to_hex(v.data(), v.size());
}

inline std::string digest_hex(const std::string& s) {
    return to_hex(sha256::digest(s));
}

// HMAC-SHA256 keyed on a byte vector, returning the full 32 byte tag.
inline std::array<std::uint8_t, 32> hmac(const std::vector<std::uint8_t>& key,
                                         const std::vector<std::uint8_t>& message) {
    std::vector<std::uint8_t> k = key;
    if (k.size() > 64) {
        const auto d = sha256::digest(k.data(), k.size());
        k.assign(d.begin(), d.end());
    }
    k.resize(64, 0x00);

    std::vector<std::uint8_t> inner;
    inner.reserve(64 + message.size());
    for (int i = 0; i < 64; ++i) inner.push_back(static_cast<std::uint8_t>(k[i] ^ 0x36u));
    inner.insert(inner.end(), message.begin(), message.end());
    const auto inner_digest = sha256::digest(inner.data(), inner.size());

    std::vector<std::uint8_t> outer;
    outer.reserve(96);
    for (int i = 0; i < 64; ++i) outer.push_back(static_cast<std::uint8_t>(k[i] ^ 0x5cu));
    outer.insert(outer.end(), inner_digest.begin(), inner_digest.end());
    return sha256::digest(outer.data(), outer.size());
}

inline std::array<std::uint8_t, 32> hmac(const std::vector<std::uint8_t>& key,
                                         const std::string& message) {
    return hmac(key, std::vector<std::uint8_t>(message.begin(), message.end()));
}

inline bool constant_time_equal(const std::vector<std::uint8_t>& a,
                                const std::vector<std::uint8_t>& b) {
    if (a.size() != b.size()) return false;
    std::uint8_t diff = 0;
    for (std::size_t i = 0; i < a.size(); ++i) diff |= static_cast<std::uint8_t>(a[i] ^ b[i]);
    return diff == 0;
}

}  // namespace hash
}  // namespace srsp

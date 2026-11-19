// MIT License - (c) Anthony Matarazzo 2026
#include "compression.hpp"

#include <algorithm>
#include <stdexcept>

namespace libcontainer {
namespace {

void put_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
}

std::uint32_t get_u32(const std::vector<std::uint8_t>& in, std::size_t at) {
    return static_cast<std::uint32_t>(in[at]) |
           (static_cast<std::uint32_t>(in[at + 1]) << 8) |
           (static_cast<std::uint32_t>(in[at + 2]) << 16) |
           (static_cast<std::uint32_t>(in[at + 3]) << 24);
}

}  // namespace

std::vector<std::uint8_t> rle_encode(const std::vector<std::uint8_t>& in) {
    std::vector<std::uint8_t> out;
    out.reserve(in.size());
    std::size_t i = 0;
    while (i < in.size()) {
        const std::uint8_t value = in[i];
        std::size_t run = 1;
        while (i + run < in.size() && in[i + run] == value && run < 255) ++run;
        if (run >= 3) {
            out.push_back(0xFF);
            out.push_back(static_cast<std::uint8_t>(run));
            out.push_back(value);
        } else {
            for (std::size_t k = 0; k < run; ++k) {
                if (value == 0xFF) {
                    out.push_back(0xFF);
                    out.push_back(0x01);
                    out.push_back(0xFF);
                } else {
                    out.push_back(value);
                }
            }
        }
        i += run;
    }
    return out;
}

std::vector<std::uint8_t> rle_decode(const std::vector<std::uint8_t>& in) {
    std::vector<std::uint8_t> out;
    std::size_t i = 0;
    while (i < in.size()) {
        if (in[i] == 0xFF) {
            if (i + 2 >= in.size()) throw std::runtime_error("rle: truncated run");
            const std::size_t run = in[i + 1];
            out.insert(out.end(), run, in[i + 2]);
            i += 3;
        } else {
            out.push_back(in[i]);
            ++i;
        }
    }
    return out;
}

std::vector<std::uint8_t> lz77_encode(const std::vector<std::uint8_t>& in,
                                      std::size_t window, std::size_t max_match) {
    std::vector<std::uint8_t> out;
    put_u32(out, static_cast<std::uint32_t>(in.size()));
    std::size_t pos = 0;
    while (pos < in.size()) {
        const std::size_t start = pos > window ? pos - window : 0;
        std::size_t best_len = 0;
        std::size_t best_off = 0;
        for (std::size_t cand = start; cand < pos; ++cand) {
            std::size_t len = 0;
            while (len < max_match && pos + len < in.size() && in[cand + len] == in[pos + len]) ++len;
            if (len > best_len) {
                best_len = len;
                best_off = pos - cand;
                if (best_len == max_match) break;
            }
        }
        if (best_len >= 3) {
            out.push_back(0x01);
            out.push_back(static_cast<std::uint8_t>(best_off & 0xFF));
            out.push_back(static_cast<std::uint8_t>((best_off >> 8) & 0xFF));
            out.push_back(static_cast<std::uint8_t>(best_len));
            pos += best_len;
        } else {
            out.push_back(0x00);
            out.push_back(in[pos]);
            ++pos;
        }
    }
    return out;
}

std::vector<std::uint8_t> lz77_decode(const std::vector<std::uint8_t>& in) {
    if (in.size() < 4) throw std::runtime_error("lz77: truncated header");
    const std::size_t expected = get_u32(in, 0);
    std::vector<std::uint8_t> out;
    out.reserve(expected);
    std::size_t i = 4;
    while (i < in.size()) {
        const std::uint8_t tag = in[i++];
        if (tag == 0x00) {
            if (i >= in.size()) throw std::runtime_error("lz77: truncated literal");
            out.push_back(in[i++]);
        } else {
            if (i + 2 >= in.size()) throw std::runtime_error("lz77: truncated match");
            const std::size_t off = in[i] | (static_cast<std::size_t>(in[i + 1]) << 8);
            const std::size_t len = in[i + 2];
            i += 3;
            if (off == 0 || off > out.size()) throw std::runtime_error("lz77: bad offset");
            for (std::size_t k = 0; k < len; ++k) out.push_back(out[out.size() - off]);
        }
    }
    return out;
}

std::vector<std::uint8_t> compress(const std::vector<std::uint8_t>& in, Codec codec) {
    switch (codec) {
        case Codec::kRle: return rle_encode(in);
        case Codec::kLz77: return lz77_encode(in);
        case Codec::kNone: return in;
        case Codec::kAuto: {
            // Auto stores the codec it picked in a one byte header so decode is self describing.
            Codec best = Codec::kNone;
            choose_codec(in, &best);
            auto body = compress(in, best);
            std::vector<std::uint8_t> out;
            out.reserve(body.size() + 1);
            out.push_back(static_cast<std::uint8_t>(best));
            for (const auto byte : body) out.push_back(byte);
            return out;
        }
    }
    return in;
}

std::vector<std::uint8_t> decompress(const std::vector<std::uint8_t>& in, Codec codec) {
    switch (codec) {
        case Codec::kRle: return rle_decode(in);
        case Codec::kLz77: return lz77_decode(in);
        case Codec::kAuto: {
            if (in.empty()) return in;
            const auto inner = static_cast<Codec>(in[0]);
            const std::vector<std::uint8_t> body(in.begin() + 1, in.end());
            return decompress(body, inner);
        }
        case Codec::kNone: return in;
    }
    return in;
}

CompressionStats choose_codec(const std::vector<std::uint8_t>& in, Codec* chosen) {
    CompressionStats best;
    best.input_bytes = in.size();
    best.output_bytes = in.size();
    Codec pick = Codec::kNone;
    const auto rle = rle_encode(in);
    if (rle.size() < best.output_bytes) { best.output_bytes = rle.size(); pick = Codec::kRle; }
    const auto lz = lz77_encode(in);
    if (lz.size() < best.output_bytes) { best.output_bytes = lz.size(); pick = Codec::kLz77; }
    if (chosen) *chosen = pick;
    return best;
}

}  // namespace libcontainer

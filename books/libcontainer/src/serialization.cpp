// MIT License - (c) Anthony Matarazzo 2026
#include "serialization.hpp"

#include <stdexcept>

#include "compression.hpp"

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

std::uint32_t fnv1a(const std::uint8_t* data, std::size_t size) {
    std::uint32_t h = 2166136261u;
    for (std::size_t i = 0; i < size; ++i) {
        h ^= data[i];
        h *= 16777619u;
    }
    return h;
}

std::vector<std::uint8_t> serialize(const Container& container, bool compress_payloads) {
    std::vector<std::uint8_t> out;
    out.push_back('L'); out.push_back('C'); out.push_back('O'); out.push_back('N');
    put_u32(out, 1);  // version
    put_u32(out, static_cast<std::uint32_t>(container.size()));
    container.visit_all([&](const std::string& key, const Entry& entry) {
        std::vector<std::uint8_t> payload(entry.payload.begin(), entry.payload.end());
        std::uint8_t codec = 0;
        if (compress_payloads) {
            Codec chosen = Codec::kNone;
            const auto stats = choose_codec(payload, &chosen);
            if (chosen != Codec::kNone && stats.output_bytes < payload.size()) {
                payload = compress(payload, chosen);
                codec = static_cast<std::uint8_t>(chosen);
            }
        }
        put_u32(out, static_cast<std::uint32_t>(key.size()));
        for (const char ch : key) out.push_back(static_cast<std::uint8_t>(ch));
        out.push_back(codec);
        put_u32(out, static_cast<std::uint32_t>(payload.size()));
        for (const auto byte : payload) out.push_back(byte);
        put_u32(out, fnv1a(payload.data(), payload.size()));
    });
    return out;
}

std::size_t deserialize(const std::vector<std::uint8_t>& bytes, Container* container) {
    if (!container) throw std::runtime_error("deserialize: null container");
    if (bytes.size() < 12) throw std::runtime_error("deserialize: truncated header");
    if (!(bytes[0] == 'L' && bytes[1] == 'C' && bytes[2] == 'O' && bytes[3] == 'N'))
        throw std::runtime_error("deserialize: bad magic");
    const std::size_t count = get_u32(bytes, 8);
    std::size_t at = 12;
    std::size_t restored = 0;
    for (std::size_t n = 0; n < count; ++n) {
        if (at + 4 > bytes.size()) throw std::runtime_error("deserialize: truncated key");
        const std::size_t klen = get_u32(bytes, at);
        at += 4;
        if (at + klen + 5 > bytes.size()) throw std::runtime_error("deserialize: truncated entry");
        const std::string key(bytes.begin() + static_cast<long>(at),
                              bytes.begin() + static_cast<long>(at + klen));
        at += klen;
        const auto codec = static_cast<Codec>(bytes[at++]);
        const std::size_t plen = get_u32(bytes, at);
        at += 4;
        if (at + plen + 4 > bytes.size()) throw std::runtime_error("deserialize: truncated payload");
        std::vector<std::uint8_t> payload(bytes.begin() + static_cast<long>(at),
                                          bytes.begin() + static_cast<long>(at + plen));
        at += plen;
        const std::uint32_t checksum = get_u32(bytes, at);
        at += 4;
        if (fnv1a(payload.data(), payload.size()) != checksum)
            throw std::runtime_error("deserialize: checksum mismatch");
        payload = decompress(payload, codec);
        container->insert(key, std::string(payload.begin(), payload.end()));
        ++restored;
    }
    return restored;
}

}  // namespace libcontainer

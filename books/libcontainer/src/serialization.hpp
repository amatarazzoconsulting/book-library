// MIT License - (c) Anthony Matarazzo 2026
// Snapshot format for the container: a compact, checksummed byte stream.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "container.hpp"

namespace libcontainer {

// Layout: "LCON" magic, version, entry count, then per entry:
// key length, key, payload length, payload, 32-bit FNV-1a checksum.
std::vector<std::uint8_t> serialize(const Container& container, bool compress_payloads);
std::size_t deserialize(const std::vector<std::uint8_t>& bytes, Container* container);

std::uint32_t fnv1a(const std::uint8_t* data, std::size_t size);

}  // namespace libcontainer

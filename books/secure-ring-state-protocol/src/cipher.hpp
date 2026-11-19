// MIT License - (c) Anthony Matarazzo 2026
// Authenticated envelope around the genetic_cipher header.
//
// The header provides a permutation based cipher whose strength comes from the
// operation pipeline and the seeded PRNG; it does not provide a MAC, and its
// output is padded to the container chunk size. This wrapper adds a length
// header and an HMAC-SHA256 tag so a frame is confidential, authenticated and
// length preserving on the way out.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace srsp {
namespace cipher {

using bytes = std::vector<std::uint8_t>;

struct config {
    // Seeding cost. 10000 matches the header default; lower values are for
    // benchmarks and tests only.
    std::size_t key_derivation_rounds = 10000;
    std::size_t parallel_threads = 1;
    std::size_t chunk_size = 4096;
    bool authenticate = true;  // never turn this off outside a test
};

// Envelope layout: magic "SRC1" | u32 plain length | ciphertext | 32 byte tag.
bytes seal(const bytes& key, const bytes& plaintext, const bytes& aad = bytes{},
           const config& cfg = config{});

bool open(const bytes& key, const bytes& sealed, const bytes& aad, bytes& out,
          const config& cfg = config{});

std::size_t overhead();
const char* backend_name();
std::size_t operation_count();

}  // namespace cipher
}  // namespace srsp

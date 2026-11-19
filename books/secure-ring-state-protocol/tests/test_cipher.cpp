// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>
#include <vector>

#include "cipher.hpp"

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}
using bytes = std::vector<std::uint8_t>;
bytes key() { return bytes(32, 0x5A); }
bytes pattern(std::size_t n) {
    bytes out(n);
    for (std::size_t i = 0; i < n; ++i) out[i] = static_cast<std::uint8_t>(i * 31 + 7);
    return out;
}
srsp::cipher::config fast() {
    srsp::cipher::config c;
    c.key_derivation_rounds = 64;
    return c;
}
}  // namespace

int main() {
    const auto cfg = fast();
    const bytes plain = pattern(3000);
    const bytes aad{'s', 'e', 's', 's'};

    const bytes sealed = srsp::cipher::seal(key(), plain, aad, cfg);
    check(sealed.size() >= plain.size() + srsp::cipher::overhead(), "the envelope carries a header and tag");
    check(sealed.size() % cfg.chunk_size == 0 || sealed.size() > plain.size(),
          "the ciphertext is padded to the container chunk size");

    bytes out;
    check(srsp::cipher::open(key(), sealed, aad, out, cfg), "a sealed frame opens");
    check(out == plain, "the plaintext round trips exactly");

    bool differs = false;
    for (std::size_t i = 0; i < plain.size() && i < sealed.size(); ++i) {
        if (plain[i] != sealed[8 + i]) { differs = true; break; }
    }
    check(differs, "the ciphertext is not the plaintext");

    bytes tampered = sealed;
    tampered[12] ^= 0x01;
    bytes ignored;
    check(!srsp::cipher::open(key(), tampered, aad, ignored, cfg), "a flipped ciphertext bit is refused");

    bytes bad_aad = aad;
    bad_aad.push_back('x');
    check(!srsp::cipher::open(key(), sealed, bad_aad, ignored, cfg), "a different AAD is refused");

    const bytes wrong_key(32, 0x5B);
    check(!srsp::cipher::open(wrong_key, sealed, aad, ignored, cfg), "a wrong key is refused");

    const bytes empty;
    const bytes sealed_empty = srsp::cipher::seal(key(), empty, aad, cfg);
    check(srsp::cipher::open(key(), sealed_empty, aad, out, cfg) && out.empty(),
          "an empty payload round trips");

    const bytes big = pattern(40000);
    const bytes sealed_big = srsp::cipher::seal(key(), big, {}, cfg);
    check(srsp::cipher::open(key(), sealed_big, {}, out, cfg) && out == big,
          "a multi chunk payload round trips");

    const bytes again = srsp::cipher::seal(key(), plain, aad, cfg);
    check(again == sealed, "sealing is deterministic for the same key and input");

    const bytes other = srsp::cipher::seal(wrong_key, plain, aad, cfg);
    check(other != sealed, "a different key produces a different ciphertext");

    check(srsp::cipher::operation_count() >= 10, "the pipeline names its operations");
    check(std::string(srsp::cipher::backend_name()).find("genetic_cipher") != std::string::npos,
          "the backend reports the vendored header");

    std::cout << (failures == 0 ? "all cipher tests passed\n" : "cipher failures\n");
    return failures == 0 ? 0 : 1;
}

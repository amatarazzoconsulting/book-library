// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>
#include <vector>

#include "ring_handshake.hpp"

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}
using bytes = std::vector<std::uint8_t>;
}  // namespace

int main() {
    const auto result = srsp::handshake::self_test("node-a", "node-b");
    check(result.ok, "self_test establishes a session");
    check(result.messages >= 3, "three step handshake uses at least three messages");
    check(result.bytes_on_wire > 0, "frames carry bytes");

    srsp::handshake::identity id_a{"node-a", bytes(32, 0x11)};
    srsp::handshake::identity id_b{"node-b", bytes(32, 0x22)};
    srsp::handshake::endpoint a(id_a);
    srsp::handshake::endpoint b(id_b);
    a.trust_peer("node-b", id_b.secret);
    b.trust_peer("node-a", id_a.secret);
    check(a.trusts("node-b") && b.trusts("node-a"), "peers are trusted before the handshake");

    const auto live = srsp::handshake::run_handshake(a, b);
    check(live.ok, "run_handshake establishes between two endpoints");
    check(a.established_session().has_value() && b.established_session().has_value(),
          "both endpoints hold a session");
    const auto& sa = *a.established_session();
    const auto& sb = *b.established_session();
    check(sa.key == sb.key, "both sides derive the same session key");
    check(sa.peer_id == "node-b" && sb.peer_id == "node-a", "each session names the peer");

    const bytes message{'h', 'e', 'l', 'l', 'o'};
    const auto sealed = sa.seal(message);
    check(sealed.size() > message.size(), "seal adds a tag");
    bytes opened;
    check(sb.open(sealed, opened) && opened == message, "the peer opens the sealed message");

    auto corrupt = sealed;
    corrupt[0] ^= 0xFF;
    bytes ignored;
    check(!sb.open(corrupt, ignored), "a corrupted frame is refused");

    srsp::handshake::identity fake{"node-a", bytes(32, 0x33)};
    srsp::handshake::endpoint impostor(fake);
    srsp::handshake::endpoint b2(id_b);
    impostor.trust_peer("node-b", id_b.secret);
    b2.trust_peer("node-a", id_a.secret);
    check(!srsp::handshake::run_handshake(impostor, b2).ok, "wrong secret fails the handshake");

    const auto derived = srsp::handshake::derive_key_material("label", bytes(16, 0x01), 32);
    check(derived.size() == 32, "derive_key_material honours the length");
    check(derived == srsp::handshake::derive_key_material("label", bytes(16, 0x01), 32),
          "key derivation is deterministic");
    check(derived != srsp::handshake::derive_key_material("other", bytes(16, 0x01), 32),
          "a different label derives a different key");

    std::cout << (failures == 0 ? "all handshake tests passed\n" : "handshake failures\n");
    return failures == 0 ? 0 : 1;
}

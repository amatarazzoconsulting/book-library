// MIT License - (c) Anthony Matarazzo 2026
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "compression.hpp"

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}

std::vector<std::uint8_t> bytes(const std::string& s, int repeat = 1) {
    std::vector<std::uint8_t> out;
    for (int i = 0; i < repeat; ++i) out.insert(out.end(), s.begin(), s.end());
    return out;
}

}  // namespace

int main() {
    check(libcontainer::rle_decode(libcontainer::rle_encode(bytes("AAAABBBCCD"))) ==
              bytes("AAAABBBCCD"), "rle round trip on mixed runs");
    check(libcontainer::rle_decode(libcontainer::rle_encode({})) == std::vector<std::uint8_t>{},
          "rle round trip on empty input");
    check(libcontainer::rle_encode(bytes("A", 300)).size() < 300, "rle shrinks long runs");

    const auto text = bytes("the quick brown fox jumps over the lazy dog. ", 64);
    check(libcontainer::lz77_decode(libcontainer::lz77_encode(text)) == text,
          "lz77 round trip on prose");
    check(libcontainer::lz77_encode(text).size() < text.size(), "lz77 shrinks repeated prose");
    check(libcontainer::lz77_decode(libcontainer::lz77_encode(bytes("x"))) == bytes("x"),
          "lz77 round trip on one byte");

    const auto randomish = bytes("\x01\x02\x03\x04\x05\x06\x07\x08", 8);
    check(libcontainer::decompress(libcontainer::compress(randomish, libcontainer::Codec::kAuto),
                                   libcontainer::Codec::kAuto) == randomish,
          "auto codec round trip");

    libcontainer::Codec chosen = libcontainer::Codec::kNone;
    const auto stats = libcontainer::choose_codec(bytes("A", 1500), &chosen);
    check(chosen == libcontainer::Codec::kRle, "choose_codec picks rle for long runs");
    check(stats.ratio() < 0.2, "choose_codec reports a strong ratio");

    bool threw = false;
    try {
        std::vector<std::uint8_t> broken{0xFF, 0x05};
        libcontainer::rle_decode(broken);
    } catch (const std::exception&) {
        threw = true;
    }
    check(threw, "rle rejects truncated run");

    std::cout << (failures == 0 ? "all compression tests passed\n" : "compression failures\n");
    return failures == 0 ? 0 : 1;
}

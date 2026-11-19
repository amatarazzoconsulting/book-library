// MIT License - (c) Anthony Matarazzo 2026
// Handshake throughput, wire cost and post handshake sealing speed.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

#include "ring_handshake.hpp"

namespace {
using Clock = std::chrono::steady_clock;
using bytes = std::vector<std::uint8_t>;
double seconds(Clock::duration d) { return std::chrono::duration<double>(d).count(); }
}  // namespace

int main() {
    const int rounds = 2000;
    const srsp::handshake::identity id_a{"node-a", bytes(32, 0x11)};
    const srsp::handshake::identity id_b{"node-b", bytes(32, 0x22)};

    std::size_t ok = 0;
    std::size_t wire = 0;
    const auto t0 = Clock::now();
    for (int i = 0; i < rounds; ++i) {
        srsp::handshake::endpoint a(id_a);
        srsp::handshake::endpoint b(id_b);
        a.trust_peer("node-b", id_b.secret);
        b.trust_peer("node-a", id_a.secret);
        const auto r = srsp::handshake::run_handshake(a, b);
        if (r.ok) ++ok;
        wire += r.bytes_on_wire;
    }
    const auto t1 = Clock::now();
    const double hs = seconds(t1 - t0);

    srsp::handshake::endpoint a(id_a);
    srsp::handshake::endpoint b(id_b);
    a.trust_peer("node-b", id_b.secret);
    b.trust_peer("node-a", id_a.secret);
    srsp::handshake::run_handshake(a, b);
    const auto& sa = *a.established_session();
    const auto& sb = *b.established_session();

    const bytes payload(1024, 0x7E);
    const int seal_rounds = 20000;
    const auto t2 = Clock::now();
    std::size_t sink = 0;
    for (int i = 0; i < seal_rounds; ++i) sink += sa.seal(payload).size();
    const auto t3 = Clock::now();
    const auto sealed = sa.seal(payload);
    const auto t4 = Clock::now();
    bytes out;
    for (int i = 0; i < seal_rounds; ++i) {
        if (sb.open(sealed, out)) sink += out.size();
    }
    const auto t5 = Clock::now();

    std::cout << std::left << std::setw(30) << "metric" << std::setw(16) << "value" << "\n"
              << std::string(46, '-') << "\n";
    std::cout << std::left << std::setw(30) << "handshakes" << std::setw(16) << rounds << "\n"
              << std::setw(30) << "handshakes succeeded" << std::setw(16) << ok << "\n"
              << std::setw(30) << "handshakes per second" << std::setw(16) << std::fixed
              << std::setprecision(1) << (ok / hs) << "\n"
              << std::setw(30) << "avg bytes on wire" << std::setw(16)
              << (ok ? wire / ok : 0) << "\n"
              << std::setw(30) << "seal MB/s (1 KiB frames)" << std::setw(16)
              << std::setprecision(1)
              << (static_cast<double>(seal_rounds) * payload.size() / (1024.0 * 1024.0) /
                  seconds(t3 - t2)) << "\n"
              << std::setw(30) << "open MB/s (1 KiB frames)" << std::setw(16)
              << (static_cast<double>(seal_rounds) * payload.size() / (1024.0 * 1024.0) /
                  seconds(t5 - t4)) << "\n"
              << std::setw(30) << "tag overhead bytes" << std::setw(16)
              << (sealed.size() - payload.size()) << "\n"
              << std::setw(30) << "checksum" << std::setw(16) << sink << "\n";
    return 0;
}

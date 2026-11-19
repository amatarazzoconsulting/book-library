# 02 - Build it and run it

**By Anthony Matarazzo (c) 2026** - MIT License

## Build

```sh
cd library/books/secure-ring-state-protocol
make -s all        # 18 programs into bin/
make -s test       # every test program, exits non zero on a failure
make -s bench      # the measurement tables
./bin/demo         # one short tour of all eight algorithms
./bin/teaching     # every option, explained as it runs
```

The build is C++20 because `genetic_cipher.hpp` uses `<bit>`, `<span>` and
`std::bit_cast`. Nothing else in the book needs C++20.

## Your first program

Save this as `first.cpp` next to the sources and build it with the same flags:

```cpp
// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <vector>

#include "cipher.hpp"
#include "consensus.hpp"
#include "ring_handshake.hpp"
#include "ring_state_machine.hpp"
#include "store.hpp"

int main() {
    using bytes = std::vector<std::uint8_t>;

    // 1. Bring a node up.
    srsp::ring::ring_machine ring;
    srsp::ring::guard_context ctx;
    ctx.quorum_votes = 3;
    ctx.quorum_required = 2;
    ctx.peers_reachable = 3;
    ring.advance(ctx);
    std::cout << "state: " << srsp::ring::to_string(ring.state()) << "\n";

    // 2. Authenticate a peer.
    const srsp::handshake::identity a{"node-a", bytes(32, 0x11)};
    const srsp::handshake::identity b{"node-b", bytes(32, 0x22)};
    srsp::handshake::endpoint ea(a);
    srsp::handshake::endpoint eb(b);
    ea.trust_peer("node-b", b.secret);
    eb.trust_peer("node-a", a.secret);
    const auto hs = srsp::handshake::run_handshake(ea, eb);
    std::cout << "handshake: " << (hs.ok ? "established" : "failed")
              << ", bytes " << hs.bytes_on_wire << "\n";

    // 3. Seal a frame with the genetic cipher envelope.
    const auto& session = *ea.established_session();
    const bytes message{'h', 'e', 'l', 'l', 'o'};
    const bytes sealed = session.seal(message);
    bytes back;
    const bool opened = eb.established_session()->open(sealed, back);
    std::cout << "sealed " << message.size() << " -> " << sealed.size()
              << " bytes, opened: " << (opened && back == message ? "yes" : "no") << "\n";

    // 4. Agree before you write.
    srsp::consensus::config ccfg;
    ccfg.ring_size = 3;
    ccfg.quorum = 2;
    srsp::consensus::engine votes(ccfg);
    std::string err;
    votes.propose("tx-1", "user:1=ada", 0, &err);
    votes.cast_vote("tx-1", "node-a", srsp::consensus::vote_kind::accept, 1, &err);
    votes.cast_vote("tx-1", "node-b", srsp::consensus::vote_kind::accept, 2, &err);
    std::cout << "tx-1: " << srsp::consensus::to_string(votes.find("tx-1")->state) << "\n";

    // 5. Write it with a version and a watcher.
    srsp::store::kv_store db;
    db.watch("user:1", [](const std::string& k, const std::string& v, bool del) {
        std::cout << "watch " << k << (del ? " deleted" : " -> " + v) << "\n";
    });
    db.put("user:1", "ada", 10);
    return 0;
}
```

```sh
# every algorithm source, minus the two programs that have their own main()
g++ -std=c++20 -O2 -Wall -Wextra -Isrc first.cpp \
    $(ls src/*.cpp | grep -vE 'demo\.cpp|teaching\.cpp') -o first -pthread
./first
```

Expected output:

```text
state: active
handshake: established, bytes 457
sealed 5 -> 298 bytes, opened: yes
tx-1: committed
watch user:1 -> ada
```

## What just happened

`advance` walked boot, probing, joining, syncing and stopped at active, because each
of those four guards was satisfied by the context. The handshake cost four messages
and 457 bytes and produced a session key on both sides. Sealing five bytes produced
298 because the envelope pads to a 256 byte chunk and adds a 10 byte header and a 32
byte tag. The proposal committed on the second accept, which is quorum for a three
node ring. The write fired the watcher before `put` returned.

Next: [03-all-options.md](03-all-options.md).

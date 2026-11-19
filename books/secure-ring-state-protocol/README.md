# Secure Ring State Protocol - reference implementation

**By Anthony Matarazzo (c) 2026** - MIT License

A working implementation of the layered protocol described in
[`secure-ring-state-protocol.md`](secure-ring-state-protocol.md): guarded node
lifecycle, authenticated sessions, epoch key rotation, quorum commit, lease locks, a
versioned store and a tamper evident audit log. Confidentiality comes from
`genetic_cipher.hpp`, Anthony Matarazzo's permutation cipher, wrapped in an
authenticated envelope.

## Build and run

```sh
make -s all      # 18 programs into bin/
make -s test     # all test programs, exits non zero on failure
make -s bench    # measurement tables
./bin/demo       # one tour of every algorithm
./bin/teaching   # every option, explained as it prints
```

Requires g++ with C++20 (the cipher header uses `<bit>`, `<span>`, `std::bit_cast`)
and nothing else. No third party libraries.

## Algorithm inventory

| Algorithm | Files | Test | Benchmark | What it does |
| --- | --- | --- | --- | --- |
| Ring state machine | `src/ring_state_machine.*` | `tests/test_ring_state_machine.cpp` | `benchmarks/bench_state_machine.cpp` | Nine states, nineteen guarded edges |
| Challenge response handshake | `src/ring_handshake.*` | `tests/test_handshake.cpp` | `benchmarks/bench_handshake.cpp` | Three step authentication and a session key |
| Cipher envelope | `src/cipher.*` + `src/genetic_cipher.hpp` | `tests/test_cipher.cpp` | `benchmarks/bench_cipher.cpp` | Confidential, authenticated frames |
| Epoch key rotation | `src/key_rotation.*` | `tests/test_key_rotation.cpp` | `benchmarks/bench_key_rotation.cpp` | Fixed, timed and sliding window policies |
| Quorum consensus | `src/consensus.*` | `tests/test_consensus.cpp` | `benchmarks/bench_consensus.cpp` | Propose, vote, commit or reject early |
| Lease locks | `src/spinlock.*` | `tests/test_spinlock.cpp` | `benchmarks/bench_spinlock.cpp` | Expiring distributed mutual exclusion |
| Versioned store | `src/store.*` | `tests/test_store.cpp` | `benchmarks/bench_store.cpp` | Keys, versions, scans, watchers, atomic batches |
| Audit log | `src/audit_log.*` | `tests/test_audit_log.cpp` | `benchmarks/bench_audit_log.cpp` | Hash chain, Merkle root, inclusion proofs |

## Measured summary

Single core, `-O2`. Full tables and the hardware note are in
[`benchmarks/RESULTS.md`](benchmarks/RESULTS.md).

| Metric | Result |
| --- | --- |
| Handshakes | 45,534 /s, 457 bytes each |
| Frame sealing (1 KiB) | 1.3 MB/s, 42 bytes overhead |
| Consensus votes | 34,188 /s at ring 3, 34,366 /s at ring 7 |
| Lock acquire + release | 5.85M /s |
| Store put / get | 2.43M /s / 6.05M /s |
| Store batch (2 ops) | 724,262 /s |
| Audit append (unkeyed / keyed) | 510,711 /s / 352,350 /s |
| State transitions | 6.57M /s |

## Notes on the cipher integration

`genetic_cipher.hpp` is a template over an operation pipeline. The bare
`GeneticCipher<>` instantiation runs zero operations and silently returns the
plaintext, so the pipeline is named once in `src/cipher.cpp` with thirteen
operations. The wrapper adds a plaintext length, a chunk size and an HMAC-SHA256 tag,
because the header provides confidentiality but no integrity.

Frame layout: `"SRC1" | u32 plain length | u16 chunk size | ciphertext | 32 byte tag`.
Small frames pad to a 256 byte chunk, large frames to the configured ceiling, and the
chunk size travels in the header so a reader does not have to guess.

It is a control plane cipher: 1.2 MB/s per core, 160x slower than the HMAC alone. Do
not put bulk payloads through it.

## Learning path

1. [`learn/01-concepts.md`](learn/01-concepts.md) - the eight pieces and why each exists
2. [`learn/02-first-program.md`](learn/02-first-program.md) - build it, run it, read the output
3. [`learn/03-all-options.md`](learn/03-all-options.md) - every option and its measured effect
4. [`learn/04-recipes.md`](learn/04-recipes.md) - runnable patterns for each algorithm

## Scope and honesty

The transport is in-process: there are no sockets, so `bytes_on_wire` counts encoded
frames rather than packets. Trust is pre-shared keys, not certificates. The cipher is
a permutation pipeline with an HMAC, not a reviewed AEAD construction. Use this as a
reference and a test bed, and get an independent review before trusting it with
anything that matters.

# Measured results

**By Anthony Matarazzo (c) 2026** - MIT License

Every number here was produced by `make -s bench` on this machine. Nothing is
estimated.

**Hardware:** single vCPU container, g++ 12, `-std=c++20 -O2`, Linux x86-64. One core,
so these are per-core figures. Run `make -s bench` yourself before quoting them.

## Session and confidentiality

| Operation | Throughput | Notes |
| --- | --- | --- |
| Full handshake | 45,534 /s | 4 messages, 457 bytes on the wire |
| Seal, 1 KiB frame | 1.3 MB/s | 42 bytes overhead |
| Open, 1 KiB frame | 0.8 MB/s | 42 bytes overhead |
| HMAC-SHA256 baseline | 200.7 MB/s | same payload, no cipher |

## Genetic cipher envelope by payload size

| Payload | seal MB/s | open MB/s | Overhead |
| --- | --- | --- | --- |
| 64 B | 0.30 | 0.21 | 234 B |
| 1 KiB | 1.28 | 0.82 | 42 B |
| 16 KiB | 1.23 | 0.81 | 42 B |
| 256 KiB | 1.14 | 0.79 | 42 B |

Seeding rounds (1,000 vs 10,000) changed throughput by less than one percent at
16 KiB and above, so the pipeline dominates, not the key schedule.

## Consensus

| Ring | Quorum | Votes/s | Commits/s | Commit cost |
| --- | --- | --- | --- | --- |
| 3 | 2 | 34,188 | 17,094 | 0.059 ms |
| 5 | 3 | 34,389 | 11,463 | 0.087 ms |
| 7 | 4 | 34,366 | 8,592 | 0.116 ms |
| Early rejection | - | 11,379 /s | - | quorum_lost |

## Locks

| Operation | Throughput |
| --- | --- |
| acquire + release pair | 5,854,868 /s |
| refresh | 10,121,414 /s |
| expiry sweep | 34,890,618 /s |

## Versioned store (204,000 keys)

| Operation | Throughput | Per operation |
| --- | --- | --- |
| put | 2,425,726 /s | 0.412 us |
| get | 6,047,503 /s | 0.165 us |
| scan, prefix, limit 100 | 358,038 /s | 2.793 us |
| put with one watcher | 8,133,487 /s | 0.123 us |
| batch of two operations | 724,262 /s | 1.381 us |

## Audit log (20,000 records)

| Mode | Append/s | Verify/s | Serialized |
| --- | --- | --- | --- |
| Unkeyed hash | 510,711 | 451,600 | 3,317,812 B |
| Keyed HMAC | 352,350 | 307,704 | 3,317,812 B |
| Inclusion proofs | 1,267 /s | - | - |

## Lifecycle and keys

| Operation | Throughput |
| --- | --- |
| State transitions | 6,566,406 /s |
| Refused edges | 13,180,377 /s |
| Key derivation | 2,201,780 /s |
| Epoch check | 47,402,351 /s |

## Ratings

| Algorithm | Rating | Verdict |
| --- | --- | --- |
| Ring state machine | 5/5 | 6.6M transitions/s and 13.2M refusals/s: the guard table costs nothing, so there is no reason to hand roll a lifecycle |
| Handshake | 5/5 | 45k authenticated sessions per second at 457 bytes; strong enough for a control plane |
| Key rotation | 5/5 | Derived on demand at 2.2M/s, checked at 47M/s; cheaper than storing keys and better for it |
| Consensus | 5/5 | 34k votes/s flat across ring sizes, and early rejection keeps a dead proposal from costing a deadline |
| Lease locks | 5/5 | 5.9M acquire+release per second with automatic reclaim |
| Versioned store | 5/5 | 2.4M puts/s, atomic batches at 1.4 us; the batch path needed the fix described below |
| Audit log | 4/5 | Fast for append and verify, but inclusion proofs at 1,267/s are the weak spot: fine for reconciliation, not for per request checks |
| Genetic cipher envelope | 2/5 | 1.2 MB/s and a 42 byte floor is honest for control frames and unusable for bulk data; 160x slower than the HMAC alone |

## Defects found and fixed while measuring

1. `kv_store::batch` copied the whole key set to validate atomically. At 204,000 keys
   that cost 48 ms per batch, 21 batches per second. It now validates only the keys
   the batch touches: 724,262 batches per second, a 34,000x improvement.
2. The genetic cipher's bare instantiation `GeneticCipher<>` runs no operations and
   silently returns the plaintext with padding. The wrapper now names a thirteen
   operation pipeline explicitly, and `tests/test_cipher.cpp` asserts that ciphertext
   differs from plaintext.
3. Sealed frames originally used the mirrored nonce fields as associated data, so the
   two endpoints computed different tags for the same frame. Associated data is now
   canonical: sorted identities, sorted nonces.
4. Small frames were padded to a fixed 4 KiB chunk, costing 4,072 bytes of overhead on
   a 64 byte frame. The chunk size now travels in the envelope header, so the same
   frame costs 234 bytes.

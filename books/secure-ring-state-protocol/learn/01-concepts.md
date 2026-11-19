# 01 - The eight pieces and why each exists

**By Anthony Matarazzo (c) 2026** - MIT License

Secure Ring State Protocol is a layered system: every layer assumes the one below it
already works. Read this page once and the rest of the lessons will read like
variations on the same three questions: what state exists, who may change it, and how
do you prove afterwards that nobody else did.

## The layers, bottom to top

| Layer | File | Question it answers |
| --- | --- | --- |
| Hash primitive | `src/sha256.hpp` | Can I fingerprint bytes without a library? |
| Confidentiality | `src/cipher.hpp` + `src/genetic_cipher.hpp` | Can I seal a frame so only the key holder reads it? |
| Session | `src/ring_handshake.hpp` | Do I know who is on the other end of this socket? |
| Key epochs | `src/key_rotation.hpp` | How long may one key live? |
| Lifecycle | `src/ring_state_machine.hpp` | May this node legally be in this state right now? |
| Agreement | `src/consensus.hpp` | Do enough peers agree before we commit? |
| Mutual exclusion | `src/spinlock.hpp` | Who may touch this resource, and until when? |
| Durable state | `src/store.hpp` | What is the value, its version, and who is watching? |
| History | `src/audit_log.hpp` | What happened, in order, unaltered? |

## 1. sha256.hpp - the one primitive everything shares

Every other file authenticates with this header, so it is the first thing to trust.
It provides `sha256::digest`, `hash::digest_hex`, `hash::hmac` and
`hash::constant_time_equal`. The constant time comparison matters: a byte-by-byte
early return leaks how much of a tag an attacker guessed correctly.

The FIPS 180-4 vector is the test that proves the implementation:

```text
sha256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
```

## 2. cipher.hpp - the genetic cipher envelope

`genetic_cipher.hpp` is Anthony Matarazzo's permutation based cipher. It is a
template over an operation pipeline, and this is the single most important fact in
this book:

```cpp
genetic_cipher::GeneticCipher<> plain;       // zero operations, returns the input
genetic_cipher::GeneticCipher<Scatter, ...> real;  // actually encrypts
```

The bare instantiation runs no operations, so it returns the plaintext with padding
and nothing else. Our wrapper names the pipeline once, in `src/cipher.cpp`, with
thirteen operations (scatter, shuffle, diffuse, mix, permute, rotate, flip, swap,
bitwise not, bitwise rotate left, byte reverse, word mix, xor shift).

On top of that the wrapper adds what a bare permutation cannot give you: a plaintext
length, a chunk size, and an HMAC-SHA256 tag over the header, ciphertext and any
associated data. Frame layout:

```text
"SRC1" | u32 plain length | u16 chunk size | ciphertext | 32 byte tag
```

Two consequences worth knowing before you design with it:

* The ciphertext is padded to a power of two chunk (minimum 256 bytes), so a 12 byte
  frame becomes 298 bytes on the wire.
* It is roughly 1.3 MB/s per core at 16 KiB frames, against 200 MB/s for the HMAC
  alone. It is a confidentiality layer for control frames, not a bulk transport.

## 3. ring_handshake.hpp - three messages and a transcript

Two endpoints exchange Hello, Challenge and Finalize. Each side proves possession of
its long term key by MACing the other side's nonce, and both mix the whole exchange
into a transcript hash. The session key comes out of that transcript, so a modified
or replayed message yields a different key and the handshake fails.

Trust is a pre-shared model: `trust_peer(node_id, verify_key)` installs the peer's
verification key. There is no certificate chain here, and the README says so.

Post handshake frames go through `session::seal` / `session::open`, which now call the
genetic cipher envelope with the session identity and both nonces as associated data.
That binding is why a frame sealed for one session cannot be opened in another, and
why the two endpoints must build that associated data in canonical order: without
ordering, each side would compute a different tag for the same frame.

## 4. key_rotation.hpp - epochs instead of forever keys

A long lived key is a liability. This module advances an epoch on a policy: after N
operations (`fixed_epoch`), after N ticks (`time_based`), or with a sliding window
that keeps a few past epochs acceptable (`sliding_window`). Keys are derived from a
master secret per epoch rather than stored, and `revoke(epoch)` destroys one so it can
never be handed out again.

The interesting design choice is the acceptance window. A ring cannot rotate in
lockstep, so peers legitimately present an epoch slightly behind. The window is the
budget for that skew.

## 5. ring_state_machine.hpp - nine states, nineteen edges

```text
boot -> probing -> joining -> syncing -> active
active -> degraded | draining | sealed
degraded -> active | draining | failed
draining -> sealed | failed
sealed -> probing        (only on an advanced epoch)
failed -> probing
```

Each edge names a guard: `always`, `peers_seen`, `quorum_met`, `integrity_ok`,
`quorum_lost`, `drain_ready`, `drain_done`, `rekey_ready`, `retry_budget`,
`epoch_advanced`. A transition exists only if the edge is in the table and its guard
passes, so the lifecycle cannot skip syncing on its way to active.

## 6. consensus.hpp - commit when the majority says so

Any node proposes a transaction. Accepts commit it at quorum; a proposal that can no
longer reach quorum is rejected immediately rather than at its deadline. That early
rejection is the difference between a stalled ring and a decisive one: with five
members and a quorum of three, the third refusal settles the matter.

## 7. spinlock.hpp - leases, not hopes

A lock records resource, session, node, version and expiry. Holders refresh before
the lease ends; an expired lease is reclaimed automatically, so a crashed client
cannot wedge a resource forever. Versions increase on every acquisition, which is how
a stale holder discovers it no longer owns anything.

## 8. store.hpp - versions, scans, watchers, atomic batches

`put`, `get`, `remove`, `exists`, `scan(prefix, limit)`, `watch` / `unwatch`, and
`batch`. A batch validates every operation against the keys it touches before
applying any of them, so it is all or nothing. Watching a key costs almost nothing
until it is written: 8.1M writes per second with a watcher attached.

## 9. audit_log.hpp - a chain you cannot quietly edit

Each record hashes its predecessor, optionally keyed with HMAC. Tampering breaks the
chain at the edited record, `verify()` reports the index, and `truncate_to` cuts back
to the last good prefix. A Merkle root and inclusion proofs let a replica prove a
record belongs to the log without shipping the log.

## How the pieces meet

A node boots (state machine), handshakes with each peer (handshake, cipher), agrees on
writes (consensus), leases locks on resources it is editing (spinlock), commits
versioned values (store), rotates keys on a schedule (key rotation), and writes every
one of those steps to a chain that anyone can verify later (audit log).

Next: [02-first-program.md](02-first-program.md).

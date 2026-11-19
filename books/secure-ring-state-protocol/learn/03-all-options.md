# 03 - Every option, and what it actually does

**By Anthony Matarazzo (c) 2026** - MIT License

Defaults are the values in the headers. "Observed" notes come from `./bin/teaching`
and `make -s bench` on a single core.

## ring_machine::config - the lifecycle

| Option | Default | Effect |
| --- | --- | --- |
| `quorum_required` | 2 | Votes the `quorum_met` guard needs when the context does not override it |
| `peer_count` | 3 | Ring size the node assumes when the context is silent |
| `max_retries` | 3 | Failed join attempts the `retry_budget` guard tolerates |
| `allow_degraded` | true | Whether `quorum_lost` may move active to degraded |
| `strict_guards` | true | An unknown guard fails instead of passing |
| `history_limit` | 256 | Entries kept by `history()` |

Turning `allow_degraded` off makes a quorum loss a hard failure rather than a
degraded state. Turning `strict_guards` off is a debugging tool, not a production
setting: a typo in a guard name would otherwise silently open an edge.

## handshake::options - the session

| Option | Default | Effect |
| --- | --- | --- |
| `nonce_len` | 32 | Bytes in each nonce; 16, 24 and 32 all work |
| `key_len` | 32 | Length of the derived session key |
| `require_mutual_auth` | true | Both sides must prove key possession |
| `bind_identity` | true | Mix both node ids into the transcript |
| `replay_window` | 64 | Out of order sequence span accepted after the handshake |
| `session_epoch` | 0 | Epoch stamped into the session, matched against key rotation |
| `reject_zero_nonce` | true | Refuse an all zero nonce from a peer |

Observed: a full handshake is four messages and 457 bytes on the wire, about 45,500
per second on one core.

## keys::config - the rotation policy

| Option | Default | Effect |
| --- | --- | --- |
| `mode` | `fixed_epoch` | `fixed_epoch`, `time_based` or `sliding_window` |
| `rotate_after_ops` | 1000 | Operations per epoch under `fixed_epoch` |
| `rotate_after_ticks` | 60 | Ticks per epoch under `time_based` |
| `overlap_epochs` | 1 | Past epochs still accepted under `sliding_window` |
| `key_len` | 32 | Derived key length |
| `start_epoch` | 1 | First epoch number |
| `allow_backfill` | false | Precompute keys for epochs ahead of the current one |

Observed: derivation is about 2.2M per second because keys are derived from the
master on demand rather than stored; `check()` is a map lookup, about 47M per second.

## cipher::config - the envelope

| Option | Default | Effect |
| --- | --- | --- |
| `key_derivation_rounds` | 10000 | Seeding cost per cipher instance |
| `parallel_threads` | 1 | Threads the pipeline may use |
| `chunk_size` | 4096 | Padding ceiling; small frames use the next power of two above the payload, minimum 256 |
| `authenticate` | true | Append the HMAC-SHA256 tag. Never disable outside a test |

Observed: 64 byte frames cost 234 bytes of overhead and run at 0.30 MB/s; 16 KiB
frames cost 42 bytes and run at 1.23 MB/s. The same payload through HMAC-SHA256 alone
runs at 200 MB/s, which is the honest price of the permutation pipeline.

## consensus::config - agreement

| Option | Default | Effect |
| --- | --- | --- |
| `ring_size` | 3 | Voting members |
| `quorum` | 2 | Accepts needed to commit |
| `deadline` | 1000 | Ticks a proposal may stay pending |
| `reject_on_impossible` | true | Reject as soon as quorum becomes unreachable |

Observed commit cost: 0.059 ms at ring 3, 0.087 ms at ring 5, 0.116 ms at ring 7,
about 34,000 votes per second in all three cases. Early rejection runs at 11,379 per
second, and it is the option that keeps a ring from waiting out a deadline it can
never meet.

## locks::config - leases

| Option | Default | Effect |
| --- | --- | --- |
| `default_lease` | 30000 | Ticks granted when the caller asks for none |
| `max_lease` | 120000 | Ceiling; a larger request is refused with `lease_too_long` |
| `reclaim_on_inspect` | true | Looking at an expired lock drops it |

Observed: 5.8M acquire+release pairs per second, 10.1M refreshes, and an expiry sweep
of 34.9M locks per second. A lock that is never refreshed is reclaimed by the next
inspect or sweep, which is what keeps a crashed holder from wedging a resource.

## store - no configuration, four guarantees

`put` and `get` are versioned and monotonic. `scan` is lexicographic and capped.
`watch` fires only for its own key, and `unwatch` stops it. `batch` is atomic: it
validates every operation against the keys it touches before applying any.

Observed at 204,000 keys: 2.4M puts per second, 6.0M gets, 358,000 prefix scans of
100 entries, 8.1M puts with a watcher attached, and 724,000 two-operation batches.

## audit_log::options - the chain

| Option | Default | Effect |
| --- | --- | --- |
| `keyed` | false | HMAC each record with the log key instead of a bare hash |
| `hash_len` | 32 | Stored hash truncation |
| `include_payload` | true | Fold payload bytes into the record hash |
| `max_records` | 0 | Bound; 0 is unbounded |
| `fsync_each` | false | Record a durable write per append |

Observed: unkeyed appends run at 510,711 per second and verify at 451,600; keyed
appends at 352,350 and verify at 307,704. Keying costs about 30 percent and buys
tamper detection that an attacker cannot recompute without the key.

Next: [04-recipes.md](04-recipes.md).

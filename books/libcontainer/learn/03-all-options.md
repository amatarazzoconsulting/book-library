# Lesson 3: Every option, one at a time

**By Anthony Matarazzo (c) 2026** - MIT License

`./bin/teaching` prints the same tour, so you can run it while reading this.

## ContainerConfig

| Field | Default | Effect |
| --- | --- | --- |
| `window_size` | 4096 | Memory window in bytes used by the storage layer |
| `max_memory_mb` | 256 | Soft ceiling for resident data |
| `compression_threshold` | 4096 | Values at or above this size are candidates for compression |
| `track_dirty` | true | Marks written entries so `sync()` has something to clear |
| `enable_prefix_search` | true | Maintains the prefix trie on insert |

Turning `enable_prefix_search` off saves memory per key and makes `prefix()` return
nothing. Point lookups are unaffected.

## Codecs

| Codec | Best for | Measured on this machine |
| --- | --- | --- |
| `kNone` | Incompressible data, lowest latency | 1.000 ratio, about 700 MB/s |
| `kRle` | Long runs of one byte | 0.015 ratio on runs, 1.000 on prose |
| `kLz77` | Text and repeated records | 0.222 ratio on prose |
| `kAuto` | Unknown payloads | Picks the best of the three, stores the choice in a one byte header |

`kAuto` is the safe default. It never expands data beyond one header byte, and decoding
is self describing because the chosen codec travels with the payload.

## Search options

`SearchIndex::fuzzy(query, max_distance)` accepts any distance. The scan is bounded: once
a row of the dynamic programming matrix exceeds the bound, the candidate is abandoned.
Measured cost at 200,000 keys:

| Distance | Time per query |
| --- | --- |
| 1 | about 62 ms |
| 2 | roughly double |

Fuzzy search is a full scan, so keep it for interactive use rather than hot paths.

## Serialization flags

`serialize(container, false)` writes payloads verbatim.
`serialize(container, true)` compresses each payload when that makes it smaller and
records which codec was used.

Measured on 50,000 entries of repeated text:

| Mode | Snapshot size | Write | Read |
| --- | --- | --- | --- |
| verbatim | 6,044,462 bytes | 36.5 ms | 47.9 ms |
| compressed payloads | 4,726,432 bytes | 130.1 ms | 30.3 ms |

Compression buys 22 percent smaller snapshots and costs about 3.5x write time. Reads are
faster because less data is copied from the buffer.

## Next

`04-recipes.md` collects the patterns that come up most often.

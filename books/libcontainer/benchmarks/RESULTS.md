# Measured results

**By Anthony Matarazzo (c) 2026** - MIT License

Everything below was produced by `make -s bench` in this directory on the machine that
built the library: g++ 12.2, `-std=c++17 -O2`, Linux x86-64, single thread, no warm-up
beyond the first allocation.

Reproduce with:

```bash
make -s bench
```

## Compression: ratio and throughput (1 MiB payloads)

| Payload | Codec | Packed bytes | Ratio | Encode MB/s | Decode MB/s |
| --- | --- | --- | --- | --- | --- |
| prose | none | 1,048,576 | 1.000 | 697.6 | 699.8 |
| prose | rle | 1,048,576 | 1.000 | 252.8 | 249.9 |
| prose | lz77 | 233,120 | 0.222 | 273.3 | 455.3 |
| prose | auto | 233,121 | 0.222 | 98.0 | 1010.6 |
| runs | none | 1,048,576 | 1.000 | 39510.1 | 39108.3 |
| runs | rle | 15,729 | 0.015 | 1563.1 | 428.0 |
| runs | lz77 | 233,040 | 0.222 | 132.3 | 998.3 |
| runs | auto | 15,730 | 0.015 | 113.8 | 7229.7 |
| random | none | 1,048,576 | 1.000 | 16843.5 | 25826.4 |
| random | rle | 1,056,902 | 1.008 | 363.7 | 369.6 |
| random | lz77 | 2,096,630 | 2.000 | 0.2 | 694.2 |
| random | auto | 1,048,577 | 1.000 | 0.2 | 799.9 |

Reading: LZ77 is the only codec that pays off on prose (4.5x smaller). RLE is
spectacular on runs (67x smaller) and worthless on prose. LZ77 doubles random data,
which is exactly why `kAuto` exists: it chose `none` for the random payload and paid one
header byte for that safety.

## Container operations (200,000 entries)

| Operation | Calls | Total ms | Per call |
| --- | --- | --- | --- |
| insert | 200,000 | 138.98 | 0.695 us |
| point lookup | 200,000 | 24.73 | 0.124 us |
| prefix search returning all matches | 2,000 | 38,197.37 | 19,098.687 us |
| fuzzy search, distance 1 | 200 | 12,456.36 | 62,281.823 us |

Reading: point lookups are fast because they hit a `std::map` and the trie only guards
prefix queries. The prefix figure looks alarming but it is 200,000 matches per call plus
a sort of all of them, so it is the cost of materialising the whole result set, not the
cost of the trie walk. Fuzzy search is a full scan by construction and should not sit in
a hot path.

## Snapshots (50,000 entries, repeated text payloads)

| Mode | Raw bytes | Snapshot | Ratio | Write ms | Read ms |
| --- | --- | --- | --- | --- | --- |
| verbatim | 5,394,450 | 6,044,462 | 1.120 | 36.5 | 47.9 |
| compressed payloads | 5,394,450 | 4,726,432 | 0.876 | 130.1 | 30.3 |

Reading: per-payload compression cuts the snapshot by 22 percent and makes loading faster
because less data is copied, at the cost of roughly 3.5x the write time. Both modes
restored all 50,000 entries.

## Usefulness rating

| Algorithm | Rating | Verdict |
| --- | --- | --- |
| Container with secondary indices | 5/5 | Sub-microsecond point lookups and O(bucket) queries, with index maintenance handled on write |
| LZ77 codec | 4/5 | 4.5x on prose at usable speed; expands random data, so wrap it in `kAuto` |
| RLE codec | 3/5 | 67x on runs, 1.0x on prose. Keep it for the run-heavy payloads only |
| Snapshot serialization | 4/5 | Checksummed, self describing, 22 percent smaller with compression |
| Prefix trie | 3/5 | Fast walk, but returning and sorting every match dominates at scale |
| Fuzzy search | 2/5 | 62 ms per query at 200,000 keys. Good for interactive use, unusable in a loop |

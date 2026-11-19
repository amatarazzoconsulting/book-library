# Universal Container Object (libContainer) - companion source

**By Anthony Matarazzo (c) 2026** - MIT License

This directory holds the translated book, the source for every algorithm it describes,
tests that prove the code works, and measurements that say whether each algorithm is
worth using.

## What is in here

| Path | Contents |
| --- | --- |
| `universal-container-object.md` | The full book, translated to Markdown with a linked table of contents |
| `src/` | One file pair per algorithm, plus the demo and teaching programs |
| `tests/` | One test program per algorithm |
| `benchmarks/` | One benchmark per algorithm, plus `RESULTS.md` with the measured tables |
| `learn/` | Four lessons that teach the API from first program to recipes |

## Algorithms

| Algorithm | Source | What it does |
| --- | --- | --- |
| Container with secondary indices | `src/container.hpp`, `src/container.cpp` | Records, extractor based indices, visitor queries, dirty tracking |
| Prefix and fuzzy search | `src/search_index.hpp`, `src/search_index.cpp` | Prefix trie with ordered results, bounded Levenshtein search |
| RLE and LZ77 codecs | `src/compression.hpp`, `src/compression.cpp` | Two codecs plus an automatic chooser with a self describing header |
| Snapshot serialization | `src/serialization.hpp`, `src/serialization.cpp` | Checksummed binary snapshots with optional payload compression |

## Build and run

```bash
make -s all      # builds every program into bin/
./bin/demo       # short tour of all four algorithms
./bin/teaching   # every option and parameter, step by step
make -s test     # all assertions, exits non-zero on failure
make -s bench    # measured tables, same numbers as RESULTS.md
```

Nothing outside the C++17 standard library is required.

## Learning path

1. `learn/01-concepts.md` - how the store, the indices and the visitors fit together.
2. `learn/02-first-program.md` - a working program in five minutes.
3. `learn/03-all-options.md` - every config field, codec and search flag.
4. `learn/04-recipes.md` - patterns for compound keys, type-ahead and persistence.

## Measured results

Full tables and the hardware note are in `benchmarks/RESULTS.md`.

| Metric | Result |
| --- | --- |
| LZ77 ratio on prose | 0.222 (4.5x smaller) at 273 MB/s encode |
| RLE ratio on runs | 0.015 (67x smaller) at 1563 MB/s encode |
| Automatic codec on random data | 1.000 ratio, never expands beyond one header byte |
| Point lookup | 0.124 us per call at 200,000 entries |
| Fuzzy search, distance 1 | 62 ms per call at 200,000 entries |
| Snapshot with compressed payloads | 22 percent smaller, 3.5x slower to write, faster to read |

## Usefulness rating

| Algorithm | Rating | Verdict |
| --- | --- | --- |
| Container with secondary indices | 5/5 | Fast lookups, correct index maintenance on update and erase |
| LZ77 codec | 4/5 | Strong on text; pair it with `kAuto` so random data is left alone |
| RLE codec | 3/5 | Excellent for run-heavy payloads, no gain on prose |
| Snapshot serialization | 4/5 | Checksummed and self describing; compression trades write time for size |
| Prefix trie | 3/5 | Good walk, but materialising every match dominates at 200,000 keys |
| Fuzzy search | 2/5 | 62 ms per query; interactive only |

## License

MIT License, copyright (c) Anthony Matarazzo 2026. The full text is at the top of the
book Markdown file.

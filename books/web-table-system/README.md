# Web Table System - companion source

**By Anthony Matarazzo (c) 2026** - MIT License

Text-defined tables with typed columns, a primary key, secondary indexes and a small
query language. This directory holds the translated book, the source for each algorithm,
tests and measured benchmarks.

## Contents

| Path | Contents |
| --- | --- |
| `web-table-system.md` | The full book with a linked table of contents |
| `src/` | One file pair per algorithm, plus `demo.cpp` and `teaching.cpp` |
| `tests/` | One test program per algorithm |
| `benchmarks/` | The benchmark and `RESULTS.md` with measured numbers |
| `learn/` | Four lessons from concepts to recipes |

## Algorithms

| Algorithm | Source | What it does |
| --- | --- | --- |
| Schema parser | `src/schema.hpp`, `src/schema.cpp` | Parses Table, Columns, Key, Separator, Width, Precision and Rounding tokens with line-numbered errors |
| Typed table store | `src/table.hpp`, `src/table.cpp` | Enforces column types, maintains the key map and secondary indexes, counts scans |
| Query engine | `src/query.hpp`, `src/query.cpp` | Predicate parsing, evaluation and bounded edit distance matching |

## Build and run

```bash
make -s all
./bin/demo
./bin/teaching
make -s test
make -s bench
```

## Learning path

1. `learn/01-concepts.md` - the format, the parser and why types are enforced on write.
2. `learn/02-first-program.md` - build a table and query it.
3. `learn/03-all-options.md` - every token and operator with measured costs.
4. `learn/04-recipes.md` - separators, error reporting, index choice and fuzzy fallback.

## Measured results

Full tables in `benchmarks/RESULTS.md`, measured at 200,000 rows.

| Metric | Result |
| --- | --- |
| Load throughput | 430,844 rows per second |
| Key lookup | 0.422 us per call |
| Indexed lookup on Category | 27.3 us per call |
| Full text scan | 12.5 ms per call |
| Fuzzy match, distance 1 | 41.6 ms per call |

## Usefulness rating

| Algorithm | Rating | Verdict |
| --- | --- | --- |
| Text schema parser | 5/5 | Self describing and precise about errors |
| Typed row store with key map | 5/5 | Fast loads, sub-microsecond lookups |
| Secondary index | 4/5 | 460x faster than the equivalent scan |
| Predicate evaluator | 4/5 | Safe on bad input, covers the common operators |
| Fuzzy matching | 2/5 | Typo tolerance only; linear in rows |

## License

MIT License, copyright (c) Anthony Matarazzo 2026.

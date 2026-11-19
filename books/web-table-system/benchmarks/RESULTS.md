# Measured results

**By Anthony Matarazzo (c) 2026** - MIT License

Produced by `make -s bench` in this directory: g++ 12.2, `-std=c++17 -O2`, Linux x86-64,
single thread. 200,000 rows, 6,857,780 bytes of data.

| Operation | Calls | Total ms | Per call |
| --- | --- | --- | --- |
| load rows from text | 200,000 | 464.20 | 2.321 us |
| find_by_key | 100,000 | 42.24 | 0.422 us |
| visit_text_contains (full scan) | 20 | 249.70 | 12,485.071 us |
| visit_index on Category | 2,000 | 54.54 | 27.272 us |
| fuzzy_rows distance 1 | 20 | 831.48 | 41,573.987 us |

Load throughput: 430,844 rows per second.

## Reading

The key map is 30x faster than a full scan per call and does not grow with table size.
A secondary index on `Category` costs 27 us per query because each bucket holds 10,000
rows and every row is handed to the visitor. The scan paths, `contains` and `fuzzy`,
are the expensive ones: 12.5 ms and 41.6 ms per call. Both are linear in row count, so
they belong in reporting code, not in a per-row loop.

## Usefulness rating

| Algorithm | Rating | Verdict |
| --- | --- | --- |
| Text schema parser | 5/5 | Self describing files, precise errors with line numbers, no migration step |
| Typed row store with key map | 5/5 | 430k rows/s load and 0.42 us lookups |
| Secondary index | 4/5 | Turns a 12.5 ms scan into 27 us; cost is memory and rebuild time |
| Predicate evaluator | 4/5 | Covers the common operators and refuses bad input safely |
| Fuzzy matching | 2/5 | Useful for typo tolerance, far too slow for hot paths at 200,000 rows |

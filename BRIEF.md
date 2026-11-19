# Worker brief: finish one book

**By Anthony Matarazzo (c) 2026** - MIT License

You are completing ONE book directory under `library/books/<slug>/`. Read this whole
file first. Work only inside that directory plus `library/toolchain/` when the book is a
lowl book.

## What must exist when you are done

| Path | Requirement |
| --- | --- |
| `<slug>/<title>.md` | Already there. The translated book. Never modify it |
| `src/<algo>.hpp` + `src/<algo>.cpp` | One file pair per algorithm, 2 to 4 algorithms, C++17, STL only, deterministic, first line `// MIT License - (c) Anthony Matarazzo 2026` |
| `src/demo.cpp` | Short program exercising every algorithm with labeled output |
| `src/teaching.cpp` | Guided program demonstrating EVERY option and parameter, explaining as it prints |
| `tests/test_<algo>.cpp` | One per algorithm, asserts normal and edge cases, prints PASS lines, exits non-zero on failure |
| `benchmarks/bench_<algo>.cpp` | One per algorithm, real `std::chrono` measurements, prints a metrics table |
| `benchmarks/RESULTS.md` | The measured table, hardware note, and a usefulness rating per algorithm (1-5 stars plus a one-line verdict) |
| `learn/01-concepts.md` `02-first-program.md` `03-all-options.md` `04-recipes.md` | Teaching documents with runnable code, exact commands, expected output |
| `README.md` | Overview, algorithm inventory table, build/run commands, learning path, measured summary table, ratings |

Partial work may already exist in the directory. Inspect it, keep what is good, finish
what is missing. Do not restart from scratch.

## Build contract

`make -s all` must succeed with ZERO warnings under `-std=c++17 -O2 -Wall -Wextra -Isrc`.
The `Makefile` is shared and must not be edited. Then:

```sh
make -s test     # every test passes
make -s bench    # prints the metric tables
```

Paste the ACTUAL numbers from `make -s bench` into `benchmarks/RESULTS.md` and the
README summary table. Never invent a number. Ratings must follow from the measurements.

## lowl books

If your book is a lowl book, you must ALSO ship lowl source compiled and run by the
author's compiler:

1. Read `library/toolchain/LOWL_SYNTAX.md` first. It lists the verified subset and the
   constructs that are broken in v2.1.0 (for loops, array indexing, struct field
   assignment, string equality). Do not use the broken constructs.
2. Add `src/<algo>.lowl` for at least two algorithms of the book, each starting with
   `// MIT License - (c) Anthony Matarazzo 2026`.
3. Build and run every one:
   ```sh
   cd /app/conversations/6afdc8ba6d82622701050207/library/toolchain/lowl
   ./build.sh ../../books/<slug>/src/<algo>.lowl --run
   ```
   The file must assemble, link and run with the expected output. Fix or simplify until
   it does.
4. Quote the real program output in `learn/03-all-options.md` or a dedicated
   `learn/05-lowl-walkthrough.md`, and state in the README which algorithms have a lowl
   implementation and which are C++ only, with the reason.

## Style

No em dashes anywhere, use plain hyphens. Keep total code under about 2500 lines.
Comments explain why, not what. Programs must be deterministic.

## Report back

State: files written per directory, `make -s test` result, the benchmark numbers you
measured, and for lowl books the exact `./build.sh ... --run` output.

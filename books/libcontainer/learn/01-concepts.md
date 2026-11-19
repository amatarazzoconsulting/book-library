# Lesson 1: How the container is put together

**By Anthony Matarazzo (c) 2026** - MIT License

The book describes a container that keeps three things in step: the records themselves,
secondary indices over those records, and a search index over the keys. Everything else
in this directory is built on that idea.

## The four source files

| File | What it owns |
| --- | --- |
| `src/container.hpp` / `.cpp` | Records, secondary indices, the visitor pattern |
| `src/search_index.hpp` / `.cpp` | Prefix trie and bounded edit distance search |
| `src/compression.hpp` / `.cpp` | RLE, LZ77 and the automatic codec chooser |
| `src/serialization.hpp` / `.cpp` | Snapshot format with per-entry checksums |

Each file is standalone. `container.cpp` depends on `search_index.hpp`, and
`serialization.cpp` depends on `compression.hpp`, but none of them depend on anything
outside the C++ standard library.

## Why indices are separate from the store

A container that only stores bytes is a map. The value comes from answering questions
like "which records are in New York" without walking every record. A secondary index is
an extractor function plus a table from value to keys:

```cpp
db.add_secondary_index("city", [](const libcontainer::Entry& e) {
    return std::vector<std::string>{city_of(e.payload)};
});
```

The container owns the bookkeeping. When you call `update` or `erase`, it removes the
old index entries first and inserts the new ones after, so the index never goes stale.
`tests/test_container.cpp` asserts exactly that behaviour.

## Why the visitor pattern

Returning a vector of results forces a copy of every match. A visitor hands each match to
your function while the container still owns the memory:

```cpp
db.visit_by_index("city", "New York",
                  [](const std::string& key, const libcontainer::Entry& entry) {
                      std::cout << key << " -> " << entry.payload << "\n";
                  });
```

Every query shape in the container uses the same signature, so you can swap
`visit_all`, `visit_prefix`, `visit_by_index` and `visit_fuzzy` without rewriting the
body of the callback.

## Next

Continue with `02-first-program.md` to build and run something in five minutes.

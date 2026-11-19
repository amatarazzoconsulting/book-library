# Lesson 4: Recipes

**By Anthony Matarazzo (c) 2026** - MIT License

## Recipe 1: Compound secondary keys

Extractors return a vector, so one record can live in several buckets. A record with
tags can be found by any of them:

```cpp
db.add_secondary_index("tag", [](const libcontainer::Entry& e) {
    return split(e.payload, ',');   // one entry per tag
});
```

## Recipe 2: Cheap case-insensitive keys

Normalize in the extractor rather than in the query:

```cpp
db.add_secondary_index("name_lc", [](const libcontainer::Entry& e) {
    return std::vector<std::string>{lower(field(e.payload, 0))};
});
```

Now `visit_by_index("name_lc", lower(user_input))` matches regardless of case, and the
stored record keeps its original spelling.

## Recipe 3: Type-ahead search

Prefix search is the right tool while the user is typing:

```cpp
db.visit_prefix(buffer, [&](const std::string& key, const libcontainer::Entry&) {
    std::cout << key << "\n";
});
```

At 200,000 keys a single `prefix("key-1")` call that returns every match measured about
19 ms, mostly in sorting. When you only need the first few results, keep the callback
cheap and stop early with your own counter.

## Recipe 4: Tolerating typos

Fuzzy search with distance 1 or 2 covers most typos. Run it only when prefix search
returns nothing:

```cpp
if (db.search().prefix(query).empty()) {
    db.visit_fuzzy(query, 2, print_hit);
}
```

## Recipe 5: Persist and reload

```cpp
const auto snapshot = libcontainer::serialize(db, true);
libcontainer::Container restored;
libcontainer::deserialize(snapshot, &restored);
```

`deserialize` verifies the FNV-1a checksum of every payload and throws
`std::runtime_error` on a mismatch, so a truncated file fails loudly instead of loading
half a database. The test suite covers the bad-magic and truncated-run cases.

## Recipe 6: Deciding between the codecs

Measure your own payload shape instead of guessing. The benchmark harness is the
template:

```bash
make -s bench
./bin/bench_compression
```

On this machine LZ77 wins on prose at 0.222 and RLE wins on runs at 0.015, while RLE
does nothing for prose. If your data is unknown, use `kAuto`.

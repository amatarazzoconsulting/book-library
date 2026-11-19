# Lesson 4: Recipes

**By Anthony Matarazzo (c) 2026** - MIT License

## Recipe 1: Data that contains commas

Set the separator and keep commas inside the text:

```text
Separator ;
```
```text
1;Smith, John;42.00
```

## Recipe 2: Reporting the bad row instead of dying

```cpp
std::string error;
const std::size_t loaded = table.load(text, &error);
if (!error.empty()) std::cout << loaded << " rows loaded, stopped: " << error << "\n";
```

`load` returns how many rows were accepted before the first failure and leaves the
message with the line number.

## Recipe 3: Choosing an index

Build an index for the columns you filter on repeatedly, and leave the rest alone:

```cpp
table.build_index("Category");
table.visit_index("Category", "tools", print_row);
```

Check `scan_rows()` and `index_hits()` around a query to confirm the index is doing the
work.

## Recipe 4: Type-ahead with fuzzy fallback

```cpp
if (webtable::fuzzy_rows(table, "Name", typed, 2).empty()) {
    std::cout << "no close matches\n";
}
```

Fuzzy search is a full scan at 41 ms per call on 200,000 rows, so run it on a keypress
release, never inside a per-row loop.

## Recipe 5: Numeric range filters

```cpp
const auto predicate = webtable::parse_predicate("Score >= 90", schema);
table.visit_all([&](const webtable::Row& row) {
    if (webtable::matches(schema, row, predicate)) print_row(row);
});
```

Range queries have no index in this implementation, so they scan. That is honest: at
200,000 rows a scan is 12.5 ms, which is fine for reporting and not fine for a live
search box.

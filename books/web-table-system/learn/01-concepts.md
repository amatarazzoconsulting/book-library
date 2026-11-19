# Lesson 1: The table format and why it exists

**By Anthony Matarazzo (c) 2026** - MIT License

WebTableSystem keeps data in plain text files that describe themselves. A schema file
names the table, lists the columns with their types and options, and marks the primary
key. A data section follows the same order. Nothing else is required: no server, no
binary format, no migration step.

## The three source files

| File | Responsibility |
| --- | --- |
| `src/schema.hpp` / `.cpp` | Turns the text schema into typed `Column` records |
| `src/table.hpp` / `.cpp` | Stores rows, enforces types, keeps key and secondary indexes |
| `src/query.hpp` / `.cpp` | Parses predicates, evaluates them, runs fuzzy matching |

## What the schema carries

```text
Table Product
Columns
ProductID Integer Key
Name Text Width=40
Price Decimal Precision=2 Rounding=Nearest
***
Separator ;
```

`Table` names the table. `Columns` starts the column block and `***` ends it. Each column
line is `Name Type` followed by optional tokens: `Key`, `Unique`, `Width=`, `Precision=`,
`Rounding=`. `Separator` changes the field delimiter for the data section, which matters
when your text contains commas.

The parser rejects what it cannot honour: two key columns, unknown tokens, a missing
table name. Those failures come back as a message with the line number, not an
exception, because loading a schema is usually a user-facing operation.

## Types are enforced on write

`Integer` and `Decimal` columns are converted on insert. A value that does not parse is
rejected with the column name in the error, so bad rows never reach the store. Text
columns keep whatever you give them.

## Indexes are explicit

A table always has a primary key map. Secondary indexes are built on demand with
`build_index("Category")`. Until you build one, `visit_index` falls back to a full scan,
which the counters make visible: `index_hits()` versus `scan_rows()`.

## Next

`02-first-program.md` builds and runs a table in five minutes.

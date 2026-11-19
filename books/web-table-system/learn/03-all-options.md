# Lesson 3: Every option

**By Anthony Matarazzo (c) 2026** - MIT License

Run `./bin/teaching` to see each of these printed with live numbers.

## Schema tokens

| Token | Where | Meaning |
| --- | --- | --- |
| `Table <name>` | header | Names the table. Required |
| `Columns` | header | Starts the column block. Required |
| `***` | header | Ends the column block |
| `Separator <c>` | header | Field delimiter for the data section, default `,` |
| `Index <column>` | header | Records an index hint you can build later |
| `Key` | column | Marks the primary key. Only one column may carry it |
| `Unique` | column | Declares uniqueness. Enforced by the key map for key columns |
| `Width=<n>` | column | Display width hint, carried into the schema for renderers |
| `Precision=<n>` | column | Decimal places used when formatting |
| `Rounding=<Nearest|Up|Down>` | column | Rounding hint carried with the column |
| `#` at line start | anywhere | Comment, ignored |

## Predicate operators

`=`, `==`, `!=`, `<>`, `<`, `>`, `<=`, `>=`, `contains`.

Text columns support `=`, `!=` and `contains`; `contains` is case insensitive. Numeric
columns support the full comparison set. A predicate that names an unknown column or an
unknown operator comes back with `valid == false` instead of throwing, so a query string
from a form can never crash the loader.

## Index behaviour

`visit_index` uses a built index when one exists and falls back to a scan otherwise.
Measured on 200,000 rows:

| Access path | Per call |
| --- | --- |
| `find_by_key` | 0.42 us |
| `visit_index` after `build_index` | 27 us for a bucket of 10,000 rows |
| `visit_text_contains` full scan | 12.5 ms |
| `fuzzy_rows` distance 1 | 41.6 ms |

The difference between 0.42 us and 12.5 ms is the whole argument for keys and indexes.

## Fuzzy matching

`edit_distance` is a bounded Levenshtein. The bound is not cosmetic: once a row of the
matrix exceeds it, the candidate is abandoned, which is what makes distance 1 usable and
distance 3 slow.

## Next

`04-recipes.md` has the patterns that come up in real schemas.

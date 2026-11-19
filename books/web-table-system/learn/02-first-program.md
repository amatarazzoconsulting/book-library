# Lesson 2: A working table in five minutes

**By Anthony Matarazzo (c) 2026** - MIT License

## Build and run

```bash
cd library/books/web-table-system
make -s all
./bin/demo
./bin/teaching
make -s test
```

## Your first program

Save as `first.cpp` in the book directory:

```cpp
#include <iostream>
#include "schema.hpp"
#include "table.hpp"

int main() {
    webtable::Schema schema;
    std::string error;
    if (!schema.parse("Table T\nColumns\nID Integer Key\nName Text\n***\n", &error)) {
        std::cout << "schema: " << error << "\n";
        return 1;
    }

    webtable::Table table(schema);
    table.load("1,Ada\n2,Grace\n", &error);

    table.visit_all([](const webtable::Row& row) {
        std::cout << row.values[0].to_string() << " -> " << row.values[1].to_string() << "\n";
    });
    return 0;
}
```

Build and run:

```bash
g++ -std=c++17 -O2 -Isrc first.cpp src/schema.cpp src/table.cpp src/query.cpp -o first
./first
```

Expected output:

```text
1 -> Ada
2 -> Grace
```

## Query it

```cpp
table.build_index("Name");
table.visit_index("Name", "Ada", [](const webtable::Row& row) {
    std::cout << "found " << row.values[1].to_string() << "\n";
});
```

Expected output:

```text
found Ada
```

## What just happened

`load` split each line on the schema separator, converted the `ID` cell to an integer,
registered it in the key map, and appended the row. `build_index` then walked the rows
once and bucketed them by name, so the lookup touched one bucket instead of the whole
table.

## Next

`03-all-options.md` covers every token, every operator and the index counters.

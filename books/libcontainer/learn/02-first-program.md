# Lesson 2: Your first container in five minutes

**By Anthony Matarazzo (c) 2026** - MIT License

## Build everything

```bash
cd library/books/libcontainer
make -s all      # compiles src/, tests/ and benchmarks/ into bin/
./bin/demo       # the guided tour
./bin/teaching   # every option, step by step
```

`make test` runs the assertions and `make bench` prints the measured tables.

## The smallest useful program

Save this as `first.cpp` in the book directory and build it with the same flags the
Makefile uses:

```cpp
#include <iostream>
#include "container.hpp"

int main() {
    libcontainer::Container db;
    db.insert("alice", "Alice Johnson|New York|30");
    db.insert("bob", "Bob Smith|Los Angeles|25");

    db.visit_all([](const std::string& key, const libcontainer::Entry& entry) {
        std::cout << key << " -> " << entry.payload << "\n";
    });
    return 0;
}
```

```bash
g++ -std=c++17 -O2 -Isrc first.cpp src/container.cpp src/search_index.cpp -o first
./first
```

Expected output:

```text
alice -> Alice Johnson|New York|30
bob -> Bob Smith|Los Angeles|25
```

## Adding a query

```cpp
db.add_secondary_index("city", [](const libcontainer::Entry& e) {
    const std::size_t a = e.payload.find('|');
    const std::size_t b = e.payload.find('|', a + 1);
    return std::vector<std::string>{e.payload.substr(a + 1, b - a - 1)};
});
db.visit_by_index("city", "New York",
                  [](const std::string& key, const libcontainer::Entry&) {
                      std::cout << "in New York: " << key << "\n";
                  });
```

Expected output:

```text
in New York: alice
```

## What just happened

`insert` wrote the record, registered the key in the prefix trie, and evaluated every
registered extractor. The extractor returned `New York`, so the container appended
`alice` to that bucket. The query then walked one bucket instead of every record.

## Next

`03-all-options.md` covers every knob: config fields, codecs, search distances and the
snapshot flags.

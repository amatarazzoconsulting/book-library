// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>

#include "schema.hpp"
#include "table.hpp"

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}
const char* kSchema = "Table T\nColumns\nID Integer Key\nName Text\nScore Decimal\n***\n";
}  // namespace

int main() {
    webtable::Schema schema;
    std::string err;
    schema.parse(kSchema, &err);
    webtable::Table t(schema);
    check(t.add_row({"1", "Ada", "10.5"}, &err), "adds a typed row");
    check(t.add_row({"2", "Grace", "20"}, &err), "adds a second row");
    check(!t.add_row({"3", "Alan", "abc"}, &err), "rejects a non numeric decimal");
    check(!t.add_row({"1", "Dup", "1.0"}, &err), "rejects a duplicate key");
    check(!t.add_row({"4", "Short"}, &err), "rejects a row with too few fields");
    check(t.size() == 2, "size counts only accepted rows");

    const auto* row = t.find_by_key("2");
    check(row != nullptr && row->values[1].text == "Grace", "key lookup returns the row");
    check(t.find_by_key("99") == nullptr, "key lookup returns null for a missing key");

    check(t.build_index("Name"), "build_index accepts a real column");
    check(!t.build_index("Nope"), "build_index rejects an unknown column");
    std::size_t hits = 0;
    t.reset_counters();
    t.visit_index("Name", "Ada", [&](const webtable::Row&) { ++hits; });
    check(hits == 1 && t.index_hits() == 1 && t.scan_rows() == 0, "index hit avoids a scan");
    hits = 0;
    t.reset_counters();
    t.visit_text_contains("Name", "rac", [&](const webtable::Row&) { ++hits; });
    check(hits == 1 && t.scan_rows() == 2, "contains scan visits every row");

    check(t.load("5,Alan,5.0\n6,Linus,7.5\n", &err) == 2, "load adds rows from the separator");
    check(t.size() == 4, "size grew after load");

    std::cout << (failures == 0 ? "all table tests passed\n" : "table failures\n");
    return failures == 0 ? 0 : 1;
}

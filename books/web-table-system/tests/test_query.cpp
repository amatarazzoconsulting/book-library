// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>

#include "query.hpp"
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
    t.add_row({"1", "Widget A", "10.5"}, &err);
    t.add_row({"2", "Widget B", "99.0"}, &err);
    t.add_row({"3", "Gadget", "50.0"}, &err);

    const auto gt = webtable::parse_predicate("Score > 40", schema);
    check(gt.valid && gt.op == webtable::Op::kGreater, "parses a greater than predicate");
    std::size_t hits = 0;
    t.visit_all([&](const webtable::Row& r) { if (webtable::matches(schema, r, gt)) ++hits; });
    check(hits == 2, "greater than matches two rows");

    const auto eq = webtable::parse_predicate("Name = Gadget", schema);
    hits = 0;
    t.visit_all([&](const webtable::Row& r) { if (webtable::matches(schema, r, eq)) ++hits; });
    check(hits == 1, "text equality matches one row");

    const auto contains = webtable::parse_predicate("Name contains widget", schema);
    hits = 0;
    t.visit_all([&](const webtable::Row& r) { if (webtable::matches(schema, r, contains)) ++hits; });
    check(hits == 2, "contains is case insensitive");

    check(!webtable::parse_predicate("Nope = 1", schema).valid, "rejects an unknown column");
    check(!webtable::parse_predicate("Score ~ 1", schema).valid, "rejects an unknown operator");

    check(webtable::edit_distance("kitten", "sitting", 5) == 3, "edit distance kitten to sitting");
    check(webtable::edit_distance("same", "same", 0) == 0, "edit distance identical");
    check(webtable::edit_distance("abc", "xyz", 1) == 2, "edit distance bound respected");

    const auto fuzzy = webtable::fuzzy_rows(t, "Name", "widgit a", 3);
    check(!fuzzy.empty() && fuzzy[0] == 0, "fuzzy rows finds the closest name first");

    std::cout << (failures == 0 ? "all query tests passed\n" : "query failures\n");
    return failures == 0 ? 0 : 1;
}

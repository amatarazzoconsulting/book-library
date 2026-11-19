// MIT License - (c) Anthony Matarazzo 2026
// Teaching program: every schema token, every query operator, every index mode.
#include <iomanip>
#include <iostream>
#include <string>

#include "query.hpp"
#include "schema.hpp"
#include "table.hpp"

namespace {

void section(const char* title) { std::cout << "\n=== " << title << " ===\n"; }

void dump(const webtable::Schema& schema, const webtable::Row& row) {
    for (std::size_t i = 0; i < row.values.size(); ++i) {
        std::cout << std::setw(10) << std::left
                  << (schema.columns[i].name + "=" + row.values[i].to_string());
    }
    std::cout << "\n";
}

}  // namespace

int main() {
    section("Step 1: schema tokens, one line at a time");
    const char* text = R"(# every token the format supports
Table Order
Columns
OrderID Integer Key
Customer Text Width=32
Total Decimal Precision=2 Rounding=Nearest
Status Text Unique
***
Separator ;
)";
    webtable::Schema schema;
    std::string error;
    const bool ok = schema.parse(text, &error);
    std::cout << "  parse ok = " << (ok ? "true" : "false") << (ok ? "" : (" (" + error + ")").c_str()) << "\n";
    std::cout << "  table name = " << schema.name << "\n";
    std::cout << "  separator  = '" << schema.separator << "'\n";
    for (const auto& c : schema.columns) {
        std::cout << "  column " << std::setw(10) << std::left << c.name
                  << " type=" << webtable::type_name(c.type)
                  << " key=" << (c.key ? "yes" : "no")
                  << " unique=" << (c.unique ? "yes" : "no")
                  << " width=" << c.width
                  << " precision=" << c.precision
                  << " rounding=" << (c.rounding.empty() ? "-" : c.rounding) << "\n";
    }

    section("Step 2: reject a schema with two key columns");
    webtable::Schema bad;
    const bool bad_ok = bad.parse("Table T\nColumns\nA Integer Key\nB Integer Key\n***\n", &error);
    std::cout << "  parse ok = " << (bad_ok ? "true" : "false") << ", error = " << error << "\n";

    section("Step 3: load rows through the separator");
    webtable::Table table(schema);
    const std::size_t loaded = table.load("1;Ada;10.50;new\n2;Grace;99.99;paid\n3;Alan;5.00;new\n", &error);
    std::cout << "  loaded " << loaded << " rows, table size " << table.size() << "\n";
    table.visit_all([&](const webtable::Row& row) { dump(schema, row); });

    section("Step 4: type validation and duplicate keys are reported");
    std::string row_error;
    const bool bad_row = table.add_row({"4", "Linus", "not-a-number", "new"}, &row_error);
    std::cout << "  add_row(bad decimal) = " << (bad_row ? "true" : "false") << " error = " << row_error << "\n";
    row_error.clear();
    const bool dup = table.add_row({"1", "Ada", "10.50", "new"}, &row_error);
    std::cout << "  add_row(duplicate key) = " << (dup ? "true" : "false") << " error = " << row_error << "\n";

    section("Step 5: every predicate operator");
    for (const char* expression : {"Total > 10", "Total >= 99.99", "Total < 20", "Total <= 10.5",
                                   "Status = new", "Status != new", "Customer contains a"}) {
        const auto p = webtable::parse_predicate(expression, schema);
        std::size_t hits = 0;
        table.visit_all([&](const webtable::Row& row) {
            if (webtable::matches(schema, row, p)) ++hits;
        });
        std::cout << "  " << std::setw(22) << std::left << expression << " -> " << hits << " row(s)"
                  << (p.valid ? "" : "  [invalid predicate]") << "\n";
    }

    section("Step 6: index on versus off, with the scan counters");
    table.reset_counters();
    std::size_t hits = 0;
    table.visit_index("Status", "new", [&](const webtable::Row&) { ++hits; });
    std::cout << "  before build_index: hits=" << hits << " index_hits=" << table.index_hits()
              << " rows_scanned=" << table.scan_rows() << "\n";
    table.build_index("Status");
    table.reset_counters();
    hits = 0;
    table.visit_index("Status", "new", [&](const webtable::Row&) { ++hits; });
    std::cout << "  after  build_index: hits=" << hits << " index_hits=" << table.index_hits()
              << " rows_scanned=" << table.scan_rows() << "\n";

    section("Step 7: key lookup versus full scan");
    table.reset_counters();
    const auto* row = table.find_by_key("2");
    std::cout << "  find_by_key(2) = " << (row ? row->values[1].to_string() : "null")
              << ", index_hits=" << table.index_hits() << ", rows_scanned=" << table.scan_rows() << "\n";

    section("Step 8: edit distance thresholds");
    for (std::size_t d = 0; d <= 3; ++d) {
        std::cout << "  fuzzy('paid', distance " << d << ") -> "
                  << webtable::fuzzy_rows(table, "Status", "paid", d).size() << " row(s)\n";
    }
    std::cout << "\n  distance(paid, paid)  = " << webtable::edit_distance("paid", "paid", 5) << "\n"
              << "  distance(paid, paidd) = " << webtable::edit_distance("paid", "paidd", 5) << "\n"
              << "  distance(kitten, sitting) = " << webtable::edit_distance("kitten", "sitting", 5) << "\n";
    return 0;
}

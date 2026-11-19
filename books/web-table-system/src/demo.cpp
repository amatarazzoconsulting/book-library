// MIT License - (c) Anthony Matarazzo 2026
// Demo: define a schema in the book's text format, load rows, query them.
#include <iomanip>
#include <iostream>
#include <string>

#include "query.hpp"
#include "schema.hpp"
#include "table.hpp"

namespace {

const char* kSchema = R"(# Product catalogue
Table Product
Columns
ProductID Integer Key
Name Text Width=40
Category Text
Price Decimal Precision=2 Rounding=Nearest
Stock Integer
***
)";

const char* kData = R"(1,Widget A,tools,19.99,120
2,Widget B,tools,24.50,8
3,Gadget Pro,gadgets,149.00,42
4,Gadget Mini,gadgets,89.95,0
5,Widget C,tools,12.25,300
)";

void print_row(const webtable::Schema& schema, const webtable::Row& row) {
    for (std::size_t i = 0; i < row.values.size(); ++i) {
        std::cout << std::setw(12) << std::left << row.values[i].to_string();
    }
    std::cout << "\n";
    (void)schema;
}

}  // namespace

int main() {
    webtable::Schema schema;
    std::string error;
    if (!schema.parse(kSchema, &error)) {
        std::cout << "schema error: " << error << "\n";
        return 1;
    }
    std::cout << "table " << schema.name << " with " << schema.columns.size() << " columns, key column index "
              << schema.key_index() << "\n";

    webtable::Table table(schema);
    const std::size_t loaded = table.load(kData, &error);
    std::cout << "loaded " << loaded << " rows";
    if (!error.empty()) std::cout << " (stopped: " << error << ")";
    std::cout << "\n\n";

    std::cout << "== all rows ==\n";
    table.visit_all([&](const webtable::Row& row) { print_row(schema, row); });

    std::cout << "\n== key lookup: 3 ==\n";
    if (const auto* row = table.find_by_key("3")) print_row(schema, *row);

    std::cout << "\n== secondary index on Category ==\n";
    table.build_index("Category");
    table.visit_index("Category", "gadgets", [&](const webtable::Row& row) { print_row(schema, row); });

    std::cout << "\n== predicate: Price > 50 ==\n";
    const auto price = webtable::parse_predicate("Price > 50", schema);
    table.visit_all([&](const webtable::Row& row) {
        if (webtable::matches(schema, row, price)) print_row(schema, row);
    });

    std::cout << "\n== predicate: Name contains widget ==\n";
    table.visit_text_contains("Name", "widget", [&](const webtable::Row& row) { print_row(schema, row); });

    std::cout << "\n== fuzzy: 'gadget' against Category, distance 1 ==\n";
    for (const auto position : webtable::fuzzy_rows(table, "Category", "gadget", 1)) {
        std::cout << "  row " << position << " matches\n";
    }

    std::cout << "\nindex hits: " << table.index_hits() << ", rows scanned: " << table.scan_rows() << "\n";
    return 0;
}

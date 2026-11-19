// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>

#include "schema.hpp"

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}
}  // namespace

int main() {
    webtable::Schema s;
    std::string err;
    const char* good = "Table Product\nColumns\nProductID Integer Key\nName Text Width=40\n"
                       "Price Decimal Precision=2 Rounding=Nearest\n***\nSeparator ;\n";
    check(s.parse(good, &err), "parses a valid schema");
    check(s.name == "Product", "table name parsed");
    check(s.columns.size() == 3, "three columns parsed");
    check(s.columns[0].key && s.columns[0].type == webtable::FieldType::kInteger, "key column typed Integer");
    check(s.columns[1].width == 40, "Width token parsed");
    check(s.columns[2].precision == 2 && s.columns[2].rounding == "Nearest", "precision and rounding parsed");
    check(s.separator == ';', "separator parsed");
    check(s.key_index() == 0, "key index located");
    check(s.column_index("Price") == 2, "column lookup by name");
    check(s.column_index("Missing") == -1, "unknown column returns -1");

    webtable::Schema bad;
    check(!bad.parse("Columns\nA Text\n***\n", &err), "rejects a schema without a Table line");
    check(!bad.parse("Table T\nColumns\nA Integer Key\nB Integer Key\n***\n", &err),
          "rejects two key columns");
    check(!bad.parse("Table T\nColumns\nA Text Bogus=1\n***\n", &err), "rejects an unknown token");
    check(!bad.parse("Table T\n", &err), "rejects a schema without columns");

    std::cout << (failures == 0 ? "all schema tests passed\n" : "schema failures\n");
    return failures == 0 ? 0 : 1;
}

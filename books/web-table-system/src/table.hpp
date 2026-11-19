// MIT License - (c) Anthony Matarazzo 2026
// Row storage with typed fields, a primary key map and secondary indexes.
#pragma once
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "schema.hpp"

namespace webtable {

struct Value {
    FieldType type = FieldType::kText;
    std::string text;
    long long integer = 0;
    double decimal = 0.0;

    static Value from_text(const std::string& s);
    static Value from_integer(long long v);
    static Value from_decimal(double v);
    std::string to_string() const;
    double as_number() const;
};

struct Row {
    std::vector<Value> values;
};

class Table {
public:
    using Visitor = std::function<void(const Row&)>;

    explicit Table(Schema schema) : schema_(std::move(schema)) {}

    const Schema& schema() const { return schema_; }

    // Adds one row from raw cell text. Returns false and fills error on bad input.
    bool add_row(const std::vector<std::string>& cells, std::string* error);
    // Loads a data section: one row per line, fields split on the schema separator.
    std::size_t load(const std::string& data, std::string* error);

    // Builds a hash index over a column so equality lookups skip the scan.
    bool build_index(const std::string& column);

    const Row* find_by_key(const std::string& key) const;
    void visit_all(const Visitor& visitor) const;
    void visit_index(const std::string& column, const std::string& value,
                     const Visitor& visitor) const;
    void visit_text_contains(const std::string& column, const std::string& needle,
                             const Visitor& visitor) const;

    std::size_t size() const { return rows_.size(); }
    std::size_t index_hits() const { return index_hits_; }
    std::size_t scan_rows() const { return scan_rows_; }
    void reset_counters() { index_hits_ = 0; scan_rows_ = 0; }

private:
    Schema schema_;
    std::vector<Row> rows_;
    std::map<std::string, std::size_t> key_map_;
    std::map<std::string, std::map<std::string, std::vector<std::size_t>>> indexes_;
    mutable std::size_t index_hits_ = 0;
    mutable std::size_t scan_rows_ = 0;
};

}  // namespace webtable

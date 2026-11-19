// MIT License - (c) Anthony Matarazzo 2026
// Predicate parsing and evaluation for table queries, plus fuzzy text matching.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include "table.hpp"

namespace webtable {

enum class Op { kEqual, kNotEqual, kLess, kGreater, kLessEqual, kGreaterEqual, kContains };

struct Predicate {
    std::string column;
    Op op = Op::kEqual;
    std::string value;
    bool valid = false;
};

// Accepts forms such as "Price > 100", "Name contains widget", "Category = tools".
Predicate parse_predicate(const std::string& expression, const Schema& schema);

bool matches(const Schema& schema, const Row& row, const Predicate& predicate);

// Bounded Levenshtein distance, returns bound + 1 when exceeded.
std::size_t edit_distance(const std::string& a, const std::string& b, std::size_t bound);

// Rows whose column value is within max_distance edits of the query.
std::vector<std::size_t> fuzzy_rows(const Table& table, const std::string& column,
                                    const std::string& query, std::size_t max_distance);

}  // namespace webtable

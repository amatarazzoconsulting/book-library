// MIT License - (c) Anthony Matarazzo 2026
#include "query.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace webtable {
namespace {

std::string lower(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

}  // namespace

Predicate parse_predicate(const std::string& expression, const Schema& schema) {
    Predicate p;
    std::istringstream in(expression);
    std::string column;
    std::string op;
    std::string rest;
    if (!(in >> column >> op)) return p;
    std::getline(in, rest);
    while (!rest.empty() && std::isspace(static_cast<unsigned char>(rest.front()))) rest.erase(rest.begin());
    while (!rest.empty() && std::isspace(static_cast<unsigned char>(rest.back()))) rest.pop_back();

    if (schema.column_index(column) < 0) return p;
    p.column = column;
    p.value = rest;
    const std::string lower_op = lower(op);
    if (lower_op == "=" || lower_op == "==") p.op = Op::kEqual;
    else if (lower_op == "!=" || lower_op == "<>") p.op = Op::kNotEqual;
    else if (lower_op == "<") p.op = Op::kLess;
    else if (lower_op == ">") p.op = Op::kGreater;
    else if (lower_op == "<=") p.op = Op::kLessEqual;
    else if (lower_op == ">=") p.op = Op::kGreaterEqual;
    else if (lower_op == "contains") p.op = Op::kContains;
    else return p;
    p.valid = true;
    return p;
}

bool matches(const Schema& schema, const Row& row, const Predicate& predicate) {
    if (!predicate.valid) return false;
    const int col = schema.column_index(predicate.column);
    if (col < 0) return false;
    const Value& cell = row.values[static_cast<std::size_t>(col)];
    if (predicate.op == Op::kContains) {
        return lower(cell.to_string()).find(lower(predicate.value)) != std::string::npos;
    }
    if (cell.type == FieldType::kText) {
        const bool equal = cell.text == predicate.value;
        switch (predicate.op) {
            case Op::kEqual: return equal;
            case Op::kNotEqual: return !equal;
            default: return false;
        }
    }
    double lhs = cell.as_number();
    double rhs = 0.0;
    try {
        rhs = std::stod(predicate.value);
    } catch (const std::exception&) {
        return false;
    }
    switch (predicate.op) {
        case Op::kEqual: return lhs == rhs;
        case Op::kNotEqual: return lhs != rhs;
        case Op::kLess: return lhs < rhs;
        case Op::kGreater: return lhs > rhs;
        case Op::kLessEqual: return lhs <= rhs;
        case Op::kGreaterEqual: return lhs >= rhs;
        case Op::kContains: break;
    }
    return false;
}

std::size_t edit_distance(const std::string& a, const std::string& b, std::size_t bound) {
    const std::size_t inf = bound + 1;
    if (a.size() > b.size() + bound || b.size() > a.size() + bound) return inf;
    std::vector<std::size_t> prev(b.size() + 1), cur(b.size() + 1);
    for (std::size_t j = 0; j <= b.size(); ++j) prev[j] = j;
    for (std::size_t i = 1; i <= a.size(); ++i) {
        cur[0] = i;
        std::size_t row_min = cur[0];
        for (std::size_t j = 1; j <= b.size(); ++j) {
            const std::size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
            row_min = std::min(row_min, cur[j]);
        }
        if (row_min > bound) return inf;
        prev.swap(cur);
    }
    return prev[b.size()] > bound ? inf : prev[b.size()];
}

std::vector<std::size_t> fuzzy_rows(const Table& table, const std::string& column,
                                    const std::string& query, std::size_t max_distance) {
    std::vector<std::size_t> hits;
    const int col = table.schema().column_index(column);
    if (col < 0) return hits;
    std::size_t position = 0;
    table.visit_all([&](const Row& row) {
        const auto d = edit_distance(lower(row.values[static_cast<std::size_t>(col)].to_string()),
                                     lower(query), max_distance);
        if (d <= max_distance) hits.push_back(position);
        ++position;
    });
    return hits;
}

}  // namespace webtable

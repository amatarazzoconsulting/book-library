// MIT License - (c) Anthony Matarazzo 2026
#include "table.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace webtable {
namespace {

std::string trim(const std::string& s) {
    std::size_t a = 0;
    std::size_t b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

std::string lower(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

}  // namespace

Value Value::from_text(const std::string& s) {
    Value v;
    v.type = FieldType::kText;
    v.text = s;
    return v;
}

Value Value::from_integer(long long n) {
    Value v;
    v.type = FieldType::kInteger;
    v.integer = n;
    v.text = std::to_string(n);
    return v;
}

Value Value::from_decimal(double d) {
    Value v;
    v.type = FieldType::kDecimal;
    v.decimal = d;
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << d;
    v.text = out.str();
    return v;
}

std::string Value::to_string() const {
    switch (type) {
        case FieldType::kInteger: return std::to_string(integer);
        case FieldType::kDecimal: {
            std::ostringstream out;
            out << std::fixed << std::setprecision(2) << decimal;
            return out.str();
        }
        case FieldType::kText: break;
    }
    return text;
}

double Value::as_number() const {
    switch (type) {
        case FieldType::kInteger: return static_cast<double>(integer);
        case FieldType::kDecimal: return decimal;
        case FieldType::kText: break;
    }
    try {
        return std::stod(text);
    } catch (const std::exception&) {
        return 0.0;
    }
}

bool Table::add_row(const std::vector<std::string>& cells, std::string* error) {
    if (cells.size() != schema_.columns.size()) {
        if (error) *error = "expected " + std::to_string(schema_.columns.size()) + " fields, got " +
                            std::to_string(cells.size());
        return false;
    }
    Row row;
    row.values.reserve(cells.size());
    for (std::size_t i = 0; i < cells.size(); ++i) {
        const Column& col = schema_.columns[i];
        const std::string cell = trim(cells[i]);
        switch (col.type) {
            case FieldType::kInteger: {
                try {
                    row.values.push_back(Value::from_integer(std::stoll(cell)));
                } catch (const std::exception&) {
                    if (error) *error = "column " + col.name + ": '" + cell + "' is not an Integer";
                    return false;
                }
                break;
            }
            case FieldType::kDecimal: {
                try {
                    row.values.push_back(Value::from_decimal(std::stod(cell)));
                } catch (const std::exception&) {
                    if (error) *error = "column " + col.name + ": '" + cell + "' is not a Decimal";
                    return false;
                }
                break;
            }
            case FieldType::kText:
                row.values.push_back(Value::from_text(cell));
                break;
        }
    }
    const int key = schema_.key_index();
    if (key >= 0) {
        const std::string k = row.values[static_cast<std::size_t>(key)].to_string();
        if (key_map_.count(k)) {
            if (error) *error = "duplicate key '" + k + "'";
            return false;
        }
        key_map_[k] = rows_.size();
    }
    const std::size_t position = rows_.size();
    rows_.push_back(std::move(row));
    for (auto& kv : indexes_) {
        const int col = schema_.column_index(kv.first);
        if (col < 0) continue;
        kv.second[rows_[position].values[static_cast<std::size_t>(col)].to_string()].push_back(position);
    }
    return true;
}

std::size_t Table::load(const std::string& data, std::string* error) {
    std::size_t loaded = 0;
    std::istringstream in(data);
    std::string line;
    std::size_t line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        while (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> cells;
        std::string cell;
        std::istringstream fields(line);
        while (std::getline(fields, cell, schema_.separator)) cells.push_back(cell);
        std::string row_error;
        if (!add_row(cells, &row_error)) {
            if (error) *error = "line " + std::to_string(line_no) + ": " + row_error;
            return loaded;
        }
        ++loaded;
    }
    return loaded;
}

bool Table::build_index(const std::string& column) {
    const int col = schema_.column_index(column);
    if (col < 0) return false;
    auto& table = indexes_[column];
    table.clear();
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        table[rows_[i].values[static_cast<std::size_t>(col)].to_string()].push_back(i);
    }
    return true;
}

const Row* Table::find_by_key(const std::string& key) const {
    auto it = key_map_.find(key);
    if (it == key_map_.end()) return nullptr;
    ++index_hits_;
    return &rows_[it->second];
}

void Table::visit_all(const Visitor& visitor) const {
    for (const auto& row : rows_) {
        ++scan_rows_;
        visitor(row);
    }
}

void Table::visit_index(const std::string& column, const std::string& value,
                        const Visitor& visitor) const {
    auto idx = indexes_.find(column);
    if (idx == indexes_.end()) {
        const int col = schema_.column_index(column);
        if (col < 0) return;
        for (const auto& row : rows_) {
            ++scan_rows_;
            if (row.values[static_cast<std::size_t>(col)].to_string() == value) visitor(row);
        }
        return;
    }
    auto bucket = idx->second.find(value);
    if (bucket == idx->second.end()) return;
    for (const auto position : bucket->second) {
        ++index_hits_;
        visitor(rows_[position]);
    }
}

void Table::visit_text_contains(const std::string& column, const std::string& needle,
                                const Visitor& visitor) const {
    const int col = schema_.column_index(column);
    if (col < 0) return;
    const std::string target = lower(needle);
    for (const auto& row : rows_) {
        ++scan_rows_;
        if (lower(row.values[static_cast<std::size_t>(col)].to_string()).find(target) !=
            std::string::npos) {
            visitor(row);
        }
    }
}

}  // namespace webtable

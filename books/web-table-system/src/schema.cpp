// MIT License - (c) Anthony Matarazzo 2026
#include "schema.hpp"

#include <cctype>
#include <sstream>

namespace webtable {
namespace {

std::vector<std::string> words(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream in(line);
    std::string w;
    while (in >> w) out.push_back(w);
    return out;
}

FieldType parse_type(const std::string& token) {
    if (token == "Integer" || token == "Int") return FieldType::kInteger;
    if (token == "Decimal" || token == "Float" || token == "Real") return FieldType::kDecimal;
    return FieldType::kText;
}

}  // namespace

const char* type_name(FieldType type) {
    switch (type) {
        case FieldType::kText: return "Text";
        case FieldType::kInteger: return "Integer";
        case FieldType::kDecimal: return "Decimal";
    }
    return "Text";
}

bool Schema::parse(const std::string& text, std::string* error) {
    columns.clear();
    index_tokens.clear();
    name.clear();
    separator = ',';
    bool in_columns = false;
    std::istringstream in(text);
    std::string line;
    int line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.empty()) continue;
        if (line[0] == '#') continue;  // comment lines are ignored by the format
        if (line == "***") { in_columns = false; continue; }

        const auto w = words(line);
        if (w.empty()) continue;
        if (w[0] == "Table" && w.size() >= 2) { name = w[1]; continue; }
        if (w[0] == "Columns") { in_columns = true; continue; }
        if (w[0] == "Separator" && w.size() >= 2) { separator = w[1][0]; continue; }
        if (w[0] == "Index" && w.size() >= 2) { index_tokens.push_back(w[1]); continue; }
        if (w[0] == "Data" || w[0] == "Rows") { in_columns = false; continue; }

        if (!in_columns) {
            if (error) *error = "line " + std::to_string(line_no) + ": unexpected token '" + w[0] + "'";
            return false;
        }
        Column c;
        c.name = w[0];
        c.type = w.size() > 1 ? parse_type(w[1]) : FieldType::kText;
        for (std::size_t i = 2; i < w.size(); ++i) {
            const std::string& t = w[i];
            if (t == "Key") { c.key = true; continue; }
            if (t == "Unique") { c.unique = true; continue; }
            if (t.rfind("Width=", 0) == 0) { c.width = std::stoi(t.substr(6)); continue; }
            if (t.rfind("Precision=", 0) == 0) { c.precision = std::stoi(t.substr(10)); continue; }
            if (t.rfind("Rounding=", 0) == 0) { c.rounding = t.substr(9); continue; }
            if (error) *error = "line " + std::to_string(line_no) + ": unknown token '" + t + "'";
            return false;
        }
        if (c.name.empty()) {
            if (error) *error = "line " + std::to_string(line_no) + ": column without a name";
            return false;
        }
        columns.push_back(c);
    }
    if (name.empty()) { if (error) *error = "missing Table declaration"; return false; }
    if (columns.empty()) { if (error) *error = "missing Columns section"; return false; }
    int keys = 0;
    for (const auto& c : columns) keys += c.key ? 1 : 0;
    if (keys > 1) { if (error) *error = "more than one Key column"; return false; }
    return true;
}

int Schema::column_index(const std::string& column_name) const {
    for (std::size_t i = 0; i < columns.size(); ++i) {
        if (columns[i].name == column_name) return static_cast<int>(i);
    }
    return -1;
}

int Schema::key_index() const {
    for (std::size_t i = 0; i < columns.size(); ++i) {
        if (columns[i].key) return static_cast<int>(i);
    }
    return -1;
}

}  // namespace webtable

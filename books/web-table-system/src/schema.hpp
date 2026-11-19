// MIT License - (c) Anthony Matarazzo 2026
// Parser for the WebTableSystem text schema (Table / Columns / Key / Separator tokens).
#pragma once
#include <string>
#include <vector>

namespace webtable {

enum class FieldType { kText, kInteger, kDecimal };

struct Column {
    std::string name;
    FieldType type = FieldType::kText;
    bool key = false;
    bool unique = false;
    int width = 0;         // Width=<n>, 0 means unset
    int precision = -1;    // Precision=<n>, -1 means unset
    std::string rounding;  // Rounding=<Nearest|Up|Down>
};

struct Schema {
    std::string name;
    std::vector<Column> columns;
    char separator = ',';
    std::vector<std::string> index_tokens;

    // Parses the token based schema text. Returns false and fills error on failure.
    bool parse(const std::string& text, std::string* error);

    int column_index(const std::string& column_name) const;
    int key_index() const;
};

const char* type_name(FieldType type);

}  // namespace webtable

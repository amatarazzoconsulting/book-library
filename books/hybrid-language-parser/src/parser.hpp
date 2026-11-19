// MIT License - (c) Anthony Matarazzo 2026
// Recursive descent statement parser with precedence climbing expressions.
// Takes the token stream from the multi-language lexer and produces the flat
// AST in ast.hpp. Block syntax is chosen from the source language, so the same
// grammar body serves brace languages, `end` languages and Python indentation.
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "ast.hpp"
#include "lexer.hpp"

namespace hybrid {

struct ParserOptions {
    Language language = Language::Unknown; // Unknown means detect
    bool detect_language = true;
    bool allow_indented_blocks = true; // Python style colon blocks
    bool allow_end_blocks = true;      // BASIC WEND / END IF, COBOL END-IF
    bool allow_print_keyword = true;
    bool require_semicolons = false; // strict mode for brace languages
    std::size_t max_depth = 256;     // recursion guard
    std::size_t max_errors = 8;      // stop collecting after this many
};

struct ParserStats {
    std::size_t tokens = 0;
    std::size_t statements = 0;
    std::size_t nodes = 0;
    std::size_t depth = 0;
    std::size_t climbs = 0;   // precedence climbing loop iterations
    std::size_t max_precedence_seen = 0;
    std::size_t errors = 0;
};

struct ParseResult {
    Program program;
    ParserStats stats;
    Language language = Language::Unknown;
    std::vector<std::string> errors;
    bool ok() const { return errors.empty(); }
};

// Parses source text end to end. Lexer errors are reported first and stop the
// parse, because a broken token stream produces meaningless syntax errors.
ParseResult parse(std::string_view source, const ParserOptions& options = ParserOptions());

// Parses an already lexed stream. Useful for teaching the two phases apart and
// for benchmarks that measure parse time without lexing.
ParseResult parse_tokens(const std::vector<Token>& tokens, Language language,
                         const ParserOptions& options = ParserOptions());

// Fully parenthesised one line rendering, for tests and teaching.
std::string to_sexpr(const NodePtr& node);

// Indented tree with node kinds, operators and source positions.
std::string dump_tree(const NodePtr& node);

} // namespace hybrid

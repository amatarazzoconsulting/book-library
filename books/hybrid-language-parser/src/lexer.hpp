// MIT License - (c) Anthony Matarazzo 2026
// Multi-language lexer: one scanner, per-language keyword tables, automatic
// language detection. This is the front end described in the book's
// "Token System" and "Language Detector" sections.
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace hybrid {

enum class Language {
    Unknown = 0, C, Cpp, Python, Go, Basic, Cobol, JavaScript, TypeScript, Rust, Zig
};

const char* language_name(Language lang);
Language language_from_name(std::string_view name);

enum class TokenType {
    Number, String, Char, Identifier, Keyword, Operator, Punct,
    Comment, Newline, EndOfInput, Invalid
};

const char* token_type_name(TokenType type);

// Cross-language keyword identity. The tables below map the spelling used by
// each language onto this shared set so the parser never needs to know which
// language produced the token.
enum class Keyword {
    None = 0,
    If, Else, ElseIf, While, For, Do, Break, Continue, Return, Goto,
    Switch, Case, Default, End,
    Func, Def, Function, Procedure, Lambda,
    Class, Struct, Union, Enum, Interface, Extends, Implements,
    Var, Let, Const, Auto, Static, Extern,
    New, Delete, This, Super, Public, Private, Protected,
    Virtual, Override, Final, Abstract,
    True, False, Null,
    Print, Println, Input, Read, Write,
    Int, Float, Double, Bool, String, Char, Void, Byte,
    And, Or, Not, Xor, Compl, In, Is,
    Import, From, Package, Go, Defer, Range, Select, Type, Map, Make,
    Append, Len, Cap, Copy, Panic, Recover, Yield, With, As, Pass,
    Raise, Try, Catch, Finally, Throw, Async, Await, Global, Nonlocal, Del
};

const char* keyword_name(Keyword kw);

struct Token {
    TokenType type = TokenType::EndOfInput;
    std::string lexeme;    // raw text, or the decoded body for strings and chars
    int line = 1;
    int column = 1;
    double number = 0.0;   // valid for TokenType::Number
    bool is_float = false; // number had a '.' or exponent
    Keyword keyword = Keyword::None;

    bool is(TokenType t) const { return type == t; }
    bool is_keyword(Keyword kw) const { return type == TokenType::Keyword && keyword == kw; }
    bool is_op(std::string_view text) const {
        return type == TokenType::Operator && lexeme == text;
    }
    std::string describe() const;
};

struct LexerOptions {
    Language language = Language::Unknown; // Unknown means detect
    bool detect_language = true;
    bool keep_comments = false;
    bool emit_newlines = false;      // useful for Python style line rules
    bool case_insensitive = false;   // forced on for BASIC and COBOL
    bool allow_underscore_digits = true;
    bool allow_dollar_identifiers = true; // BASIC name$ suffixes
    bool nested_block_comments = false;   // Rust and Zig allow nesting
    std::size_t max_tokens = 0;           // 0 means unlimited
};

struct LexerStats {
    std::size_t tokens = 0;
    std::size_t keywords = 0;
    std::size_t identifiers = 0;
    std::size_t numbers = 0;
    std::size_t strings = 0;
    std::size_t operators = 0;
    std::size_t comments = 0;
    std::size_t invalid = 0;
    std::size_t lines = 1;
};

struct LexResult {
    std::vector<Token> tokens;
    LexerStats stats;
    Language language = Language::Unknown;
    std::vector<std::string> errors;
    bool ok() const { return errors.empty(); }
};

struct DetectionResult {
    Language language = Language::Unknown;
    int score = 0;
    std::string reason;
};

// Scores a source string using shebang lines, keyword frequencies and
// syntactic markers. Deterministic: ties resolve by enum order.
DetectionResult detect_language(std::string_view source);

// Returns Keyword::None when the word is not a keyword of that language.
Keyword lookup_keyword(Language lang, std::string_view word);

// All keywords known for a language, sorted, for teaching and tests.
std::vector<std::string> keywords_for(Language lang);

// Tokenizes with the given options. Tokens never include the sentinel
// EndOfInput entry; it is appended by callers that need it.
LexResult lex(std::string_view source, const LexerOptions& options = LexerOptions());

// Convenience: detect (unless overridden), drop comments, return tokens.
std::vector<Token> tokenize(std::string_view source,
                            Language language = Language::Unknown,
                            LexerStats* stats = nullptr);

} // namespace hybrid

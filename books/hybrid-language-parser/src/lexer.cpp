// MIT License - (c) Anthony Matarazzo 2026
#include "lexer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <utility>

namespace hybrid {
namespace {

constexpr int kLanguageCount = 11;

bool is_digit(char c) { return c >= '0' && c <= '9'; }
bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
bool is_alnum(char c) { return is_alpha(c) || is_digit(c); }
bool is_space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f'; }
char lower(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c; }

std::string lowered(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out.push_back(lower(c));
    return out;
}

// ---------------------------------------------------------------------------
// Keyword tables. One row per language, spelling -> shared Keyword id. The
// scanner never has to know the source language beyond this lookup.
// ---------------------------------------------------------------------------

struct KwPair { const char* text; Keyword kw; };

struct KeywordTables {
    std::array<std::unordered_map<std::string, Keyword>, kLanguageCount> by_lang;
    std::vector<std::pair<Language, std::string>> spellings;
};

void add_row(std::unordered_map<std::string, Keyword>& m,
             std::vector<std::pair<Language, std::string>>& spellings,
             Language lang, std::initializer_list<KwPair> list) {
    for (const KwPair& p : list) {
        m.emplace(lowered(p.text), p.kw);
        spellings.emplace_back(lang, p.text);
    }
}

const KeywordTables& keyword_tables() {
    static const KeywordTables tables = [] {
        KeywordTables t;
        auto at = [&t](Language l) -> std::unordered_map<std::string, Keyword>& {
            return t.by_lang[static_cast<std::size_t>(l)];
        };
        auto row = [&t, &at](Language l, std::initializer_list<KwPair> list) {
            add_row(at(l), t.spellings, l, list);
        };

        row(Language::C, {
            {"if", Keyword::If}, {"else", Keyword::Else}, {"while", Keyword::While},
            {"for", Keyword::For}, {"do", Keyword::Do}, {"break", Keyword::Break},
            {"continue", Keyword::Continue}, {"return", Keyword::Return},
            {"goto", Keyword::Goto}, {"switch", Keyword::Switch}, {"case", Keyword::Case},
            {"default", Keyword::Default}, {"struct", Keyword::Struct},
            {"union", Keyword::Union}, {"enum", Keyword::Enum}, {"typedef", Keyword::Type},
            {"sizeof", Keyword::Len}, {"static", Keyword::Static},
            {"extern", Keyword::Extern}, {"const", Keyword::Const}, {"auto", Keyword::Auto},
            {"register", Keyword::Auto}, {"volatile", Keyword::Extern},
            {"void", Keyword::Void}, {"int", Keyword::Int}, {"float", Keyword::Float},
            {"double", Keyword::Double}, {"char", Keyword::Char}, {"short", Keyword::Int},
            {"long", Keyword::Int}, {"unsigned", Keyword::Byte}, {"signed", Keyword::Int},
            {"true", Keyword::True}, {"false", Keyword::False}, {"NULL", Keyword::Null},
            {"include", Keyword::Import}, {"define", Keyword::Let},
            {"ifdef", Keyword::If}, {"ifndef", Keyword::If}, {"endif", Keyword::End},
            {"printf", Keyword::Print}, {"scanf", Keyword::Input}});

        row(Language::Cpp, {
            {"if", Keyword::If}, {"else", Keyword::Else}, {"while", Keyword::While},
            {"for", Keyword::For}, {"do", Keyword::Do}, {"break", Keyword::Break},
            {"continue", Keyword::Continue}, {"return", Keyword::Return},
            {"goto", Keyword::Goto}, {"switch", Keyword::Switch}, {"case", Keyword::Case},
            {"default", Keyword::Default}, {"struct", Keyword::Struct},
            {"union", Keyword::Union}, {"enum", Keyword::Enum}, {"typedef", Keyword::Type},
            {"sizeof", Keyword::Len}, {"static", Keyword::Static},
            {"extern", Keyword::Extern}, {"const", Keyword::Const}, {"auto", Keyword::Auto},
            {"void", Keyword::Void}, {"int", Keyword::Int}, {"float", Keyword::Float},
            {"double", Keyword::Double}, {"char", Keyword::Char}, {"unsigned", Keyword::Byte},
            {"true", Keyword::True}, {"false", Keyword::False}, {"NULL", Keyword::Null},
            {"class", Keyword::Class}, {"public", Keyword::Public},
            {"private", Keyword::Private}, {"protected", Keyword::Protected},
            {"virtual", Keyword::Virtual}, {"override", Keyword::Override},
            {"final", Keyword::Final}, {"new", Keyword::New}, {"delete", Keyword::Delete},
            {"this", Keyword::This}, {"namespace", Keyword::Package},
            {"using", Keyword::Import}, {"template", Keyword::Class},
            {"typename", Keyword::Type}, {"constexpr", Keyword::Const},
            {"nullptr", Keyword::Null}, {"bool", Keyword::Bool}, {"friend", Keyword::Private},
            {"operator", Keyword::Func}, {"try", Keyword::Try}, {"catch", Keyword::Catch},
            {"throw", Keyword::Throw}, {"noexcept", Keyword::Extern},
            {"inline", Keyword::Static}, {"explicit", Keyword::Extern},
            {"mutable", Keyword::Var}, {"decltype", Keyword::Type},
            {"concept", Keyword::Interface}, {"requires", Keyword::Extends},
            {"include", Keyword::Import}, {"define", Keyword::Let},
            {"cout", Keyword::Print}, {"cin", Keyword::Input}, {"std", Keyword::Package}});

        row(Language::Python, {
            {"def", Keyword::Def}, {"if", Keyword::If}, {"elif", Keyword::ElseIf},
            {"else", Keyword::Else}, {"while", Keyword::While}, {"for", Keyword::For},
            {"in", Keyword::In}, {"is", Keyword::Is}, {"not", Keyword::Not},
            {"and", Keyword::And}, {"or", Keyword::Or}, {"return", Keyword::Return},
            {"break", Keyword::Break}, {"continue", Keyword::Continue},
            {"pass", Keyword::Pass}, {"class", Keyword::Class},
            {"import", Keyword::Import}, {"from", Keyword::From}, {"as", Keyword::As},
            {"lambda", Keyword::Lambda}, {"True", Keyword::True},
            {"False", Keyword::False}, {"None", Keyword::Null}, {"print", Keyword::Print},
            {"input", Keyword::Input}, {"len", Keyword::Len}, {"global", Keyword::Global},
            {"nonlocal", Keyword::Nonlocal}, {"del", Keyword::Del}, {"try", Keyword::Try},
            {"except", Keyword::Catch}, {"finally", Keyword::Finally},
            {"raise", Keyword::Raise}, {"yield", Keyword::Yield}, {"with", Keyword::With},
            {"async", Keyword::Async}, {"await", Keyword::Await}, {"assert", Keyword::Case},
            {"int", Keyword::Int}, {"float", Keyword::Float}, {"str", Keyword::String},
            {"bool", Keyword::Bool}, {"list", Keyword::Map}, {"dict", Keyword::Map},
            {"range", Keyword::Range}, {"self", Keyword::This}});

        row(Language::Go, {
            {"func", Keyword::Func}, {"if", Keyword::If}, {"else", Keyword::Else},
            {"for", Keyword::For}, {"range", Keyword::Range}, {"return", Keyword::Return},
            {"break", Keyword::Break}, {"continue", Keyword::Continue},
            {"switch", Keyword::Switch}, {"case", Keyword::Case},
            {"default", Keyword::Default}, {"struct", Keyword::Struct},
            {"interface", Keyword::Interface}, {"type", Keyword::Type},
            {"var", Keyword::Var}, {"const", Keyword::Const}, {"package", Keyword::Package},
            {"import", Keyword::Import}, {"go", Keyword::Go}, {"defer", Keyword::Defer},
            {"select", Keyword::Select}, {"map", Keyword::Map}, {"make", Keyword::Make},
            {"append", Keyword::Append}, {"len", Keyword::Len}, {"cap", Keyword::Cap},
            {"copy", Keyword::Copy}, {"panic", Keyword::Panic}, {"recover", Keyword::Recover},
            {"chan", Keyword::Map}, {"true", Keyword::True}, {"false", Keyword::False},
            {"nil", Keyword::Null}, {"int", Keyword::Int}, {"int64", Keyword::Int},
            {"float64", Keyword::Double}, {"string", Keyword::String},
            {"byte", Keyword::Byte}, {"bool", Keyword::Bool}, {"goto", Keyword::Goto},
            {"fallthrough", Keyword::Default}, {"error", Keyword::String}});

        row(Language::Basic, {
            {"IF", Keyword::If}, {"THEN", Keyword::If}, {"ELSE", Keyword::Else},
            {"ELSEIF", Keyword::ElseIf}, {"ENDIF", Keyword::End}, {"WHILE", Keyword::While},
            {"WEND", Keyword::End}, {"FOR", Keyword::For}, {"TO", Keyword::For},
            {"STEP", Keyword::For}, {"NEXT", Keyword::Continue}, {"DO", Keyword::Do},
            {"LOOP", Keyword::End}, {"EXIT", Keyword::Break}, {"BREAK", Keyword::Break},
            {"CONTINUE", Keyword::Continue}, {"GOTO", Keyword::Goto},
            {"GOSUB", Keyword::Procedure}, {"RETURN", Keyword::Return},
            {"SUB", Keyword::Procedure}, {"FUNCTION", Keyword::Function},
            {"END", Keyword::End}, {"PRINT", Keyword::Print}, {"INPUT", Keyword::Input},
            {"LET", Keyword::Let}, {"DIM", Keyword::Var}, {"AS", Keyword::As},
            {"AND", Keyword::And}, {"OR", Keyword::Or}, {"NOT", Keyword::Not},
            {"XOR", Keyword::Xor}, {"TRUE", Keyword::True}, {"FALSE", Keyword::False},
            {"INTEGER", Keyword::Int}, {"SINGLE", Keyword::Float},
            {"DOUBLE", Keyword::Double}, {"STRING", Keyword::String},
            {"SELECT", Keyword::Switch}, {"CASE", Keyword::Case},
            {"RANDOMIZE", Keyword::Make}, {"CLS", Keyword::Print}});

        row(Language::Cobol, {
            {"IDENTIFICATION", Keyword::Package}, {"PROGRAM-ID", Keyword::Package},
            {"DIVISION", Keyword::Package}, {"SECTION", Keyword::Procedure},
            {"WORKING-STORAGE", Keyword::Var}, {"PROCEDURE", Keyword::Procedure},
            {"IF", Keyword::If}, {"ELSE", Keyword::Else}, {"END-IF", Keyword::End},
            {"PERFORM", Keyword::While}, {"UNTIL", Keyword::While},
            {"VARYING", Keyword::For}, {"FROM", Keyword::For}, {"BY", Keyword::For},
            {"DISPLAY", Keyword::Print}, {"ACCEPT", Keyword::Input},
            {"MOVE", Keyword::Write}, {"COMPUTE", Keyword::Write},
            {"ADD", Keyword::Write}, {"STOP", Keyword::End}, {"RUN", Keyword::End},
            {"AND", Keyword::And}, {"OR", Keyword::Or}, {"NOT", Keyword::Not},
            {"TRUE", Keyword::True}, {"FALSE", Keyword::False}, {"PIC", Keyword::Type},
            {"PICTURE", Keyword::Type}, {"VALUE", Keyword::Let}, {"OCCURS", Keyword::For},
            {"GIVING", Keyword::Write}, {"THEN", Keyword::If}, {"GO", Keyword::Goto},
            {"TO", Keyword::For}, {"THRU", Keyword::For}, {"IS", Keyword::Is},
            {"IN", Keyword::In}, {"OF", Keyword::In}, {"ZERO", Keyword::Null},
            {"SPACES", Keyword::Null}});

        row(Language::JavaScript, {
            {"var", Keyword::Var}, {"let", Keyword::Let}, {"const", Keyword::Const},
            {"function", Keyword::Function}, {"if", Keyword::If}, {"else", Keyword::Else},
            {"while", Keyword::While}, {"for", Keyword::For}, {"do", Keyword::Do},
            {"break", Keyword::Break}, {"continue", Keyword::Continue},
            {"return", Keyword::Return}, {"switch", Keyword::Switch},
            {"case", Keyword::Case}, {"default", Keyword::Default},
            {"class", Keyword::Class}, {"extends", Keyword::Extends},
            {"new", Keyword::New}, {"delete", Keyword::Delete}, {"this", Keyword::This},
            {"super", Keyword::Super}, {"static", Keyword::Static},
            {"true", Keyword::True}, {"false", Keyword::False}, {"null", Keyword::Null},
            {"undefined", Keyword::Null}, {"typeof", Keyword::Type},
            {"instanceof", Keyword::Is}, {"in", Keyword::In}, {"of", Keyword::In},
            {"import", Keyword::Import}, {"from", Keyword::From},
            {"export", Keyword::Public}, {"async", Keyword::Async},
            {"await", Keyword::Await}, {"yield", Keyword::Yield}, {"try", Keyword::Try},
            {"catch", Keyword::Catch}, {"finally", Keyword::Finally},
            {"throw", Keyword::Throw}, {"console", Keyword::Package},
            {"require", Keyword::Import}, {"arguments", Keyword::Var},
            {"void", Keyword::Void}, {"debugger", Keyword::Break}});

        row(Language::TypeScript, {
            {"var", Keyword::Var}, {"let", Keyword::Let}, {"const", Keyword::Const},
            {"function", Keyword::Function}, {"if", Keyword::If}, {"else", Keyword::Else},
            {"while", Keyword::While}, {"for", Keyword::For}, {"do", Keyword::Do},
            {"break", Keyword::Break}, {"continue", Keyword::Continue},
            {"return", Keyword::Return}, {"switch", Keyword::Switch},
            {"case", Keyword::Case}, {"default", Keyword::Default},
            {"class", Keyword::Class}, {"extends", Keyword::Extends},
            {"new", Keyword::New}, {"delete", Keyword::Delete}, {"this", Keyword::This},
            {"super", Keyword::Super}, {"static", Keyword::Static},
            {"true", Keyword::True}, {"false", Keyword::False}, {"null", Keyword::Null},
            {"undefined", Keyword::Null}, {"typeof", Keyword::Type},
            {"instanceof", Keyword::Is}, {"in", Keyword::In}, {"of", Keyword::In},
            {"import", Keyword::Import}, {"from", Keyword::From},
            {"export", Keyword::Public}, {"async", Keyword::Async},
            {"await", Keyword::Await}, {"yield", Keyword::Yield}, {"try", Keyword::Try},
            {"catch", Keyword::Catch}, {"finally", Keyword::Finally},
            {"throw", Keyword::Throw}, {"interface", Keyword::Interface},
            {"implements", Keyword::Implements}, {"public", Keyword::Public},
            {"private", Keyword::Private}, {"protected", Keyword::Protected},
            {"readonly", Keyword::Const}, {"enum", Keyword::Enum}, {"type", Keyword::Type},
            {"as", Keyword::As}, {"any", Keyword::Type}, {"number", Keyword::Double},
            {"string", Keyword::String}, {"boolean", Keyword::Bool},
            {"never", Keyword::Void}, {"unknown", Keyword::Type},
            {"declare", Keyword::Extern}, {"abstract", Keyword::Abstract},
            {"override", Keyword::Override}, {"namespace", Keyword::Package},
            {"module", Keyword::Package}, {"satisfies", Keyword::Implements},
            {"keyof", Keyword::Type}, {"void", Keyword::Void}});

        row(Language::Rust, {
            {"fn", Keyword::Func}, {"let", Keyword::Let}, {"mut", Keyword::Var},
            {"if", Keyword::If}, {"else", Keyword::Else}, {"while", Keyword::While},
            {"loop", Keyword::While}, {"for", Keyword::For}, {"in", Keyword::In},
            {"return", Keyword::Return}, {"break", Keyword::Break},
            {"continue", Keyword::Continue}, {"match", Keyword::Switch},
            {"struct", Keyword::Struct}, {"enum", Keyword::Enum},
            {"impl", Keyword::Implements}, {"trait", Keyword::Interface},
            {"pub", Keyword::Public}, {"use", Keyword::Import}, {"mod", Keyword::Package},
            {"const", Keyword::Const}, {"static", Keyword::Static},
            {"true", Keyword::True}, {"false", Keyword::False}, {"None", Keyword::Null},
            {"Some", Keyword::Null}, {"Ok", Keyword::Null}, {"Err", Keyword::Null},
            {"self", Keyword::This}, {"Self", Keyword::This}, {"super", Keyword::Super},
            {"as", Keyword::As}, {"unsafe", Keyword::Extern}, {"async", Keyword::Async},
            {"await", Keyword::Await}, {"move", Keyword::Write}, {"where", Keyword::Extends},
            {"dyn", Keyword::Virtual}, {"ref", Keyword::Var}, {"crate", Keyword::Package},
            {"i32", Keyword::Int}, {"i64", Keyword::Int}, {"u32", Keyword::Int},
            {"f64", Keyword::Double}, {"f32", Keyword::Float}, {"str", Keyword::String},
            {"bool", Keyword::Bool}, {"u8", Keyword::Byte}, {"usize", Keyword::Int},
            {"println", Keyword::Print}, {"print", Keyword::Print}, {"vec", Keyword::Make},
            {"Vec", Keyword::Map}});

        row(Language::Zig, {
            {"fn", Keyword::Func}, {"const", Keyword::Const}, {"var", Keyword::Var},
            {"if", Keyword::If}, {"else", Keyword::Else}, {"while", Keyword::While},
            {"for", Keyword::For}, {"return", Keyword::Return}, {"break", Keyword::Break},
            {"continue", Keyword::Continue}, {"switch", Keyword::Switch},
            {"struct", Keyword::Struct}, {"enum", Keyword::Enum},
            {"union", Keyword::Union}, {"pub", Keyword::Public},
            {"comptime", Keyword::Const}, {"true", Keyword::True},
            {"false", Keyword::False}, {"null", Keyword::Null},
            {"undefined", Keyword::Null}, {"defer", Keyword::Defer}, {"try", Keyword::Try},
            {"catch", Keyword::Catch}, {"orelse", Keyword::Else}, {"and", Keyword::And},
            {"or", Keyword::Or}, {"test", Keyword::Function}, {"i32", Keyword::Int},
            {"u64", Keyword::Int}, {"f64", Keyword::Double}, {"u8", Keyword::Byte},
            {"bool", Keyword::Bool}, {"void", Keyword::Void}, {"asm", Keyword::Extern},
            {"error", Keyword::String}, {"unreachable", Keyword::End},
            {"export", Keyword::Public}, {"extern", Keyword::Extern},
            {"usingnamespace", Keyword::Import}, {"align", Keyword::Var},
            {"callconv", Keyword::Var}});

        return t;
    }();
    return tables;
}

// ---------------------------------------------------------------------------
// Operator table, longest first so maximal munch is a simple prefix scan.
// ---------------------------------------------------------------------------

const std::string_view* operator_table(std::size_t& count) {
    static const std::string_view ops[] = {
        ">>>=", "<<=", ">>=", "**=", "===", "!==", "...", "->*", ">>>",
        "==", "!=", "<=", ">=", "&&", "||", "++", "--", "+=", "-=", "*=", "/=",
        "%=", "&=", "|=", "^=", "<<", ">>", "->", "::", ":=", "**", "=>", "??",
        "?.", "..", "<-", "=", "<", ">", "!", "&", "|", "^", "~", "?", "+", "-",
        "*", "/", "%"};
    count = sizeof(ops) / sizeof(ops[0]);
    return ops;
}

std::string encode_utf8(std::uint32_t cp) {
    std::string out;
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return out;
}

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool is_c_like(Language l) {
    return l == Language::C || l == Language::Cpp || l == Language::Go ||
           l == Language::JavaScript || l == Language::TypeScript ||
           l == Language::Rust || l == Language::Zig;
}

bool uses_slash_comments(Language l) { return is_c_like(l); }
bool uses_hash_comments(Language l) { return l == Language::Python; }
bool uses_tick_comments(Language l) {
    return l == Language::Basic || l == Language::Cobol;
}
bool single_quote_is_char(Language l) {
    return l == Language::C || l == Language::Cpp || l == Language::Go ||
           l == Language::Rust || l == Language::Zig;
}

// Scanner. Kept as a struct so every helper can advance the same cursor and
// keep line/column bookkeeping in one place.
struct Scanner {
    std::string_view src;
    LexerOptions opt;
    Language lang = Language::Unknown;
    std::size_t i = 0;
    int line = 1;
    int column = 1;
    LexResult out;
    bool stopped = false;

    explicit Scanner(std::string_view s, const LexerOptions& o) : src(s), opt(o) {}

    bool at_end() const { return i >= src.size(); }
    char peek(std::size_t ahead = 0) const {
        return (i + ahead < src.size()) ? src[i + ahead] : '\0';
    }
    bool match(std::string_view text) const {
        return src.size() - i >= text.size() && src.compare(i, text.size(), text) == 0;
    }

    void advance(std::size_t n = 1) {
        for (std::size_t k = 0; k < n && i < src.size(); ++k) {
            if (src[i] == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
            ++i;
        }
    }

    void fail(const std::string& message) {
        out.errors.push_back("line " + std::to_string(line) + ", column " +
                             std::to_string(column) + ": " + message);
    }

    void push(TokenType type, std::string lexeme, int start_line, int start_column) {
        if (stopped) return;
        if (opt.max_tokens != 0 && out.tokens.size() >= opt.max_tokens) {
            stopped = true;
            return;
        }
        Token t;
        t.type = type;
        t.lexeme = std::move(lexeme);
        t.line = start_line;
        t.column = start_column;
        if (type == TokenType::Keyword) t.keyword = lookup_keyword(lang, t.lexeme);
        out.tokens.push_back(std::move(t));
    }

    // Reads an escape sequence starting at the backslash; returns the decoded
    // text and advances past it.
    std::string read_escape() {
        advance(); // backslash
        if (at_end()) {
            fail("escape at end of input");
            return {};
        }
        char c = src[i];
        advance();
        switch (c) {
            case 'n': return "\n";
            case 't': return "\t";
            case 'r': return "\r";
            case '0': return std::string(1, '\0');
            case 'a': return "\a";
            case 'b': return "\b";
            case 'f': return "\f";
            case 'v': return "\v";
            case '\\': return "\\";
            case '\'': return "'";
            case '"': return "\"";
            case 'x': {
                int hi = hex_value(peek(0));
                int lo = hex_value(peek(1));
                if (hi < 0 || lo < 0) {
                    fail("bad \\x escape");
                    return "x";
                }
                advance(2);
                return std::string(1, static_cast<char>(hi * 16 + lo));
            }
            case 'u': {
                std::uint32_t cp = 0;
                for (int k = 0; k < 4; ++k) {
                    int v = hex_value(peek(0));
                    if (v < 0) {
                        fail("bad \\u escape");
                        return "u";
                    }
                    cp = cp * 16 + static_cast<std::uint32_t>(v);
                    advance();
                }
                return encode_utf8(cp);
            }
            default:
                fail(std::string("unknown escape \\") + c);
                return std::string(1, c);
        }
    }

    std::string read_quoted(char quote, bool allow_newline) {
        advance(); // opening quote
        std::string body;
        while (!at_end()) {
            char c = src[i];
            if (c == '\\') {
                body += read_escape();
                continue;
            }
            if (c == quote) {
                advance();
                return body;
            }
            if (c == '\n' && !allow_newline) {
                fail("unterminated literal");
                return body;
            }
            body.push_back(c);
            advance();
        }
        fail("unterminated literal");
        return body;
    }

    void scan_line_comment(std::size_t skip) {
        int start_line = line;
        int start_column = column;
        std::size_t begin = i;
        while (!at_end() && src[i] != '\n') advance();
        if (opt.keep_comments) {
            push(TokenType::Comment, std::string(src.substr(begin, i - begin)),
                 start_line, start_column);
        }
        out.stats.comments += 1;
        (void)skip;
    }

    bool scan_block_comment() {
        int start_line = line;
        int start_column = column;
        std::size_t begin = i;
        advance(2); // "/*"
        int depth = 1;
        while (!at_end()) {
            if (opt.nested_block_comments && match("/*")) {
                ++depth;
                advance(2);
                continue;
            }
            if (match("*/")) {
                advance(2);
                --depth;
                if (depth == 0) {
                    if (opt.keep_comments) {
                        push(TokenType::Comment,
                             std::string(src.substr(begin, i - begin)), start_line,
                             start_column);
                    }
                    out.stats.comments += 1;
                    return true;
                }
                continue;
            }
            advance();
        }
        fail("unterminated block comment");
        return false;
    }

    // BASIC: REM to end of line, but only where a statement can begin so that
    // an identifier named REM still works inside an expression.
    bool at_statement_start() const {
        if (out.tokens.empty()) return true;
        const Token& t = out.tokens.back();
        if (t.type == TokenType::Newline) return true;
        if (t.type == TokenType::Punct && (t.lexeme == ":" || t.lexeme == ";")) return true;
        return false;
    }

    void scan_number() {
        int start_line = line;
        int start_column = column;
        std::size_t begin = i;
        bool is_float = false;
        int base = 10;

        if (peek(0) == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
            base = 16;
            advance(2);
            while (is_digit(peek(0)) || hex_value(peek(0)) >= 0 ||
                   (peek(0) == '_' && opt.allow_underscore_digits)) {
                advance();
            }
        } else if (peek(0) == '0' && (peek(1) == 'b' || peek(1) == 'B')) {
            base = 2;
            advance(2);
            while (peek(0) == '0' || peek(0) == '1' ||
                   (peek(0) == '_' && opt.allow_underscore_digits)) {
                advance();
            }
        } else if (peek(0) == '0' && (peek(1) == 'o' || peek(1) == 'O')) {
            base = 8;
            advance(2);
            while ((peek(0) >= '0' && peek(0) <= '7') ||
                   (peek(0) == '_' && opt.allow_underscore_digits)) {
                advance();
            }
        } else {
            while (is_digit(peek(0)) ||
                   (peek(0) == '_' && opt.allow_underscore_digits)) {
                advance();
            }
            if (peek(0) == '.' && is_digit(peek(1))) {
                is_float = true;
                advance();
                while (is_digit(peek(0)) ||
                       (peek(0) == '_' && opt.allow_underscore_digits)) {
                    advance();
                }
            }
            if (peek(0) == 'e' || peek(0) == 'E') {
                std::size_t save = i;
                int save_line = line;
                int save_column = column;
                advance();
                if (peek(0) == '+' || peek(0) == '-') advance();
                if (is_digit(peek(0))) {
                    is_float = true;
                    while (is_digit(peek(0))) advance();
                } else {
                    i = save;
                    line = save_line;
                    column = save_column;
                }
            }
        }

        // Type suffixes (f, L, u, ll) and BASIC/COBOL type markers (% # ! &).
        while (!at_end()) {
            char c = src[i];
            if (c == 'f' || c == 'F' || c == 'l' || c == 'L' || c == 'u' || c == 'U') {
                advance();
                continue;
            }
            break;
        }
        if (uses_tick_comments(lang)) {
            char c = peek(0);
            if (c == '%' || c == '#' || c == '!' || c == '&') advance();
        }

        std::string text(src.substr(begin, i - begin));
        std::string digits;
        digits.reserve(text.size());
        for (char c : text) {
            if (c != '_') digits.push_back(c);
        }
        // Strip trailing suffix characters so the numeric conversion is clean.
        std::size_t cut = digits.size();
        while (cut > 0 && (digits[cut - 1] == 'f' || digits[cut - 1] == 'F' ||
                           digits[cut - 1] == 'l' || digits[cut - 1] == 'L' ||
                           digits[cut - 1] == 'u' || digits[cut - 1] == 'U')) {
            --cut;
        }
        std::string body = digits.substr(0, cut);
        while (!body.empty() && (body.back() == '%' || body.back() == '#' ||
                                 body.back() == '!' || body.back() == '&')) {
            body.pop_back();
        }

        double value = 0.0;
        if (base != 10) {
            unsigned long long acc = 0;
            for (std::size_t k = 2; k < body.size(); ++k) {
                int d = hex_value(body[k]);
                if (d < 0) continue;
                acc = acc * static_cast<unsigned long long>(base) +
                      static_cast<unsigned long long>(d);
            }
            value = static_cast<double>(acc);
        } else {
            const char* cstr = body.c_str();
            char* end = nullptr;
            value = std::strtod(cstr, &end);
            if (end == cstr) {
                fail("bad numeric literal '" + text + "'");
                push(TokenType::Invalid, text, start_line, start_column);
                out.stats.invalid += 1;
                return;
            }
            if (body.find('.') != std::string::npos ||
                body.find('e') != std::string::npos ||
                body.find('E') != std::string::npos) {
                is_float = true;
            }
        }

        if (stopped) return;
        if (opt.max_tokens != 0 && out.tokens.size() >= opt.max_tokens) {
            stopped = true;
            return;
        }
        Token t;
        t.type = TokenType::Number;
        t.lexeme = text;
        t.line = start_line;
        t.column = start_column;
        t.number = value;
        t.is_float = is_float;
        out.tokens.push_back(std::move(t));
        out.stats.numbers += 1;
    }

    void scan_identifier() {
        int start_line = line;
        int start_column = column;
        std::size_t begin = i;
        while (!at_end()) {
            char c = src[i];
            if (is_alnum(c)) {
                advance();
                continue;
            }
            if (c == '$' && opt.allow_dollar_identifiers) {
                advance();
                continue;
            }
            // COBOL words are hyphenated: WORKING-STORAGE, END-IF.
            if (c == '-' && lang == Language::Cobol && is_alnum(peek(1))) {
                advance();
                continue;
            }
            break;
        }
        std::string text(src.substr(begin, i - begin));
        Keyword kw = lookup_keyword(lang, text);
        if (kw != Keyword::None) {
            if (stopped) return;
            if (opt.max_tokens != 0 && out.tokens.size() >= opt.max_tokens) {
                stopped = true;
                return;
            }
            Token t;
            t.type = TokenType::Keyword;
            t.lexeme = text;
            t.line = start_line;
            t.column = start_column;
            t.keyword = kw;
            out.tokens.push_back(std::move(t));
            out.stats.keywords += 1;
            return;
        }
        push(TokenType::Identifier, text, start_line, start_column);
        out.stats.identifiers += 1;
    }

    void run() {
        std::size_t op_count = 0;
        const std::string_view* ops = operator_table(op_count);

        while (!at_end() && !stopped) {
            char c = src[i];

            if (is_space(c)) {
                advance();
                continue;
            }

            if (c == '\n') {
                int start_line = line;
                int start_column = column;
                advance();
                if (opt.emit_newlines) push(TokenType::Newline, "\n", start_line, start_column);
                continue;
            }

            // Comments.
            if (uses_slash_comments(lang) && match("//")) {
                scan_line_comment(2);
                continue;
            }
            if (uses_slash_comments(lang) && match("/*")) {
                scan_block_comment();
                continue;
            }
            if (uses_hash_comments(lang) && c == '#') {
                scan_line_comment(1);
                continue;
            }
            // C and C++ preprocessor lines are not expressions; the scanner
            // records them as comments so a source file with includes still
            // tokenizes into a clean statement stream.
            if ((lang == Language::C || lang == Language::Cpp) && c == '#' &&
                at_statement_start()) {
                scan_line_comment(1);
                continue;
            }
            if (lang == Language::Cobol && match("*>")) {
                scan_line_comment(2);
                continue;
            }
            if (uses_tick_comments(lang) && c == '\'') {
                // BASIC and COBOL use a leading apostrophe for remarks.
                scan_line_comment(1);
                continue;
            }
            if (lang == Language::Cobol && c == '*' && column == 7) {
                scan_line_comment(1);
                continue;
            }
            if (lang == Language::Basic && at_statement_start()) {
                std::size_t remain = src.size() - i;
                if (remain >= 3 && lower(src[i]) == 'r' && lower(src[i + 1]) == 'e' &&
                    lower(src[i + 2]) == 'm' && (remain == 3 || !is_alnum(src[i + 3]))) {
                    scan_line_comment(3);
                    continue;
                }
            }

            // String and character literals.
            if (c == '"') {
                if (lang == Language::Python && match("\"\"\"")) {
                    int sl = line;
                    int sc = column;
                    advance(3);
                    std::size_t begin = i;
                    std::string body;
                    bool closed = false;
                    while (!at_end()) {
                        if (match("\"\"\"")) {
                            body = std::string(src.substr(begin, i - begin));
                            advance(3);
                            closed = true;
                            break;
                        }
                        if (src[i] == '\\') {
                            body += read_escape();
                            begin = i;
                            continue;
                        }
                        advance();
                    }
                    if (!closed) fail("unterminated triple-quoted string");
                    push(TokenType::String, body, sl, sc);
                    out.stats.strings += 1;
                    continue;
                }
                int sl = line;
                int sc = column;
                std::string body = read_quoted('"', lang == Language::Python);
                push(TokenType::String, body, sl, sc);
                out.stats.strings += 1;
                continue;
            }
            if (c == '\'' && lang == Language::Python && match("'''")) {
                int sl = line;
                int sc = column;
                advance(3);
                std::size_t begin = i;
                std::string body;
                bool closed = false;
                while (!at_end()) {
                    if (match("'''")) {
                        body = std::string(src.substr(begin, i - begin));
                        advance(3);
                        closed = true;
                        break;
                    }
                    if (src[i] == '\\') {
                        body += read_escape();
                        begin = i;
                        continue;
                    }
                    advance();
                }
                if (!closed) fail("unterminated triple-quoted string");
                push(TokenType::String, body, sl, sc);
                out.stats.strings += 1;
                continue;
            }
            if (c == '\'') {
                int sl = line;
                int sc = column;
                std::string body = read_quoted('\'', false);
                push(single_quote_is_char(lang) ? TokenType::Char : TokenType::String, body, sl, sc);
                out.stats.strings += 1;
                continue;
            }
            if (c == '`' && (lang == Language::JavaScript || lang == Language::TypeScript)) {
                int sl = line;
                int sc = column;
                std::string body = read_quoted('`', true);
                push(TokenType::String, body, sl, sc);
                out.stats.strings += 1;
                continue;
            }
            if (c == 'r' && lang == Language::Rust && peek(1) == '"') {
                int sl = line;
                int sc = column;
                advance(); // r
                std::string body = read_quoted('"', true);
                push(TokenType::String, body, sl, sc);
                out.stats.strings += 1;
                continue;
            }

            // Numbers, including a leading dot form such as .5.
            if (is_digit(c) || (c == '.' && is_digit(peek(1)))) {
                scan_number();
                continue;
            }

            // Zig addresses builtins with a leading @ (@import, @intCast).
            if (c == '@' && lang == Language::Zig && is_alpha(peek(1))) {
                int sl = line;
                int sc = column;
                advance();
                scan_identifier();
                if (!out.tokens.empty() && out.tokens.back().line == sl) {
                    out.tokens.back().lexeme.insert(out.tokens.back().lexeme.begin(), '@');
                    out.tokens.back().column = sc;
                }
                continue;
            }

            if (is_alpha(c)) {
                scan_identifier();
                continue;
            }

            // Operators: maximal munch over the table.
            bool matched = false;
            for (std::size_t k = 0; k < op_count; ++k) {
                const std::string_view& op = ops[k];
                if (match(op)) {
                    int sl = line;
                    int sc = column;
                    advance(op.size());
                    push(TokenType::Operator, std::string(op), sl, sc);
                    out.stats.operators += 1;
                    matched = true;
                    break;
                }
            }
            if (matched) continue;

            if (std::strchr("()[]{},;:.", c) != nullptr) {
                int sl = line;
                int sc = column;
                advance();
                push(TokenType::Punct, std::string(1, c), sl, sc);
                out.stats.operators += 1;
                continue;
            }

            int sl = line;
            int sc = column;
            advance();
            fail(std::string("unexpected character '") + c + "'");
            push(TokenType::Invalid, std::string(1, c), sl, sc);
            out.stats.invalid += 1;
        }

        out.stats.tokens = out.tokens.size();
        out.stats.lines = static_cast<std::size_t>(line);
    }
};

// ---------------------------------------------------------------------------
// Detection helpers.
// ---------------------------------------------------------------------------

struct Scores {
    std::array<int, kLanguageCount> value{};
    std::array<std::string, kLanguageCount> why{};

    void add(Language l, int points, const std::string& reason) {
        std::size_t idx = static_cast<std::size_t>(l);
        value[idx] += points;
        if (why[idx].empty()) {
            why[idx] = reason;
        } else if (why[idx].size() < 96) {
            why[idx] += ", " + reason;
        }
    }
};

void count_word(Scores& s, std::string_view word) {
    // Characteristic words. A hit is worth 2 points; rare words worth more.
    struct Rule { const char* word; Language lang; int points; };
    static const Rule rules[] = {
        {"def", Language::Python, 4}, {"elif", Language::Python, 6},
        {"lambda", Language::Python, 5}, {"None", Language::Python, 4},
        {"self", Language::Python, 2}, {"import", Language::Python, 1},
        {"fn", Language::Rust, 4}, {"impl", Language::Rust, 6},
        {"mut", Language::Rust, 6}, {"trait", Language::Rust, 6},
        {"crate", Language::Rust, 6}, {"println", Language::Rust, 4},
        {"func", Language::Go, 6}, {"package", Language::Go, 4},
        {"defer", Language::Go, 5}, {"chan", Language::Go, 6},
        {"nil", Language::Go, 4}, {"golang", Language::Go, 6},
        {"function", Language::JavaScript, 3}, {"var", Language::JavaScript, 2},
        {"undefined", Language::JavaScript, 5}, {"console", Language::JavaScript, 4},
        {"interface", Language::TypeScript, 4}, {"implements", Language::TypeScript, 4},
        {"readonly", Language::TypeScript, 6}, {"satisfies", Language::TypeScript, 6},
        {"namespace", Language::TypeScript, 3}, {"declare", Language::TypeScript, 5},
        {"template", Language::Cpp, 4}, {"constexpr", Language::Cpp, 6},
        {"nullptr", Language::Cpp, 5}, {"namespace", Language::Cpp, 5},
        {"virtual", Language::Cpp, 4}, {"override", Language::Cpp, 3},
        {"cout", Language::Cpp, 4}, {"std", Language::Cpp, 2},
        {"typedef", Language::C, 4}, {"struct", Language::C, 1},
        {"printf", Language::C, 4}, {"include", Language::C, 3},
        {"DIM", Language::Basic, 6}, {"WEND", Language::Basic, 8},
        {"PRINT", Language::Basic, 2}, {"GOSUB", Language::Basic, 8},
        {"ELSEIF", Language::Basic, 5}, {"RANDOMIZE", Language::Basic, 8},
        {"PERFORM", Language::Cobol, 8}, {"DIVISION", Language::Cobol, 8},
        {"WORKING-STORAGE", Language::Cobol, 9}, {"PIC", Language::Cobol, 6},
        {"PROGRAM-ID", Language::Cobol, 9}, {"DISPLAY", Language::Cobol, 5},
        {"comptime", Language::Zig, 8}, {"orelse", Language::Zig, 8},
        {"usingnamespace", Language::Zig, 9}, {"test", Language::Zig, 3},
    };
    for (const Rule& r : rules) {
        if (word == r.word) s.add(r.lang, r.points, r.word);
    }
}

Scores score_source(std::string_view source) {
    Scores s;
    std::size_t n = source.size();

    // Shebang line is decisive.
    if (n > 2 && source[0] == '#' && source[1] == '!') {
        std::size_t end = source.find('\n');
        std::string_view first = source.substr(0, end == std::string_view::npos ? n : end);
        if (first.find("python") != std::string_view::npos) {
            s.add(Language::Python, 40, "shebang python");
        } else if (first.find("node") != std::string_view::npos) {
            s.add(Language::JavaScript, 40, "shebang node");
        } else if (first.find("sh") != std::string_view::npos) {
            s.add(Language::Unknown, 0, "");
        }
    }

    // Word scan, honouring a small set of characteristic spellings.
    std::size_t i = 0;
    while (i < n) {
        char c = source[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            std::size_t begin = i;
            while (i < n) {
                char d = source[i];
                bool word_char = (d >= 'a' && d <= 'z') || (d >= 'A' && d <= 'Z') ||
                                 (d >= '0' && d <= '9') || d == '_';
                if (!word_char && d == '-' && i + 1 < n) {
                    char e = source[i + 1];
                    if ((e >= 'A' && e <= 'Z') || (e >= 'a' && e <= 'z')) {
                        ++i;
                        continue;
                    }
                }
                if (!word_char) break;
                ++i;
            }
            std::string_view word = source.substr(begin, i - begin);
            count_word(s, word);
            continue;
        }
        ++i;
    }

    // Structural markers.
    auto has = [&](std::string_view needle) {
        return source.find(needle) != std::string_view::npos;
    };
    if (has("#include")) s.add(Language::C, 8, "#include");
    if (has("std::") || has("->")) s.add(Language::Cpp, 5, "std:: or ->");
    if (has("::")) s.add(Language::Cpp, 3, "::");
    if (has("</") && has("<?php") == false) s.add(Language::JavaScript, 1, "tag");
    if (has(":=")) s.add(Language::Go, 6, ":=");
    if (has("=>")) s.add(Language::TypeScript, 2, "=>");
    if (has(": number") || has(": string") || has(": boolean")) {
        s.add(Language::TypeScript, 6, "type annotation");
    }
    if (has("END-IF") || has("END-PERFORM") || has("01 ")) {
        s.add(Language::Cobol, 8, "COBOL division marker");
    }
    if (has("fn main()")) s.add(Language::Rust, 6, "fn main");
    if (has("fn ") && has("var ") && has("const ")) s.add(Language::Zig, 2, "zig style");
    if (has("pub fn")) s.add(Language::Rust, 5, "pub fn");
    if (has("@import")) s.add(Language::Zig, 10, "@import");
    if (has(") void")) s.add(Language::Zig, 8, ") void");
    if (has("!void")) s.add(Language::Zig, 8, "!void");
    if (has("unreachable")) s.add(Language::Zig, 6, "unreachable");
    if (has("std.debug.print")) s.add(Language::Zig, 8, "std.debug.print");
    if (has("@import") || has("!void")) {
        // Zig shares fn/pub/const with Rust, so a Zig marker overrules the
        // Rust word hits instead of merely adding to the tally.
        s.add(Language::Zig, 12, "zig markers");
        s.add(Language::Rust, -10, "zig markers");
    }
    if (has("func main()")) s.add(Language::Go, 8, "func main");
    if (has("let mut")) s.add(Language::Rust, 6, "let mut");
    if (has("dim ") || has("Dim ")) s.add(Language::Basic, 3, "dim");
    if (has("BEGIN") && has("END.")) s.add(Language::Cobol, 6, "BEGIN/END.");

    // Indentation instead of braces is Python's strongest signal.
    if (has(":\n") && !has("{") && !has("}")) s.add(Language::Python, 6, "colon blocks, no braces");
    if (has("{\n") && has("}\n")) s.add(Language::C, 1, "brace blocks");

    return s;
}

} // namespace

const char* language_name(Language lang) {
    switch (lang) {
        case Language::Unknown: return "Unknown";
        case Language::C: return "C";
        case Language::Cpp: return "C++";
        case Language::Python: return "Python";
        case Language::Go: return "Go";
        case Language::Basic: return "BASIC";
        case Language::Cobol: return "COBOL";
        case Language::JavaScript: return "JavaScript";
        case Language::TypeScript: return "TypeScript";
        case Language::Rust: return "Rust";
        case Language::Zig: return "Zig";
    }
    return "Unknown";
}

Language language_from_name(std::string_view name) {
    std::string key = lowered(name);
    if (key == "c") return Language::C;
    if (key == "cpp" || key == "c++" || key == "cxx") return Language::Cpp;
    if (key == "python" || key == "py") return Language::Python;
    if (key == "go" || key == "golang") return Language::Go;
    if (key == "basic" || key == "qbasic") return Language::Basic;
    if (key == "cobol") return Language::Cobol;
    if (key == "javascript" || key == "js") return Language::JavaScript;
    if (key == "typescript" || key == "ts") return Language::TypeScript;
    if (key == "rust" || key == "rs") return Language::Rust;
    if (key == "zig") return Language::Zig;
    return Language::Unknown;
}

const char* token_type_name(TokenType type) {
    switch (type) {
        case TokenType::Number: return "Number";
        case TokenType::String: return "String";
        case TokenType::Char: return "Char";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Keyword: return "Keyword";
        case TokenType::Operator: return "Operator";
        case TokenType::Punct: return "Punct";
        case TokenType::Comment: return "Comment";
        case TokenType::Newline: return "Newline";
        case TokenType::EndOfInput: return "EndOfInput";
        case TokenType::Invalid: return "Invalid";
    }
    return "Invalid";
}

const char* keyword_name(Keyword kw) {
    switch (kw) {
        case Keyword::None: return "None";
        case Keyword::If: return "If";
        case Keyword::Else: return "Else";
        case Keyword::ElseIf: return "ElseIf";
        case Keyword::While: return "While";
        case Keyword::For: return "For";
        case Keyword::Do: return "Do";
        case Keyword::Break: return "Break";
        case Keyword::Continue: return "Continue";
        case Keyword::Return: return "Return";
        case Keyword::Goto: return "Goto";
        case Keyword::Switch: return "Switch";
        case Keyword::Case: return "Case";
        case Keyword::Default: return "Default";
        case Keyword::End: return "End";
        case Keyword::Func: return "Func";
        case Keyword::Def: return "Def";
        case Keyword::Function: return "Function";
        case Keyword::Procedure: return "Procedure";
        case Keyword::Lambda: return "Lambda";
        case Keyword::Class: return "Class";
        case Keyword::Struct: return "Struct";
        case Keyword::Union: return "Union";
        case Keyword::Enum: return "Enum";
        case Keyword::Interface: return "Interface";
        case Keyword::Extends: return "Extends";
        case Keyword::Implements: return "Implements";
        case Keyword::Var: return "Var";
        case Keyword::Let: return "Let";
        case Keyword::Const: return "Const";
        case Keyword::Auto: return "Auto";
        case Keyword::Static: return "Static";
        case Keyword::Extern: return "Extern";
        case Keyword::New: return "New";
        case Keyword::Delete: return "Delete";
        case Keyword::This: return "This";
        case Keyword::Super: return "Super";
        case Keyword::Public: return "Public";
        case Keyword::Private: return "Private";
        case Keyword::Protected: return "Protected";
        case Keyword::Virtual: return "Virtual";
        case Keyword::Override: return "Override";
        case Keyword::Final: return "Final";
        case Keyword::Abstract: return "Abstract";
        case Keyword::True: return "True";
        case Keyword::False: return "False";
        case Keyword::Null: return "Null";
        case Keyword::Print: return "Print";
        case Keyword::Println: return "Println";
        case Keyword::Input: return "Input";
        case Keyword::Read: return "Read";
        case Keyword::Write: return "Write";
        case Keyword::Int: return "Int";
        case Keyword::Float: return "Float";
        case Keyword::Double: return "Double";
        case Keyword::Bool: return "Bool";
        case Keyword::String: return "String";
        case Keyword::Char: return "Char";
        case Keyword::Void: return "Void";
        case Keyword::Byte: return "Byte";
        case Keyword::And: return "And";
        case Keyword::Or: return "Or";
        case Keyword::Not: return "Not";
        case Keyword::Xor: return "Xor";
        case Keyword::Compl: return "Compl";
        case Keyword::In: return "In";
        case Keyword::Is: return "Is";
        case Keyword::Import: return "Import";
        case Keyword::From: return "From";
        case Keyword::Package: return "Package";
        case Keyword::Go: return "Go";
        case Keyword::Defer: return "Defer";
        case Keyword::Range: return "Range";
        case Keyword::Select: return "Select";
        case Keyword::Type: return "Type";
        case Keyword::Map: return "Map";
        case Keyword::Make: return "Make";
        case Keyword::Append: return "Append";
        case Keyword::Len: return "Len";
        case Keyword::Cap: return "Cap";
        case Keyword::Copy: return "Copy";
        case Keyword::Panic: return "Panic";
        case Keyword::Recover: return "Recover";
        case Keyword::Yield: return "Yield";
        case Keyword::With: return "With";
        case Keyword::As: return "As";
        case Keyword::Pass: return "Pass";
        case Keyword::Raise: return "Raise";
        case Keyword::Try: return "Try";
        case Keyword::Catch: return "Catch";
        case Keyword::Finally: return "Finally";
        case Keyword::Throw: return "Throw";
        case Keyword::Async: return "Async";
        case Keyword::Await: return "Await";
        case Keyword::Global: return "Global";
        case Keyword::Nonlocal: return "Nonlocal";
        case Keyword::Del: return "Del";
    }
    return "None";
}

std::string Token::describe() const {
    std::string out = token_type_name(type);
    out += " '";
    for (char c : lexeme) {
        if (c == '\n') {
            out += "\\n";
        } else if (c == '\t') {
            out += "\\t";
        } else {
            out += c;
        }
    }
    out += "' @";
    out += std::to_string(line);
    out += ":";
    out += std::to_string(column);
    return out;
}

DetectionResult detect_language(std::string_view source) {
    Scores s = score_source(source);
    DetectionResult best;
    for (std::size_t k = 1; k < static_cast<std::size_t>(kLanguageCount); ++k) {
        if (s.value[k] > best.score) {
            best.score = s.value[k];
            best.language = static_cast<Language>(k);
            best.reason = s.why[k];
        }
    }
    if (best.score == 0) {
        best.language = Language::Unknown;
        best.reason = "no signal";
    }
    return best;
}

Keyword lookup_keyword(Language lang, std::string_view word) {
    if (lang == Language::Unknown) return Keyword::None;
    const KeywordTables& tables = keyword_tables();
    const auto& map = tables.by_lang[static_cast<std::size_t>(lang)];
    std::string key;
    if (lang == Language::Basic || lang == Language::Cobol) {
        key = lowered(word);
    } else {
        key.assign(word.begin(), word.end());
    }
    auto it = map.find(key);
    return (it == map.end()) ? Keyword::None : it->second;
}

std::vector<std::string> keywords_for(Language lang) {
    std::vector<std::string> out;
    if (lang == Language::Unknown) return out;
    const KeywordTables& tables = keyword_tables();
    for (const auto& entry : tables.spellings) {
        if (entry.first == lang) out.push_back(entry.second);
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

LexResult lex(std::string_view source, const LexerOptions& options) {
    Scanner scanner(source, options);
    Language lang = options.language;
    if (lang == Language::Unknown && options.detect_language) {
        DetectionResult d = detect_language(source);
        lang = d.language;
    }
    if (lang == Language::Unknown) lang = Language::C;
    scanner.lang = lang;
    scanner.out.language = lang;
    scanner.run();
    return std::move(scanner.out);
}

std::vector<Token> tokenize(std::string_view source, Language language, LexerStats* stats) {
    LexerOptions options;
    options.language = language;
    options.detect_language = (language == Language::Unknown);
    LexResult result = lex(source, options);
    if (stats != nullptr) *stats = result.stats;
    return std::move(result.tokens);
}

} // namespace hybrid

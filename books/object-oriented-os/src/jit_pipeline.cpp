// MIT License - (c) Anthony Matarazzo 2026
#include "jit_pipeline.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <stdexcept>

namespace nebula {

// ---------------------------------------------------------------- AST nodes

BlockPtr Block::make() { return std::make_shared<Block>(); }

ExprPtr Expr::Const(const Box& v) {
    auto e = std::make_shared<Expr>();
    e->kind = Kind::CONST;
    e->value = v;
    return e;
}

ExprPtr Expr::Var(const std::string& n) {
    auto e = std::make_shared<Expr>();
    e->kind = Kind::VAR;
    e->name = n;
    return e;
}

ExprPtr Expr::BinOp_(BinOp op, ExprPtr l, ExprPtr r) {
    auto e = std::make_shared<Expr>();
    e->kind = Kind::BINOP;
    e->binop = op;
    e->lhs = std::move(l);
    e->rhs = std::move(r);
    return e;
}

ExprPtr Expr::UnaryOp_(UnOp op, ExprPtr v) {
    auto e = std::make_shared<Expr>();
    e->kind = Kind::UNARYOP;
    e->unop = op;
    e->lhs = std::move(v);
    return e;
}

ExprPtr Expr::Call_(const std::string& n, const std::vector<ExprPtr>& a) {
    auto e = std::make_shared<Expr>();
    e->kind = Kind::CALL;
    e->name = n;
    e->args = a;
    return e;
}

ExprPtr Expr::clone() const {
    auto e = std::make_shared<Expr>(*this);
    if (lhs) e->lhs = lhs->clone();
    if (rhs) e->rhs = rhs->clone();
    e->args.clear();
    for (const ExprPtr& a : args) e->args.push_back(a->clone());
    return e;
}

std::size_t Expr::nodeCount() const {
    std::size_t n = 1;
    if (lhs) n += lhs->nodeCount();
    if (rhs) n += rhs->nodeCount();
    for (const ExprPtr& a : args) n += a->nodeCount();
    return n;
}

namespace {
const char* binOpText(BinOp op) {
    switch (op) {
        case BinOp::ADD: return "+";
        case BinOp::SUB: return "-";
        case BinOp::MUL: return "*";
        case BinOp::DIV: return "/";
        case BinOp::MOD: return "%";
        case BinOp::LT: return "<";
        case BinOp::GT: return ">";
        case BinOp::LE: return "<=";
        case BinOp::GE: return ">=";
        case BinOp::EQ: return "==";
        case BinOp::NE: return "!=";
        case BinOp::BAND: return "&";
        case BinOp::BOR: return "|";
        case BinOp::BXOR: return "^";
        case BinOp::SHL: return "<<";
        case BinOp::SHR: return ">>";
    }
    return "?";
}
}  // namespace

std::string Expr::toString() const {
    switch (kind) {
        case Kind::CONST: return value.asString();
        case Kind::VAR: return name;
        case Kind::BINOP:
            return "(" + (lhs ? lhs->toString() : std::string("?")) + " " +
                   binOpText(binop) + " " + (rhs ? rhs->toString() : std::string("?")) + ")";
        case Kind::UNARYOP:
            return std::string(unop == UnOp::NEG ? "-" : "!") +
                   (lhs ? lhs->toString() : std::string("?"));
        case Kind::CALL: {
            std::string s = name + "(";
            for (std::size_t i = 0; i < args.size(); ++i) {
                if (i) s += ", ";
                s += args[i]->toString();
            }
            return s + ")";
        }
    }
    return "?";
}

StmtPtr Stmt::Expr_(ExprPtr e) {
    auto s = std::make_shared<Stmt>();
    s->kind = Kind::EXPR;
    s->expr = std::move(e);
    return s;
}

StmtPtr Stmt::Assign(const std::string& v, ExprPtr e) {
    auto s = std::make_shared<Stmt>();
    s->kind = Kind::ASSIGN;
    s->var = v;
    s->expr = std::move(e);
    return s;
}

StmtPtr Stmt::If(ExprPtr cond, BlockPtr thenB, BlockPtr elseB) {
    auto s = std::make_shared<Stmt>();
    s->kind = Kind::IF;
    s->expr = std::move(cond);
    s->body = thenB ? thenB : Block::make();
    s->elseBody = elseB;
    return s;
}

StmtPtr Stmt::While(ExprPtr cond, BlockPtr body) {
    auto s = std::make_shared<Stmt>();
    s->kind = Kind::WHILE;
    s->expr = std::move(cond);
    s->body = body ? body : Block::make();
    return s;
}

StmtPtr Stmt::Return(ExprPtr v) {
    auto s = std::make_shared<Stmt>();
    s->kind = Kind::RETURN;
    s->expr = std::move(v);
    return s;
}

StmtPtr Stmt::Block_(BlockPtr b) {
    auto s = std::make_shared<Stmt>();
    s->kind = Kind::BLOCK;
    s->body = b ? b : Block::make();
    return s;
}

StmtPtr Stmt::clone() const {
    auto s = std::make_shared<Stmt>(*this);
    if (expr) s->expr = expr->clone();
    if (body) {
        s->body = Block::make();
        for (const StmtPtr& c : body->stmts) s->body->stmts.push_back(c->clone());
    }
    if (elseBody) {
        s->elseBody = Block::make();
        for (const StmtPtr& c : elseBody->stmts) s->elseBody->stmts.push_back(c->clone());
    }
    return s;
}

std::string Stmt::toString() const {
    switch (kind) {
        case Kind::EXPR: return "expr " + expr->toString() + ";";
        case Kind::ASSIGN: return var + " = " + expr->toString() + ";";
        case Kind::IF: return "if " + expr->toString() + " { ... }";
        case Kind::WHILE: return "while " + expr->toString() + " { ... }";
        case Kind::RETURN: return "return " + (expr ? expr->toString() : std::string("void")) + ";";
        case Kind::BLOCK: return "{ ... }";
    }
    return "?";
}

// ------------------------------------------------------------------- Lexer

namespace {

enum class Tok { END, NUM, STR, IDENT, OP, LPAREN, RPAREN, LBRACE, RBRACE,
                 SEMI, COMMA };

struct Token {
    Tok kind = Tok::END;
    std::string text;
    Box value;
    std::size_t pos = 0;
};

bool isIdentStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
bool isIdentChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

class Lexer {
public:
    explicit Lexer(std::string src) : src_(std::move(src)) {}

    std::vector<Token> run() {
        std::vector<Token> out;
        while (true) {
            skipSpace();
            Token t;
            t.pos = i_;
            if (i_ >= src_.size()) {
                t.kind = Tok::END;
                out.push_back(t);
                return out;
            }
            const char c = src_[i_];
            if (std::isdigit(static_cast<unsigned char>(c))) {
                out.push_back(number());
            } else if (isIdentStart(c)) {
                out.push_back(word());
            } else if (c == '"' || c == '\'') {
                out.push_back(stringLit());
            } else {
                out.push_back(punct());
            }
        }
    }

private:
    void skipSpace() {
        while (i_ < src_.size()) {
            const char c = src_[i_];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { ++i_; continue; }
            if (c == '#' || (c == '/' && i_ + 1 < src_.size() && src_[i_ + 1] == '/')) {
                while (i_ < src_.size() && src_[i_] != '\n') ++i_;
                continue;
            }
            break;
        }
    }

    Token number() {
        Token t;
        t.kind = Tok::NUM;
        const std::size_t start = i_;
        bool is_float = false;
        while (i_ < src_.size() &&
               (std::isdigit(static_cast<unsigned char>(src_[i_])) || src_[i_] == '.')) {
            if (src_[i_] == '.') is_float = true;
            ++i_;
        }
        t.text = src_.substr(start, i_ - start);
        if (is_float) t.value = Box(std::stod(t.text));
        else t.value = Box(static_cast<long long>(std::stoll(t.text)));
        return t;
    }

    Token word() {
        Token t;
        t.kind = Tok::IDENT;
        const std::size_t start = i_;
        while (i_ < src_.size() && isIdentChar(src_[i_])) ++i_;
        t.text = src_.substr(start, i_ - start);
        return t;
    }

    Token stringLit() {
        Token t;
        t.kind = Tok::STR;
        const char quote = src_[i_++];
        std::string s;
        while (i_ < src_.size() && src_[i_] != quote) {
            if (src_[i_] == '\\' && i_ + 1 < src_.size()) {
                ++i_;
                switch (src_[i_]) {
                    case 'n': s += '\n'; break;
                    case 't': s += '\t'; break;
                    default: s += src_[i_]; break;
                }
            } else {
                s += src_[i_];
            }
            ++i_;
        }
        if (i_ < src_.size()) ++i_;
        t.text = s;
        t.value = Box(s);
        return t;
    }

    Token punct() {
        Token t;
        const char c = src_[i_++];
        t.text = std::string(1, c);
        switch (c) {
            case '(': t.kind = Tok::LPAREN; break;
            case ')': t.kind = Tok::RPAREN; break;
            case '{': t.kind = Tok::LBRACE; break;
            case '}': t.kind = Tok::RBRACE; break;
            case ';': t.kind = Tok::SEMI; break;
            case ',': t.kind = Tok::COMMA; break;
            default: t.kind = Tok::OP; break;
        }
        if (i_ < src_.size()) {
            const std::string two = std::string(1, c) + src_[i_];
            static const char* ops[] = {"==", "!=", "<=", ">=", "<<", ">>", "&&", "||"};
            for (const char* o : ops) {
                if (two == o) {
                    t.text = two;
                    ++i_;
                    break;
                }
            }
        }
        return t;
    }

    std::string src_;
    std::size_t i_ = 0;
};

class Parser {
public:
    explicit Parser(std::vector<Token> toks) : t_(std::move(toks)) {}

    BlockPtr parseProgram() {
        BlockPtr b = Block::make();
        while (!at(Tok::END)) {
            if (at(Tok::SEMI)) { ++p_; continue; }
            b->stmts.push_back(parseStatement());
        }
        return b;
    }

    ExprPtr parseExpressionOnly() {
        ExprPtr e = parseExpr();
        if (!at(Tok::END)) throw std::runtime_error("Parser: trailing input");
        return e;
    }

private:
    const Token& cur() const { return t_[p_]; }
    bool at(Tok kind) const { return cur().kind == kind; }
    bool atWord(const char* w) const {
        return cur().kind == Tok::IDENT && cur().text == w;
    }
    void expect(Tok k, const char* what) {
        if (!at(k)) throw std::runtime_error(std::string("Parser: expected ") + what);
        ++p_;
    }

    StmtPtr parseStatement() {
        if (atWord("let")) { ++p_; return parseAssignment(); }  // BASIC sugar
        if (atWord("print")) {
            ++p_;
            ExprPtr e;
            if (at(Tok::LPAREN)) {
                ++p_;
                e = parseExpr();
                expect(Tok::RPAREN, ")");
            } else {
                e = parseExpr();
            }
            auto call = Expr::Call_("print", {e});
            return Stmt::Expr_(call);
        }
        if (atWord("return")) {
            ++p_;
            if (at(Tok::SEMI) || at(Tok::RBRACE) || at(Tok::END)) {
                if (at(Tok::SEMI)) ++p_;
                return Stmt::Return(nullptr);
            }
            ExprPtr e = parseExpr();
            if (at(Tok::SEMI)) ++p_;
            return Stmt::Return(e);
        }
        if (atWord("if")) return parseIf();
        if (atWord("while")) return parseWhile();
        if (at(Tok::LBRACE)) {
            ++p_;
            BlockPtr b = Block::make();
            while (!at(Tok::RBRACE)) {
                if (at(Tok::END)) throw std::runtime_error("Parser: unterminated block");
                if (at(Tok::SEMI)) { ++p_; continue; }
                b->stmts.push_back(parseStatement());
            }
            ++p_;
            return Stmt::Block_(b);
        }
        return parseAssignment();
    }

    StmtPtr parseAssignment() {
        if (cur().kind == Tok::IDENT && p_ + 1 < t_.size() &&
            t_[p_ + 1].kind == Tok::OP && t_[p_ + 1].text == "=") {
            const std::string name = cur().text;
            p_ += 2;
            ExprPtr e = parseExpr();
            if (at(Tok::SEMI)) ++p_;
            return Stmt::Assign(name, e);
        }
        ExprPtr e = parseExpr();
        if (at(Tok::SEMI)) ++p_;
        return Stmt::Expr_(e);
    }

    StmtPtr parseIf() {
        ++p_;  // if
        expect(Tok::LPAREN, "(");
        ExprPtr cond = parseExpr();
        expect(Tok::RPAREN, ")");
        if (atWord("then")) ++p_;  // BASIC sugar
        BlockPtr thenB = parseBody();
        BlockPtr elseB;
        if (atWord("else")) {
            ++p_;
            elseB = parseBody();
        } else if (atWord("end")) {  // BASIC "END IF"
            ++p_;
            if (atWord("if")) ++p_;
        }
        return Stmt::If(cond, thenB, elseB);
    }

    StmtPtr parseWhile() {
        ++p_;  // while
        expect(Tok::LPAREN, "(");
        ExprPtr cond = parseExpr();
        expect(Tok::RPAREN, ")");
        BlockPtr body = parseBody();
        return Stmt::While(cond, body);
    }

    BlockPtr parseBody() {
        if (at(Tok::LBRACE)) {
            ++p_;
            BlockPtr b = Block::make();
            while (!at(Tok::RBRACE)) {
                if (at(Tok::END)) throw std::runtime_error("Parser: unterminated block");
                if (at(Tok::SEMI)) { ++p_; continue; }
                b->stmts.push_back(parseStatement());
            }
            ++p_;
            return b;
        }
        BlockPtr b = Block::make();
        b->stmts.push_back(parseStatement());
        return b;
    }

    ExprPtr parseExpr() { return parseBinary(0); }

    static int precedence(const std::string& op) {
        if (op == "||" || op == "or") return 1;
        if (op == "&&" || op == "and") return 2;
        if (op == "|") return 3;
        if (op == "^") return 4;
        if (op == "&") return 5;
        if (op == "==" || op == "!=") return 6;
        if (op == "<" || op == ">" || op == "<=" || op == ">=") return 7;
        if (op == "<<" || op == ">>") return 8;
        if (op == "+" || op == "-") return 9;
        if (op == "*" || op == "/" || op == "%") return 10;
        return -1;
    }

    static BinOp binOpFor(const std::string& op) {
        if (op == "+") return BinOp::ADD;
        if (op == "-") return BinOp::SUB;
        if (op == "*") return BinOp::MUL;
        if (op == "/") return BinOp::DIV;
        if (op == "%") return BinOp::MOD;
        if (op == "<") return BinOp::LT;
        if (op == ">") return BinOp::GT;
        if (op == "<=") return BinOp::LE;
        if (op == ">=") return BinOp::GE;
        if (op == "==") return BinOp::EQ;
        if (op == "!=") return BinOp::NE;
        if (op == "&" || op == "&&" || op == "and") return BinOp::BAND;
        if (op == "|" || op == "||" || op == "or") return BinOp::BOR;
        if (op == "^") return BinOp::BXOR;
        if (op == "<<") return BinOp::SHL;
        return BinOp::SHR;
    }

    ExprPtr parseBinary(int minPrec) {
        ExprPtr left = parseUnary();
        while (true) {
            std::string op;
            if (cur().kind == Tok::OP) op = cur().text;
            else if (cur().kind == Tok::IDENT &&
                     (cur().text == "and" || cur().text == "or")) op = cur().text;
            else break;
            const int prec = precedence(op);
            if (prec < minPrec) break;
            ++p_;
            ExprPtr right = parseBinary(prec + 1);
            left = Expr::BinOp_(binOpFor(op), left, right);
        }
        return left;
    }

    ExprPtr parseUnary() {
        if (at(Tok::OP) && cur().text == "-") {
            ++p_;
            return Expr::UnaryOp_(UnOp::NEG, parseUnary());
        }
        if ((at(Tok::OP) && cur().text == "!") || atWord("not")) {
            ++p_;
            return Expr::UnaryOp_(UnOp::NOT, parseUnary());
        }
        if (at(Tok::OP) && cur().text == "~") {
            ++p_;
            return Expr::UnaryOp_(UnOp::NOT, parseUnary());
        }
        return parsePrimary();
    }

    ExprPtr parsePrimary() {
        if (at(Tok::NUM) || at(Tok::STR)) {
            ExprPtr e = Expr::Const(cur().value);
            ++p_;
            return e;
        }
        if (at(Tok::LPAREN)) {
            ++p_;
            ExprPtr e = parseExpr();
            expect(Tok::RPAREN, ")");
            return e;
        }
        if (at(Tok::IDENT)) {
            const std::string name = cur().text;
            ++p_;
            if (at(Tok::LPAREN)) {
                ++p_;
                std::vector<ExprPtr> args;
                if (!at(Tok::RPAREN)) {
                    while (true) {
                        args.push_back(parseExpr());
                        if (at(Tok::COMMA)) { ++p_; continue; }
                        break;
                    }
                }
                expect(Tok::RPAREN, ")");
                return Expr::Call_(name, args);
            }
            if (name == "true") return Expr::Const(Box(true));
            if (name == "false") return Expr::Const(Box(false));
            if (name == "nil") return Expr::Const(Box());
            return Expr::Var(name);
        }
        throw std::runtime_error("Parser: unexpected token '" + cur().text + "'");
    }

    std::vector<Token> t_;
    std::size_t p_ = 0;
};

Language detectLanguage(const std::string& src) {
    if (src.find("LET ") != std::string::npos || src.find("PRINT ") != std::string::npos ||
        src.find("THEN") != std::string::npos)
        return Language::BASIC;
    if (src.find(" and ") != std::string::npos || src.find(" or ") != std::string::npos ||
        src.find("def ") != std::string::npos)
        return Language::PYTHON;
    return Language::NEBULA;
}

}  // namespace

// --------------------------------------------------------------- Optimizer

namespace {

bool constTruthy(const Box& b) { return b.asBool(); }

// Evaluate a constant binary expression without throwing on divide by zero.
bool foldBinary(BinOp op, const Box& l, const Box& r, Box& out) {
    try {
        switch (op) {
            case BinOp::ADD: out = l + r; return true;
            case BinOp::SUB: out = l - r; return true;
            case BinOp::MUL: out = l * r; return true;
            case BinOp::DIV: out = l / r; return true;
            case BinOp::MOD: out = l % r; return true;
            case BinOp::LT: out = Box(l < r); return true;
            case BinOp::GT: out = Box(r < l); return true;
            case BinOp::LE: out = Box(!(r < l)); return true;
            case BinOp::GE: out = Box(!(l < r)); return true;
            case BinOp::EQ: out = Box(l == r); return true;
            case BinOp::NE: out = Box(!(l == r)); return true;
            case BinOp::BAND: out = l & r; return true;
            case BinOp::BOR: out = l | r; return true;
            case BinOp::BXOR: out = l ^ r; return true;
            case BinOp::SHL:
                out = Box(static_cast<long long>(l.asInt64() << r.asInt64()));
                return true;
            case BinOp::SHR:
                out = Box(static_cast<long long>(l.asInt64() >> r.asInt64()));
                return true;
        }
    } catch (const std::exception&) {
        return false;
    }
    return false;
}

class Optimizer {
public:
    explicit Optimizer(const OptimizeOptions& o) : o_(o) {}

    void run(BlockPtr b) {
        for (std::size_t pass = 0; pass < o_.max_passes; ++pass) {
            changed_ = false;
            block(b);
            if (!changed_) break;
            ++rep_.passes_run;
        }
    }

    const OptimizeReport& report() const { return rep_; }

private:
    // Names assigned anywhere inside a block, including nested blocks. These
    // must not be treated as constants across a branch or a loop back edge.
    static void collectAssigned(const BlockPtr& b, std::vector<std::string>& out) {
        if (!b) return;
        for (const StmtPtr& s : b->stmts) {
            if (s->kind == Stmt::Kind::ASSIGN) out.push_back(s->var);
            if (s->body) collectAssigned(s->body, out);
            if (s->elseBody) collectAssigned(s->elseBody, out);
        }
    }

    static void invalidate(std::unordered_map<std::string, Box>& env,
                           const std::vector<std::string>& names) {
        for (const std::string& n : names) env.erase(n);
    }

    void block(const BlockPtr& b) {
        if (!b) return;
        std::unordered_map<std::string, Box> env;
        std::vector<StmtPtr> kept;
        for (StmtPtr& s : b->stmts) {
            // A loop condition is re-evaluated after the body ran, so any name
            // the body assigns must lose its constant before the condition is
            // touched. Same for names assigned inside a branch.
            std::vector<std::string> branch_assigned;
            if (s->kind == Stmt::Kind::WHILE) {
                collectAssigned(s->body, branch_assigned);
                invalidate(env, branch_assigned);
            } else if (s->kind == Stmt::Kind::IF) {
                collectAssigned(s->body, branch_assigned);
                collectAssigned(s->elseBody, branch_assigned);
                invalidate(env, branch_assigned);
            }

            if (s->expr) s->expr = expr(s->expr, env);
            if (s->kind == Stmt::Kind::ASSIGN && s->expr) {
                if (o_.constant_propagation && s->expr->isConst()) env[s->var] = s->expr->value;
                else env.erase(s->var);
            }

            if (s->kind == Stmt::Kind::IF && s->expr && s->expr->isConst() &&
                o_.dead_code_elimination) {
                const bool t = constTruthy(s->expr->value);
                BlockPtr chosen = t ? s->body : s->elseBody;
                ++rep_.dead_stmts;
                changed_ = true;
                if (chosen) {
                    block(chosen);
                    for (const StmtPtr& c : chosen->stmts) kept.push_back(c);
                }
                invalidate(env, branch_assigned);
                continue;
            }
            if (s->kind == Stmt::Kind::WHILE && s->expr && s->expr->isConst() &&
                !constTruthy(s->expr->value) && o_.dead_code_elimination) {
                ++rep_.dead_stmts;
                changed_ = true;
                continue;
            }

            if (s->body) block(s->body);
            if (s->elseBody) block(s->elseBody);
            if (s->kind == Stmt::Kind::WHILE || s->kind == Stmt::Kind::IF)
                invalidate(env, branch_assigned);
            kept.push_back(s);
        }
        b->stmts.swap(kept);
    }

    ExprPtr expr(const ExprPtr& e, std::unordered_map<std::string, Box>& env) {
        if (!e) return e;
        ExprPtr out = e;
        if (e->lhs) out->lhs = expr(e->lhs, env);
        if (e->rhs) out->rhs = expr(e->rhs, env);
        for (ExprPtr& a : out->args) a = expr(a, env);

        if (out->kind == Expr::Kind::VAR && o_.constant_propagation) {
            auto it = env.find(out->name);
            if (it != env.end()) {
                ++rep_.propagations;
                changed_ = true;
                return Expr::Const(it->second);
            }
        }

        if (out->kind == Expr::Kind::UNARYOP && out->lhs && out->lhs->isConst()) {
            if (!o_.constant_folding) return out;
            const Box& v = out->lhs->value;
            Box folded;
            if (out->unop == UnOp::NEG) folded = -v;
            else folded = Box(!v.asBool());
            ++rep_.folds;
            changed_ = true;
            return Expr::Const(folded);
        }

        if (out->kind != Expr::Kind::BINOP || !out->lhs || !out->rhs) return out;

        if (o_.constant_folding && out->lhs->isConst() && out->rhs->isConst()) {
            Box folded;
            if (foldBinary(out->binop, out->lhs->value, out->rhs->value, folded)) {
                ++rep_.folds;
                changed_ = true;
                return Expr::Const(folded);
            }
            return out;
        }

        if (o_.algebraic_simplification) {
            const bool lc = out->lhs->isConst();
            const bool rc = out->rhs->isConst();
            if (out->binop == BinOp::ADD && rc && out->rhs->value.asDouble() == 0.0) {
                ++rep_.algebraic;
                changed_ = true;
                return out->lhs;
            }
            if (out->binop == BinOp::MUL && rc && out->rhs->value.asDouble() == 1.0) {
                ++rep_.algebraic;
                changed_ = true;
                return out->lhs;
            }
            if (out->binop == BinOp::MUL && (lc || rc)) {
                const Box& k = lc ? out->lhs->value : out->rhs->value;
                if (k.asDouble() == 0.0) {
                    ++rep_.algebraic;
                    changed_ = true;
                    return Expr::Const(Box(static_cast<long long>(0)));
                }
            }
            if (out->binop == BinOp::SUB && rc && out->rhs->value.asDouble() == 0.0) {
                ++rep_.algebraic;
                changed_ = true;
                return out->lhs;
            }
        }

        if (o_.strength_reduction) {
            // x * 2^n  ->  x << n,  x / 2^n  ->  x >> n
            const bool lc = out->lhs->isConst();
            const bool rc = out->rhs->isConst();
            if ((out->binop == BinOp::MUL || out->binop == BinOp::DIV) && rc &&
                out->rhs->value.isInteger()) {
                const long long k = out->rhs->value.asInt64();
                int shift = -1;
                if (k > 1 && (k & (k - 1)) == 0) {
                    shift = 0;
                    for (long long v = k; v > 1; v >>= 1) ++shift;
                }
                if (shift >= 0) {
                    ++rep_.strength_reductions;
                    changed_ = true;
                    const BinOp op = out->binop == BinOp::MUL ? BinOp::SHL : BinOp::SHR;
                    return Expr::BinOp_(op, out->lhs,
                                        Expr::Const(Box(static_cast<long long>(shift))));
                }
            }
            (void)lc;
        }
        return out;
    }

    OptimizeOptions o_;
    OptimizeReport rep_;
    bool changed_ = false;
};

// ----------------------------------------------------------- Bytecode defs

enum Op : std::int32_t {
    OP_PUSH = 1, OP_LOAD, OP_STORE, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NE, OP_BAND, OP_BOR, OP_BXOR,
    OP_SHL, OP_SHR, OP_NEG, OP_NOT, OP_CALL, OP_JMP, OP_JZ, OP_POP, OP_RET,
    OP_HALT
};

const char* opName(std::int32_t op) {
    switch (op) {
        case OP_PUSH: return "PUSH";
        case OP_LOAD: return "LOAD";
        case OP_STORE: return "STORE";
        case OP_ADD: return "ADD";
        case OP_SUB: return "SUB";
        case OP_MUL: return "MUL";
        case OP_DIV: return "DIV";
        case OP_MOD: return "MOD";
        case OP_LT: return "LT";
        case OP_GT: return "GT";
        case OP_LE: return "LE";
        case OP_GE: return "GE";
        case OP_EQ: return "EQ";
        case OP_NE: return "NE";
        case OP_BAND: return "BAND";
        case OP_BOR: return "BOR";
        case OP_BXOR: return "BXOR";
        case OP_SHL: return "SHL";
        case OP_SHR: return "SHR";
        case OP_NEG: return "NEG";
        case OP_NOT: return "NOT";
        case OP_CALL: return "CALL";
        case OP_JMP: return "JMP";
        case OP_JZ: return "JZ";
        case OP_POP: return "POP";
        case OP_RET: return "RET";
        case OP_HALT: return "HALT";
        default: return "?";
    }
}

bool hasOperand(std::int32_t op) {
    return op == OP_PUSH || op == OP_LOAD || op == OP_STORE ||
           op == OP_JMP || op == OP_JZ;
}

std::int32_t opForBinOp(BinOp op) {
    switch (op) {
        case BinOp::ADD: return OP_ADD;
        case BinOp::SUB: return OP_SUB;
        case BinOp::MUL: return OP_MUL;
        case BinOp::DIV: return OP_DIV;
        case BinOp::MOD: return OP_MOD;
        case BinOp::LT: return OP_LT;
        case BinOp::GT: return OP_GT;
        case BinOp::LE: return OP_LE;
        case BinOp::GE: return OP_GE;
        case BinOp::EQ: return OP_EQ;
        case BinOp::NE: return OP_NE;
        case BinOp::BAND: return OP_BAND;
        case BinOp::BOR: return OP_BOR;
        case BinOp::BXOR: return OP_BXOR;
        case BinOp::SHL: return OP_SHL;
        case BinOp::SHR: return OP_SHR;
    }
    return OP_HALT;
}

class CodeGen {
public:
    CodeGen(std::vector<std::int32_t>& code, std::vector<Box>& consts,
            std::vector<std::string>& names)
        : code_(code), consts_(consts), names_(names) {}

    void program(const BlockPtr& b) {
        if (!b) { emit(OP_HALT); return; }
        // The final statement, when it is a bare expression, leaves its value
        // on the stack as the program result instead of discarding it.
        const std::size_t n = b->stmts.size();
        for (std::size_t i = 0; i < n; ++i) {
            const StmtPtr& st = b->stmts[i];
            const bool last = (i + 1 == n);
            if (last && st->kind == Stmt::Kind::EXPR) {
                expr(st->expr);
            } else {
                stmt(st);
            }
        }
        emit(OP_HALT);
    }

private:
    void emit(std::int32_t op) { code_.push_back(op); }
    void emit(std::int32_t op, std::int32_t operand) {
        code_.push_back(op);
        code_.push_back(operand);
    }
    std::int32_t here() const { return static_cast<std::int32_t>(code_.size()); }
    void patch(std::int32_t at, std::int32_t target) { code_[static_cast<std::size_t>(at)] = target; }

    std::int32_t nameSlot(const std::string& n) {
        for (std::size_t i = 0; i < names_.size(); ++i)
            if (names_[i] == n) return static_cast<std::int32_t>(i);
        names_.push_back(n);
        return static_cast<std::int32_t>(names_.size() - 1);
    }

    std::int32_t constSlot(const Box& v) {
        consts_.push_back(v);
        return static_cast<std::int32_t>(consts_.size() - 1);
    }

    void block(const BlockPtr& b) {
        if (!b) return;
        for (const StmtPtr& s : b->stmts) stmt(s);
    }

    void stmt(const StmtPtr& s) {
        switch (s->kind) {
            case Stmt::Kind::EXPR:
                expr(s->expr);
                emit(OP_POP);
                break;
            case Stmt::Kind::ASSIGN:
                expr(s->expr);
                emit(OP_STORE, nameSlot(s->var));
                break;
            case Stmt::Kind::RETURN:
                if (s->expr) expr(s->expr);
                else emit(OP_PUSH, constSlot(Box()));
                emit(OP_RET);
                break;
            case Stmt::Kind::IF: {
                expr(s->expr);
                const std::int32_t jz = here();
                emit(OP_JZ, 0);
                block(s->body);
                if (s->elseBody) {
                    const std::int32_t jmp = here();
                    emit(OP_JMP, 0);
                    patch(jz + 1, here());
                    block(s->elseBody);
                    patch(jmp + 1, here());
                } else {
                    patch(jz + 1, here());
                }
                break;
            }
            case Stmt::Kind::WHILE: {
                const std::int32_t start = here();
                expr(s->expr);
                const std::int32_t jz = here();
                emit(OP_JZ, 0);
                block(s->body);
                emit(OP_JMP, start);
                patch(jz + 1, here());
                break;
            }
            case Stmt::Kind::BLOCK:
                block(s->body);
                break;
        }
    }

    void expr(const ExprPtr& e) {
        if (!e) { emit(OP_PUSH, constSlot(Box())); return; }
        switch (e->kind) {
            case Expr::Kind::CONST: emit(OP_PUSH, constSlot(e->value)); break;
            case Expr::Kind::VAR: emit(OP_LOAD, nameSlot(e->name)); break;
            case Expr::Kind::BINOP:
                expr(e->lhs);
                expr(e->rhs);
                emit(opForBinOp(e->binop));
                break;
            case Expr::Kind::UNARYOP:
                expr(e->lhs);
                emit(e->unop == UnOp::NEG ? OP_NEG : OP_NOT);
                break;
            case Expr::Kind::CALL:
                for (const ExprPtr& a : e->args) expr(a);
                emit(OP_CALL, nameSlot(e->name));
                emit(static_cast<std::int32_t>(e->args.size()));
                break;
        }
    }

    std::vector<std::int32_t>& code_;
    std::vector<Box>& consts_;
    std::vector<std::string>& names_;
};

Box applyBinary(BinOp op, const Box& l, const Box& r) {
    switch (op) {
        case BinOp::ADD: return l + r;
        case BinOp::SUB: return l - r;
        case BinOp::MUL: return l * r;
        case BinOp::DIV: return l / r;
        case BinOp::MOD: return l % r;
        case BinOp::LT: return Box(l < r);
        case BinOp::GT: return Box(r < l);
        case BinOp::LE: return Box(!(r < l));
        case BinOp::GE: return Box(!(l < r));
        case BinOp::EQ: return Box(l == r);
        case BinOp::NE: return Box(!(l == r));
        case BinOp::BAND: return l & r;
        case BinOp::BOR: return l | r;
        case BinOp::BXOR: return l ^ r;
        case BinOp::SHL: return Box(static_cast<long long>(l.asInt64() << r.asInt64()));
        case BinOp::SHR: return Box(static_cast<long long>(l.asInt64() >> r.asInt64()));
    }
    return Box();
}

// AST tier: direct tree walk.
class AstEvaluator {
public:
    AstEvaluator(std::unordered_map<std::string, Box>& g,
                 std::unordered_map<std::string, std::function<Box(std::vector<Box>&)>>& b,
                 std::size_t limit)
        : globals_(g), builtins_(b), limit_(limit) {}

    Box run(const BlockPtr& prog) {
        Box result;
        execBlock(prog, result);
        return result;
    }

    std::size_t steps() const { return steps_; }

private:
    bool execBlock(const BlockPtr& b, Box& result) {
        if (!b) return false;
        for (const StmtPtr& s : b->stmts) {
            if (exec(s, result)) return true;
        }
        return false;
    }

    bool exec(const StmtPtr& s, Box& result) {
        switch (s->kind) {
            case Stmt::Kind::EXPR:
                result = eval(s->expr);
                return false;
            case Stmt::Kind::ASSIGN:
                globals_[s->var] = eval(s->expr);
                return false;
            case Stmt::Kind::IF:
                if (eval(s->expr).asBool()) return execBlock(s->body, result);
                if (s->elseBody) return execBlock(s->elseBody, result);
                return false;
            case Stmt::Kind::WHILE:
                while (eval(s->expr).asBool()) {
                    if (execBlock(s->body, result)) return true;
                }
                return false;
            case Stmt::Kind::RETURN:
                result = s->expr ? eval(s->expr) : Box();
                return true;
            case Stmt::Kind::BLOCK:
                return execBlock(s->body, result);
        }
        return false;
    }

    Box eval(const ExprPtr& e) {
        if (++steps_ > limit_) throw std::runtime_error("JIT: AST step limit exceeded");
        switch (e->kind) {
            case Expr::Kind::CONST: return e->value;
            case Expr::Kind::VAR: {
                auto it = globals_.find(e->name);
                if (it != globals_.end()) return it->second;
                auto b = builtins_.find(e->name);
                if (b != builtins_.end()) return Box(e->name);
                throw std::runtime_error("JIT: unknown variable '" + e->name + "'");
            }
            case Expr::Kind::BINOP: {
                const Box l = eval(e->lhs);
                const Box r = eval(e->rhs);
                return applyBin(e->binop, l, r);
            }
            case Expr::Kind::UNARYOP: {
                const Box v = eval(e->lhs);
                return e->unop == UnOp::NEG ? -v : Box(!v.asBool());
            }
            case Expr::Kind::CALL: {
                std::vector<Box> args;
                for (const ExprPtr& a : e->args) args.push_back(eval(a));
                auto it = builtins_.find(e->name);
                if (it == builtins_.end())
                    throw std::runtime_error("JIT: unknown function '" + e->name + "'");
                return it->second(args);
            }
        }
        return Box();
    }

    static Box applyBin(BinOp op, const Box& l, const Box& r) {
        return applyBinary(op, l, r);
    }

    std::unordered_map<std::string, Box>& globals_;
    std::unordered_map<std::string, std::function<Box(std::vector<Box>&)>>& builtins_;
    std::size_t limit_;
    std::size_t steps_ = 0;
};

}  // namespace

// --------------------------------------------------------------- JIT class

JITCompiler::Closure::Closure(std::string name, Tier tier)
    : name_(std::move(name)), tier_(tier) {}

void JITCompiler::Closure::setCode(std::vector<std::int32_t> code,
                                   std::vector<Box> consts,
                                   std::vector<std::string> names,
                                   std::vector<std::int32_t> lines) {
    code_ = std::move(code);
    consts_ = std::move(consts);
    names_ = std::move(names);
    lines_ = std::move(lines);
}

std::vector<std::string> JITCompiler::Closure::disassemble() const {
    std::vector<std::string> out;
    std::size_t i = 0;
    while (i < code_.size()) {
        const std::int32_t op = code_[i];
        std::string line = std::to_string(i) + "  " + opName(op);
        if (op == OP_CALL && i + 2 < code_.size()) {
            const std::int32_t a = code_[i + 1];
            line += " " + (a >= 0 && static_cast<std::size_t>(a) < names_.size()
                               ? names_[static_cast<std::size_t>(a)]
                               : std::to_string(a));
            line += " argc=" + std::to_string(code_[i + 2]);
            i += 3;
        } else if (hasOperand(op) && i + 1 < code_.size()) {
            const std::int32_t a = code_[i + 1];
            if (op == OP_PUSH && a >= 0 && static_cast<std::size_t>(a) < consts_.size())
                line += " " + consts_[static_cast<std::size_t>(a)].asString();
            else if (op == OP_LOAD && a >= 0 && static_cast<std::size_t>(a) < names_.size())
                line += " " + names_[static_cast<std::size_t>(a)];
            else if (op == OP_STORE && a >= 0 && static_cast<std::size_t>(a) < names_.size())
                line += " " + names_[static_cast<std::size_t>(a)];
            else if (op == OP_CALL && a >= 0 && static_cast<std::size_t>(a) < names_.size())
                line += " " + names_[static_cast<std::size_t>(a)];
            else
                line += " " + std::to_string(a);
            i += 2;
        } else {
            ++i;
        }
        out.push_back(line);
    }
    return out;
}

VMResult JITCompiler::Closure::run(std::size_t stack_limit) {
    std::vector<Box> stack;
    std::vector<Box> locals(names_.size());
    steps_ = 0;
    std::size_t pc = 0;

    auto pop = [&]() -> Box {
        if (stack.empty()) throw std::runtime_error("JIT VM: stack underflow");
        Box v = stack.back();
        stack.pop_back();
        return v;
    };

    while (pc < code_.size()) {
        if (++steps_ > stack_limit * 64)
            throw std::runtime_error("JIT VM: step limit exceeded");
        const std::int32_t op = code_[pc++];
        switch (op) {
            case OP_PUSH: {
                const std::int32_t a = code_[pc++];
                stack.push_back(consts_[static_cast<std::size_t>(a)]);
                break;
            }
            case OP_LOAD: {
                const std::int32_t a = code_[pc++];
                if (globals_ && globals_->count(names_[static_cast<std::size_t>(a)])) {
                    stack.push_back((*globals_)[names_[static_cast<std::size_t>(a)]]);
                } else {
                    stack.push_back(locals[static_cast<std::size_t>(a)]);
                }
                break;
            }
            case OP_STORE: {
                const std::int32_t a = code_[pc++];
                const std::string& n = names_[static_cast<std::size_t>(a)];
                Box v = pop();
                locals[static_cast<std::size_t>(a)] = v;
                if (globals_) (*globals_)[n] = v;
                break;
            }
            case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_MOD:
            case OP_LT: case OP_GT: case OP_LE: case OP_GE: case OP_EQ: case OP_NE:
            case OP_BAND: case OP_BOR: case OP_BXOR: case OP_SHL: case OP_SHR: {
                const Box r = pop();
                const Box l = pop();
                BinOp bop = BinOp::ADD;
                switch (op) {
                    case OP_ADD: bop = BinOp::ADD; break;
                    case OP_SUB: bop = BinOp::SUB; break;
                    case OP_MUL: bop = BinOp::MUL; break;
                    case OP_DIV: bop = BinOp::DIV; break;
                    case OP_MOD: bop = BinOp::MOD; break;
                    case OP_LT: bop = BinOp::LT; break;
                    case OP_GT: bop = BinOp::GT; break;
                    case OP_LE: bop = BinOp::LE; break;
                    case OP_GE: bop = BinOp::GE; break;
                    case OP_EQ: bop = BinOp::EQ; break;
                    case OP_NE: bop = BinOp::NE; break;
                    case OP_BAND: bop = BinOp::BAND; break;
                    case OP_BOR: bop = BinOp::BOR; break;
                    case OP_BXOR: bop = BinOp::BXOR; break;
                    case OP_SHL: bop = BinOp::SHL; break;
                    default: bop = BinOp::SHR; break;
                }
                stack.push_back(applyBinary(bop, l, r));
                break;
            }
            case OP_NEG: { Box v = pop(); stack.push_back(-v); break; }
            case OP_NOT: { Box v = pop(); stack.push_back(Box(!v.asBool())); break; }
            case OP_JMP: pc = static_cast<std::size_t>(code_[pc]); break;
            case OP_JZ: {
                const std::int32_t target = code_[pc++];
                const Box c = pop();
                if (!c.asBool()) pc = static_cast<std::size_t>(target);
                break;
            }
            case OP_CALL: {
                const std::int32_t a = code_[pc++];
                const std::int32_t argc = code_[pc++];
                std::vector<Box> args(static_cast<std::size_t>(argc));
                for (std::int32_t k = argc - 1; k >= 0; --k) args[static_cast<std::size_t>(k)] = pop();
                const std::string& fname = names_[static_cast<std::size_t>(a)];
                if (builtins_ == nullptr) throw std::runtime_error("JIT VM: no builtin table");
                auto it = builtins_->find(fname);
                if (it == builtins_->end())
                    throw std::runtime_error("JIT VM: unknown function '" + fname + "'");
                stack.push_back(it->second(args));
                break;
            }
            case OP_POP: (void)pop(); break;
            case OP_RET: {
                const Box v = pop();
                return VMResult{v, steps_};
            }
            case OP_HALT: {
                Box v = stack.empty() ? Box() : stack.back();
                return VMResult{v, steps_};
            }
            default:
                throw std::runtime_error("JIT VM: bad opcode " + std::to_string(op));
        }
        if (stack.size() > stack_limit)
            throw std::runtime_error("JIT VM: stack limit exceeded");
    }
    return VMResult{stack.empty() ? Box() : stack.back(), steps_};
}

Box JITCompiler::Closure::call(const std::vector<Box>& args) {
    if (!args.empty()) throw std::runtime_error("JIT: program takes no arguments");
    const VMResult r = run(4096);
    return r.value;
}

JITCompiler::JITCompiler(JITOptions opt) : opt_(opt), program_(Block::make()) {}

void JITCompiler::addStatement(StmtPtr stmt) { program_->stmts.push_back(std::move(stmt)); }

void JITCompiler::addExpression(ExprPtr expr) {
    program_->stmts.push_back(Stmt::Expr_(std::move(expr)));
}

void JITCompiler::parseSource(const std::string& source, Language lang) {
    if (lang == Language::AUTO) lang = detectLanguage(source);
    language_ = lang;
    Parser p(Lexer(source).run());
    BlockPtr b = p.parseProgram();
    for (const StmtPtr& s : b->stmts) program_->stmts.push_back(s);
}

ExprPtr JITCompiler::optimize(ExprPtr expr) {
    BlockPtr tmp = Block::make();
    tmp->stmts.push_back(Stmt::Expr_(expr));
    Optimizer o(opt_.optimize);
    o.run(tmp);
    const OptimizeReport& r = o.report();
    stats_.opt.folds += r.folds;
    stats_.opt.propagations += r.propagations;
    stats_.opt.dead_stmts += r.dead_stmts;
    stats_.opt.strength_reductions += r.strength_reductions;
    stats_.opt.algebraic += r.algebraic;
    return tmp->stmts.front()->expr;
}

void JITCompiler::setGlobal(const std::string& name, const Box& value) {
    globals_[name] = value;
}

Box JITCompiler::getGlobal(const std::string& name) const {
    auto it = globals_.find(name);
    return it == globals_.end() ? Box() : it->second;
}

void JITCompiler::registerFunction(const std::string& name,
                                   std::function<Box(std::vector<Box>&)> fn) {
    builtins_[name] = std::move(fn);
}

std::shared_ptr<JITCompiler::Closure> JITCompiler::compile() {
    BlockPtr work = Block::make();
    for (const StmtPtr& s : program_->stmts) work->stmts.push_back(s->clone());

    stats_.ast_nodes = 0;
    for (const StmtPtr& s : work->stmts)
        if (s->expr) stats_.ast_nodes += s->expr->nodeCount();

    if (opt_.tier != Tier::AST && opt_.tier != Tier::BYTECODE) {
        Optimizer o(opt_.optimize);
        o.run(work);
        stats_.opt = o.report();
    }

    if (opt_.trace) {
        std::printf("[jit] tier=%s ast_nodes=%zu folds=%zu strength=%zu dead=%zu\n",
                    describeTier(opt_.tier).c_str(), stats_.ast_nodes,
                    stats_.opt.folds, stats_.opt.strength_reductions,
                    stats_.opt.dead_stmts);
    }

    closure_ = std::make_shared<Closure>("main", opt_.tier);
    closure_->bindGlobals(&globals_);
    closure_->bindBuiltins(&builtins_);

    if (opt_.tier != Tier::AST) {
        std::vector<std::int32_t> code;
        std::vector<Box> consts;
        std::vector<std::string> names;
        CodeGen gen(code, consts, names);
        gen.program(work);
        stats_.bytecode_ops = code.size();
        closure_->setCode(std::move(code), std::move(consts), std::move(names), {});
    } else {
        stats_.bytecode_ops = 0;
    }

    if (opt_.trace) {
        for (const std::string& line : closure_->disassemble())
            std::printf("[jit] %s\n", line.c_str());
    }
    ++stats_.closures_built;
    return closure_;
}

Box JITCompiler::execute() {
    if (!closure_) compile();
    ++stats_.calls;
    if (opt_.tier == Tier::AST) {
        AstEvaluator ev(globals_, builtins_, opt_.stack_limit);
        BlockPtr work = Block::make();
        for (const StmtPtr& s : program_->stmts) work->stmts.push_back(s->clone());
        Box r = ev.run(work);
        stats_.vm_steps = ev.steps();
        return r;
    }
    const VMResult r = closure_->run(opt_.stack_limit);
    stats_.vm_steps = r.steps;
    return r.value;
}

Box JITCompiler::call(const std::string& fn, const std::vector<Box>& args) {
    auto it = builtins_.find(fn);
    if (it == builtins_.end())
        throw std::runtime_error("JIT: unknown function '" + fn + "'");
    std::vector<Box> copy = args;
    ++stats_.calls;
    return it->second(copy);
}

std::vector<std::string> JITCompiler::disassemble() const {
    return closure_ ? closure_->disassemble() : std::vector<std::string>();
}

ExprPtr parseExpression(const std::string& source) {
    Parser p(Lexer(source).run());
    return p.parseExpressionOnly();
}

std::string describeTier(Tier t) {
    switch (t) {
        case Tier::AST: return "AST (tree walk)";
        case Tier::BYTECODE: return "BYTECODE (unoptimized)";
        case Tier::OPTIMIZED: return "OPTIMIZED (folded bytecode)";
    }
    return "?";
}

}  // namespace nebula

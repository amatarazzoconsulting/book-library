// MIT License - (c) Anthony Matarazzo 2026
//
// ast.cpp - AST construction, printing, evaluation and a small infix parser.

#include "ast.hpp"

#include <cctype>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace jitx {

const char* opName(Op op) {
    switch (op) {
        case Op::ADD: return "+";
        case Op::SUB: return "-";
        case Op::MUL: return "*";
        case Op::DIV: return "/";
        case Op::MOD: return "%";
        case Op::AND: return "&";
        case Op::OR: return "|";
        case Op::XOR: return "^";
        case Op::SHL: return "<<";
        case Op::SHR: return ">>";
        case Op::EQ: return "==";
        case Op::NE: return "!=";
        case Op::LT: return "<";
        case Op::LE: return "<=";
        case Op::GT: return ">";
        case Op::GE: return ">=";
        case Op::LAND: return "&&";
        case Op::LOR: return "||";
    }
    return "?";
}

const char* unOpName(UnOp op) {
    switch (op) {
        case UnOp::NEG: return "-";
        case UnOp::NOT: return "!";
        case UnOp::BNOT: return "~";
    }
    return "?";
}

//---------------------------------------------------------------------------
// Factories
//---------------------------------------------------------------------------

NodePtr Node::makeConst(const Box& v) {
    auto n = std::make_shared<Node>();
    n->kind = Kind::CONST;
    n->value = v;
    return n;
}

NodePtr Node::makeVar(const std::string& nm) {
    auto n = std::make_shared<Node>();
    n->kind = Kind::VAR;
    n->name = nm;
    return n;
}

NodePtr Node::makeBin(Op o, NodePtr l, NodePtr r) {
    auto n = std::make_shared<Node>();
    n->kind = Kind::BIN;
    n->op = o;
    n->a = std::move(l);
    n->b = std::move(r);
    return n;
}

NodePtr Node::makeUnary(UnOp o, NodePtr v) {
    auto n = std::make_shared<Node>();
    n->kind = Kind::UNARY;
    n->unop = o;
    n->a = std::move(v);
    return n;
}

NodePtr Node::makeTernary(NodePtr cond, NodePtr t, NodePtr f) {
    auto n = std::make_shared<Node>();
    n->kind = Kind::TERNARY;
    n->a = std::move(cond);
    n->b = std::move(t);
    n->c = std::move(f);
    return n;
}

NodePtr Node::makeCall(const std::string& fn, std::vector<NodePtr> args) {
    auto n = std::make_shared<Node>();
    n->kind = Kind::CALL;
    n->name = fn;
    n->args = std::move(args);
    return n;
}

NodePtr Const(const Box& v) { return Node::makeConst(v); }
NodePtr Var(const std::string& n) { return Node::makeVar(n); }
NodePtr Bin(Op op, NodePtr l, NodePtr r) { return Node::makeBin(op, l, r); }
NodePtr Un(UnOp op, NodePtr v) { return Node::makeUnary(op, v); }
NodePtr Ternary(NodePtr c, NodePtr t, NodePtr f) {
    return Node::makeTernary(c, t, f);
}
NodePtr Call(const std::string& fn, std::vector<NodePtr> args) {
    return Node::makeCall(fn, args);
}

NodePtr Add(NodePtr l, NodePtr r) { return Bin(Op::ADD, std::move(l), std::move(r)); }
NodePtr Sub(NodePtr l, NodePtr r) { return Bin(Op::SUB, std::move(l), std::move(r)); }
NodePtr Mul(NodePtr l, NodePtr r) { return Bin(Op::MUL, std::move(l), std::move(r)); }
NodePtr Div(NodePtr l, NodePtr r) { return Bin(Op::DIV, std::move(l), std::move(r)); }
NodePtr Neg(NodePtr v) { return Un(UnOp::NEG, std::move(v)); }

//---------------------------------------------------------------------------
// Printing
//---------------------------------------------------------------------------

std::string print(const NodePtr& n) {
    if (!n) return "<null>";
    std::ostringstream os;
    switch (n->kind) {
        case Node::Kind::CONST:
            if (n->value.type() == Box::Type::STRING)
                os << '"' << n->value.toString() << '"';
            else
                os << n->value.toString();
            break;
        case Node::Kind::VAR:
            os << n->name;
            break;
        case Node::Kind::BIN:
            os << '(' << opName(n->op) << ' ' << print(n->a) << ' '
               << print(n->b) << ')';
            break;
        case Node::Kind::UNARY:
            os << '(' << unOpName(n->unop) << ' ' << print(n->a) << ')';
            break;
        case Node::Kind::TERNARY:
            os << "(?: " << print(n->a) << ' ' << print(n->b) << ' '
               << print(n->c) << ')';
            break;
        case Node::Kind::CALL: {
            os << '(' << n->name;
            for (const auto& arg : n->args) os << ' ' << print(arg);
            os << ')';
            break;
        }
    }
    return os.str();
}

std::string toInfix(const NodePtr& n) {
    if (!n) return "<null>";
    std::ostringstream os;
    switch (n->kind) {
        case Node::Kind::CONST:
            return print(n);
        case Node::Kind::VAR:
            return n->name;
        case Node::Kind::BIN:
            os << '(' << toInfix(n->a) << ' ' << opName(n->op) << ' '
               << toInfix(n->b) << ')';
            return os.str();
        case Node::Kind::UNARY:
            os << '(' << unOpName(n->unop) << toInfix(n->a) << ')';
            return os.str();
        case Node::Kind::TERNARY:
            os << '(' << toInfix(n->a) << " ? " << toInfix(n->b) << " : "
               << toInfix(n->c) << ')';
            return os.str();
        case Node::Kind::CALL: {
            os << n->name << '(';
            for (size_t i = 0; i < n->args.size(); ++i) {
                if (i) os << ", ";
                os << toInfix(n->args[i]);
            }
            os << ')';
            return os.str();
        }
    }
    return os.str();
}

size_t nodeCount(const NodePtr& n) {
    if (!n) return 0;
    size_t total = 1;
    total += nodeCount(n->a);
    total += nodeCount(n->b);
    total += nodeCount(n->c);
    for (const auto& arg : n->args) total += nodeCount(arg);
    return total;
}

bool sameTree(const NodePtr& x, const NodePtr& y) {
    if (!x && !y) return true;
    if (!x || !y) return false;
    if (x->kind != y->kind) return false;
    switch (x->kind) {
        case Node::Kind::CONST:
            return x->value == y->value && x->value.type() == y->value.type();
        case Node::Kind::VAR:
            return x->name == y->name;
        case Node::Kind::BIN:
            return x->op == y->op && sameTree(x->a, y->a) && sameTree(x->b, y->b);
        case Node::Kind::UNARY:
            return x->unop == y->unop && sameTree(x->a, y->a);
        case Node::Kind::TERNARY:
            return sameTree(x->a, y->a) && sameTree(x->b, y->b) &&
                   sameTree(x->c, y->c);
        case Node::Kind::CALL: {
            if (x->name != y->name || x->args.size() != y->args.size())
                return false;
            for (size_t i = 0; i < x->args.size(); ++i)
                if (!sameTree(x->args[i], y->args[i])) return false;
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------
// Evaluation
//---------------------------------------------------------------------------

Box callBuiltin(const std::string& name, std::vector<Box>& a) {
    if (name == "abs" && a.size() == 1) return Box(std::fabs(a[0].toDouble()));
    if (name == "min" && a.size() == 2) return a[0] < a[1] ? a[0] : a[1];
    if (name == "max" && a.size() == 2) return a[0] > a[1] ? a[0] : a[1];
    if (name == "sqrt" && a.size() == 1) return Box(std::sqrt(a[0].toDouble()));
    if (name == "len" && a.size() == 1)
        return Box(static_cast<int64_t>(a[0].size()));
    if (name == "clamp" && a.size() == 3) {
        double v = a[0].toDouble(), lo = a[1].toDouble(), hi = a[2].toDouble();
        return Box(v < lo ? lo : (v > hi ? hi : v));
    }
    return Box();  // unknown call yields VOID rather than throwing
}

static Box evalCall(const NodePtr& n, const Env& env) {
    std::vector<Box> a;
    a.reserve(n->args.size());
    for (const auto& arg : n->args) a.push_back(eval(arg, env));
    return callBuiltin(n->name, a);
}

Box eval(const NodePtr& n, const Env& env) {
    if (!n) return Box();
    switch (n->kind) {
        case Node::Kind::CONST:
            return n->value;
        case Node::Kind::VAR: {
            auto it = env.find(n->name);
            return it == env.end() ? Box() : it->second;
        }
        case Node::Kind::UNARY: {
            Box v = eval(n->a, env);
            switch (n->unop) {
                case UnOp::NEG: return Box(-v.toDouble());
                case UnOp::NOT: return Box(!v.toBool());
                case UnOp::BNOT: return ~v;
            }
            return Box();
        }
        case Node::Kind::TERNARY:
            return eval(n->a, env).toBool() ? eval(n->b, env) : eval(n->c, env);
        case Node::Kind::CALL:
            return evalCall(n, env);
        case Node::Kind::BIN: {
            // Short circuit: the deciding operand is the result, which is
            // what the bytecode and closure backends also return.
            if (n->op == Op::LAND) {
                Box l = eval(n->a, env);
                return l.toBool() ? eval(n->b, env) : l;
            }
            if (n->op == Op::LOR) {
                Box l = eval(n->a, env);
                return l.toBool() ? l : eval(n->b, env);
            }
            Box l = eval(n->a, env);
            Box r = eval(n->b, env);
            switch (n->op) {
                case Op::ADD: return l + r;
                case Op::SUB: return l - r;
                case Op::MUL: return l * r;
                case Op::DIV: return l / r;
                case Op::MOD: return l % r;
                case Op::AND: return l & r;
                case Op::OR: return l | r;
                case Op::XOR: return l ^ r;
                case Op::SHL: return l << static_cast<int>(r.toInt64());
                case Op::SHR: return l >> static_cast<int>(r.toInt64());
                case Op::EQ: return Box(l == r);
                case Op::NE: return Box(l != r);
                case Op::LT: return Box(l < r);
                case Op::LE: return Box(l <= r);
                case Op::GT: return Box(l > r);
                case Op::GE: return Box(l >= r);
                default: return Box();
            }
        }
    }
    return Box();
}

//---------------------------------------------------------------------------
// Parser
//---------------------------------------------------------------------------

namespace {

class Parser {
public:
    explicit Parser(const std::string& s) : src_(s), pos_(0) {}

    NodePtr parseAll() {
        NodePtr n = parseExpr();
        skip();
        if (pos_ != src_.size())
            throw std::runtime_error("trailing input at position " +
                                     std::to_string(pos_));
        return n;
    }

private:
    const std::string& src_;
    size_t pos_;

    void skip() {
        while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_])))
            ++pos_;
    }
    char peek() { skip(); return pos_ < src_.size() ? src_[pos_] : '\0'; }
    bool eat(char c) {
        skip();
        if (pos_ < src_.size() && src_[pos_] == c) { ++pos_; return true; }
        return false;
    }
    bool eatStr(const char* s) {
        skip();
        size_t n = std::char_traits<char>::length(s);
        if (src_.compare(pos_, n, s) == 0) { pos_ += n; return true; }
        return false;
    }

    NodePtr parseExpr() {
        NodePtr left = parseTerm();
        for (;;) {
            if (eatStr("<=")) left = Bin(Op::LE, left, parseTerm());
            else if (eatStr(">=")) left = Bin(Op::GE, left, parseTerm());
            else if (eatStr("==")) left = Bin(Op::EQ, left, parseTerm());
            else if (eatStr("!=")) left = Bin(Op::NE, left, parseTerm());
            else if (peek() == '<' && !(pos_ + 1 < src_.size() && src_[pos_ + 1] == '<')) {
                ++pos_; left = Bin(Op::LT, left, parseTerm());
            } else if (peek() == '>' && !(pos_ + 1 < src_.size() && src_[pos_ + 1] == '>')) {
                ++pos_; left = Bin(Op::GT, left, parseTerm());
            } else if (eat('+')) left = Bin(Op::ADD, left, parseTerm());
            else if (eat('-')) left = Bin(Op::SUB, left, parseTerm());
            else break;
        }
        return left;
    }

    NodePtr parseTerm() {
        NodePtr left = parseUnary();
        for (;;) {
            if (eat('*')) left = Bin(Op::MUL, left, parseUnary());
            else if (eat('/')) left = Bin(Op::DIV, left, parseUnary());
            else if (eat('%')) left = Bin(Op::MOD, left, parseUnary());
            else break;
        }
        return left;
    }

    NodePtr parseUnary() {
        if (eat('-')) return Neg(parseUnary());
        if (eat('!')) return Un(UnOp::NOT, parseUnary());
        if (eat('~')) return Un(UnOp::BNOT, parseUnary());
        return parsePrimary();
    }

    NodePtr parsePrimary() {
        skip();
        if (eat('(')) {
            NodePtr n = parseExpr();
            if (!eat(')')) throw std::runtime_error("expected ')'");
            return n;
        }
        if (pos_ < src_.size() && std::isalpha(static_cast<unsigned char>(src_[pos_]))) {
            std::string id;
            while (pos_ < src_.size() &&
                   (std::isalnum(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '_'))
                id += src_[pos_++];
            if (eat('(')) {
                std::vector<NodePtr> args;
                if (!eat(')')) {
                    do { args.push_back(parseExpr()); } while (eat(','));
                    if (!eat(')')) throw std::runtime_error("expected ')' after args");
                }
                return Call(id, args);
            }
            return Var(id);
        }
        if (pos_ < src_.size() && (std::isdigit(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '.')) {
            size_t start = pos_;
            bool isFloat = false;
            while (pos_ < src_.size() &&
                   (std::isdigit(static_cast<unsigned char>(src_[pos_])) ||
                    src_[pos_] == '.' || src_[pos_] == 'e' || src_[pos_] == 'E' ||
                    ((src_[pos_] == '+' || src_[pos_] == '-') && pos_ > start &&
                     (src_[pos_ - 1] == 'e' || src_[pos_ - 1] == 'E')))) {
                if (src_[pos_] == '.' || src_[pos_] == 'e' || src_[pos_] == 'E')
                    isFloat = true;
                ++pos_;
            }
            std::string text = src_.substr(start, pos_ - start);
            if (text.empty()) throw std::runtime_error("expected number");
            if (isFloat) return Const(Box(std::stod(text)));
            return Const(Box(static_cast<int64_t>(std::stoll(text))));
        }
        throw std::runtime_error("unexpected character in expression");
    }
};

} // namespace

NodePtr parse(const std::string& text) {
    Parser p(text);
    return p.parseAll();
}

} // namespace jitx

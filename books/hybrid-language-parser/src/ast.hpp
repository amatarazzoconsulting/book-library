// MIT License - (c) Anthony Matarazzo 2026
// Shared AST core for the Hybrid Computer Language Parser companion sources.
// One flat tagged node type is used by both parsers, the evaluator and the
// pretty printer, so a tree built by either front end can be handed to any
// back end without conversion.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace hybrid {

enum class NodeKind {
    Number, String, Bool, Var, Unary, Binary, Ternary, Call,
    VarDecl, Assign, Print, If, While, Block
};

// Operator set follows the book: arithmetic, the full bitwise family, logical
// and comparison. Pow is an extra used by the Python style ** operator.
enum class BinOp {
    Add, Sub, Mul, Div, Mod, Pow,
    Shl, Shr, BitAnd, BitOr, BitXor,
    LogAnd, LogOr,
    Eq, Ne, Lt, Le, Gt, Ge
};

enum class UnOp { Neg, Plus, Not, BitNot };

struct Node;
using NodePtr = std::shared_ptr<const Node>;

struct Node {
    NodeKind kind = NodeKind::Number;
    BinOp op = BinOp::Add;
    UnOp uop = UnOp::Neg;
    double number = 0.0;
    bool boolean = false;
    std::string text;          // identifier, string literal or call name
    NodePtr a, b, c;           // operands, initializer, branches
    std::vector<NodePtr> args; // call arguments or block statements
    int line = 0;
    int column = 0;
};

struct Program {
    std::vector<NodePtr> statements;
};

namespace detail {

inline std::shared_ptr<Node> fresh(NodeKind kind, int line, int column) {
    auto n = std::make_shared<Node>();
    n->kind = kind;
    n->line = line;
    n->column = column;
    return n;
}

} // namespace detail

inline NodePtr number_node(double v, int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::Number, line, column);
    n->number = v;
    return n;
}

inline NodePtr string_node(const std::string& s, int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::String, line, column);
    n->text = s;
    return n;
}

inline NodePtr bool_node(bool v, int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::Bool, line, column);
    n->boolean = v;
    return n;
}

inline NodePtr var_node(const std::string& name, int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::Var, line, column);
    n->text = name;
    return n;
}

inline NodePtr unary_node(UnOp op, const NodePtr& operand, int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::Unary, line, column);
    n->uop = op;
    n->a = operand;
    return n;
}

inline NodePtr binary_node(BinOp op, const NodePtr& lhs, const NodePtr& rhs,
                           int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::Binary, line, column);
    n->op = op;
    n->a = lhs;
    n->b = rhs;
    return n;
}

inline NodePtr ternary_node(const NodePtr& cond, const NodePtr& yes,
                            const NodePtr& no, int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::Ternary, line, column);
    n->a = cond;
    n->b = yes;
    n->c = no;
    return n;
}

inline NodePtr call_node(const std::string& name, std::vector<NodePtr> args,
                         int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::Call, line, column);
    n->text = name;
    n->args = std::move(args);
    return n;
}

inline NodePtr decl_node(const std::string& name, const NodePtr& init,
                         int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::VarDecl, line, column);
    n->text = name;
    n->a = init;
    return n;
}

inline NodePtr assign_node(const std::string& name, const NodePtr& value,
                           int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::Assign, line, column);
    n->text = name;
    n->a = value;
    return n;
}

inline NodePtr print_node(const NodePtr& value, int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::Print, line, column);
    n->a = value;
    return n;
}

inline NodePtr if_node(const NodePtr& cond, const NodePtr& then_branch,
                       const NodePtr& else_branch, int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::If, line, column);
    n->a = cond;
    n->b = then_branch;
    n->c = else_branch;
    return n;
}

inline NodePtr while_node(const NodePtr& cond, const NodePtr& body,
                          int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::While, line, column);
    n->a = cond;
    n->b = body;
    return n;
}

inline NodePtr block_node(std::vector<NodePtr> statements, int line = 0, int column = 0) {
    auto n = detail::fresh(NodeKind::Block, line, column);
    n->args = std::move(statements);
    return n;
}

inline const char* node_kind_name(NodeKind k) {
    switch (k) {
        case NodeKind::Number: return "Number";
        case NodeKind::String: return "String";
        case NodeKind::Bool: return "Bool";
        case NodeKind::Var: return "Var";
        case NodeKind::Unary: return "Unary";
        case NodeKind::Binary: return "Binary";
        case NodeKind::Ternary: return "Ternary";
        case NodeKind::Call: return "Call";
        case NodeKind::VarDecl: return "VarDecl";
        case NodeKind::Assign: return "Assign";
        case NodeKind::Print: return "Print";
        case NodeKind::If: return "If";
        case NodeKind::While: return "While";
        case NodeKind::Block: return "Block";
    }
    return "?";
}

inline const char* binop_symbol(BinOp op) {
    switch (op) {
        case BinOp::Add: return "+";
        case BinOp::Sub: return "-";
        case BinOp::Mul: return "*";
        case BinOp::Div: return "/";
        case BinOp::Mod: return "%";
        case BinOp::Pow: return "**";
        case BinOp::Shl: return "<<";
        case BinOp::Shr: return ">>";
        case BinOp::BitAnd: return "&";
        case BinOp::BitOr: return "|";
        case BinOp::BitXor: return "^";
        case BinOp::LogAnd: return "&&";
        case BinOp::LogOr: return "||";
        case BinOp::Eq: return "==";
        case BinOp::Ne: return "!=";
        case BinOp::Lt: return "<";
        case BinOp::Le: return "<=";
        case BinOp::Gt: return ">";
        case BinOp::Ge: return ">=";
    }
    return "?";
}

inline const char* binop_word(BinOp op) {
    switch (op) {
        case BinOp::Add: return "add";
        case BinOp::Sub: return "sub";
        case BinOp::Mul: return "mul";
        case BinOp::Div: return "div";
        case BinOp::Mod: return "mod";
        case BinOp::Pow: return "pow";
        case BinOp::Shl: return "shl";
        case BinOp::Shr: return "shr";
        case BinOp::BitAnd: return "bitand";
        case BinOp::BitOr: return "bitor";
        case BinOp::BitXor: return "bitxor";
        case BinOp::LogAnd: return "and";
        case BinOp::LogOr: return "or";
        case BinOp::Eq: return "eq";
        case BinOp::Ne: return "ne";
        case BinOp::Lt: return "lt";
        case BinOp::Le: return "le";
        case BinOp::Gt: return "gt";
        case BinOp::Ge: return "ge";
    }
    return "?";
}

inline const char* unop_symbol(UnOp op) {
    switch (op) {
        case UnOp::Neg: return "-";
        case UnOp::Plus: return "+";
        case UnOp::Not: return "!";
        case UnOp::BitNot: return "~";
    }
    return "?";
}

// Precedence numbers match the book's precedence climbing parser: 20 for
// * / %, 10 for + -, 9 for shifts, 8 relational, 7 equality, 6 &, 5 ^, 4 |,
// 3 &&, 2 ||. Pow sits just above multiplicative, ternary below logical OR.
inline int precedence_of(BinOp op) {
    switch (op) {
        case BinOp::Pow: return 21;
        case BinOp::Mul:
        case BinOp::Div:
        case BinOp::Mod: return 20;
        case BinOp::Add:
        case BinOp::Sub: return 10;
        case BinOp::Shl:
        case BinOp::Shr: return 9;
        case BinOp::Lt:
        case BinOp::Le:
        case BinOp::Gt:
        case BinOp::Ge: return 8;
        case BinOp::Eq:
        case BinOp::Ne: return 7;
        case BinOp::BitAnd: return 6;
        case BinOp::BitXor: return 5;
        case BinOp::BitOr: return 4;
        case BinOp::LogAnd: return 3;
        case BinOp::LogOr: return 2;
    }
    return 0;
}

// Unary operators bind tighter than every binary operator except **, which is
// handled by giving the unary operand the pow precedence as its minimum.
inline int unary_operand_precedence() { return precedence_of(BinOp::Pow); }

inline bool is_right_assoc(BinOp op) { return op == BinOp::Pow; }

inline std::size_t count_nodes(const NodePtr& n) {
    if (!n) return 0;
    std::size_t total = 1 + count_nodes(n->a) + count_nodes(n->b) + count_nodes(n->c);
    for (const NodePtr& arg : n->args) total += count_nodes(arg);
    return total;
}

inline std::size_t tree_depth(const NodePtr& n) {
    if (!n) return 0;
    std::size_t best = tree_depth(n->a);
    best = std::max(best, tree_depth(n->b));
    best = std::max(best, tree_depth(n->c));
    for (const NodePtr& arg : n->args) best = std::max(best, tree_depth(arg));
    return best + 1;
}

// One shared number formatter so the printer, the evaluator and the tests all
// agree on what "42" looks like. 10 significant digits keeps binary floating
// point noise (0.1 + 0.2) from leaking into printed programs.
inline std::string format_number(double v) {
    if (v == 0.0) return "0";
    if (v == static_cast<double>(static_cast<long long>(v)) &&
        v > -1e15 && v < 1e15) {
        char buf[32];
        std::snprintf(buf, sizeof buf, "%lld", static_cast<long long>(v));
        return buf;
    }
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.10g", v);
    return buf;
}

} // namespace hybrid

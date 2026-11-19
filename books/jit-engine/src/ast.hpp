// MIT License - (c) Anthony Matarazzo 2026
//
// ast.hpp - Expression AST builder.
//
// The book's jit_ast.h layer, narrowed to what a formula engine needs:
// constants, variables, binary operators, unary operators, ternaries and
// function calls. Nodes are shared_ptr so the optimizer can share unchanged
// subtrees instead of copying them.
//
// Three things live here:
//   1. factories (Const, Var, Add, ...) that build nodes
//   2. print() which renders an S-expression, so optimizations are visible
//   3. parse() a small recursive-descent parser for infix text like
//      "(x + 2) * 3 - 1" so demos can work from strings
//   4. eval() a tree-walking interpreter used as the O0 baseline

#pragma once

#include "box.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace jitx {

enum class Op {
    // arithmetic
    ADD, SUB, MUL, DIV, MOD,
    // bitwise
    AND, OR, XOR, SHL, SHR,
    // comparison
    EQ, NE, LT, LE, GT, GE,
    // logical
    LAND, LOR
};

enum class UnOp { NEG, NOT, BNOT };

const char* opName(Op op);
const char* unOpName(UnOp op);

class Node;
using NodePtr = std::shared_ptr<Node>;

class Node {
public:
    enum class Kind { CONST, VAR, BIN, UNARY, TERNARY, CALL };

    Kind kind = Kind::CONST;

    Box value;                  // CONST
    std::string name;           // VAR / CALL
    Op op = Op::ADD;            // BIN
    UnOp unop = UnOp::NEG;      // UNARY
    NodePtr a, b, c;            // children
    std::vector<NodePtr> args;  // CALL arguments

    static NodePtr makeConst(const Box& v);
    static NodePtr makeVar(const std::string& n);
    static NodePtr makeBin(Op op, NodePtr l, NodePtr r);
    static NodePtr makeUnary(UnOp op, NodePtr v);
    static NodePtr makeTernary(NodePtr cond, NodePtr t, NodePtr f);
    static NodePtr makeCall(const std::string& fn, std::vector<NodePtr> args);
};

// Convenience constructors. Box is implicit here so Add(Var("x"), 2) reads
// like the formula it represents.
NodePtr Const(const Box& v);
NodePtr Var(const std::string& n);
NodePtr Bin(Op op, NodePtr l, NodePtr r);
NodePtr Un(UnOp op, NodePtr v);
NodePtr Ternary(NodePtr cond, NodePtr t, NodePtr f);
NodePtr Call(const std::string& fn, std::vector<NodePtr> args);

// Sugar used heavily in tests and demos.
NodePtr Add(NodePtr l, NodePtr r);
NodePtr Sub(NodePtr l, NodePtr r);
NodePtr Mul(NodePtr l, NodePtr r);
NodePtr Div(NodePtr l, NodePtr r);
NodePtr Neg(NodePtr v);

using Env = std::unordered_map<std::string, Box>;

// S-expression rendering, e.g. (* (+ x 2) 3).
std::string print(const NodePtr& n);

// Fully parenthesized infix rendering, e.g. "((x + 2) * 3)".
std::string toInfix(const NodePtr& n);

// Number of nodes in the tree (used by tests and the optimizer report).
size_t nodeCount(const NodePtr& n);

// Structural equality, used by tests to assert an optimization fired.
bool sameTree(const NodePtr& a, const NodePtr& b);

// Builtin function dispatch shared by the tree walker, the VM and the
// closure backend, so all three agree on what abs/min/max/sqrt/len/clamp do.
Box callBuiltin(const std::string& name, std::vector<Box>& args);

// Tree-walking evaluation. This is the "no compilation at all" baseline that
// the compiled backends are measured against.
Box eval(const NodePtr& n, const Env& env);

// Recursive-descent parser for infix arithmetic. Grammar:
//   expr    := term (('+'|'-') term)*
//   term    := factor (('*'|'/'|'%') factor)*
//   factor  := unary ('**'? no) ; handled left to right
//   unary   := ('-'|'!') unary | primary
//   primary := number | ident | ident '(' args ')' | '(' expr ')'
// Throws std::runtime_error on malformed input.
NodePtr parse(const std::string& text);

} // namespace jitx

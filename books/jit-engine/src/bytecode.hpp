// MIT License - (c) Anthony Matarazzo 2026
//
// bytecode.hpp - compile the AST to stack bytecode and run it two ways.
//
// The book's execution layer. It has three tiers, each one removing work
// from the hot path:
//
//   tree walk     eval() in ast.cpp, one virtual-ish dispatch per node
//   bytecode VM   Program + VM below, a flat instruction array and a switch
//   closure       compileClosure() below, a tree of std::function objects
//
// All three must agree on every formula; the tests check exactly that. The
// benchmark measures how much the two compiled forms buy over the tree walk.
//
// Bytecode is stack based: PUSH a constant or slot, apply an operator that
// pops two and pushes one. Control flow uses absolute jumps resolved at
// compile time, so a Program is immutable and safe to share between runs.

#pragma once

#include "ast.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace jitx {

enum class Opcode : uint8_t {
    PUSH_CONST,   // arg = constant index
    PUSH_SLOT,    // arg = variable slot
    ADD, SUB, MUL, DIV, MOD,
    AND, OR, XOR, SHL, SHR,
    EQ, NE, LT, LE, GT, GE,
    NEG, NOT, BNOT,
    DUP,          // duplicate top of stack
    JUMP,         // arg = target pc
    JUMP_IF_FALSE,// arg = target pc, pops the condition
    JUMP_IF_TRUE, // arg = target pc, pops the condition
    CALL,         // arg = callee index, next byte holds argc
    POP,          // discard top of stack
    HALT
};

const char* opcodeName(Opcode op);

struct Instr {
    Opcode op;
    int64_t arg;
    int argc;  // only used by CALL
};

// A compiled, immutable bytecode program.
class Program {
public:
    std::vector<Instr> code;
    std::vector<Box> consts;
    std::vector<std::string> symbols;  // variable slots
    std::vector<std::string> callees;

    int slotFor(const std::string& name);  // interns, returns slot index
    int constFor(const Box& value);        // interns, returns const index
    int calleeFor(const std::string& name);

    // Human readable listing, one instruction per line. Used by the teaching
    // program and by test failure messages.
    std::string disassemble() const;

    size_t instructionCount() const { return code.size(); }
    size_t slotCount() const { return symbols.size(); }
};

// Compile an expression tree. Returns an empty Program when the input is null.
Program compile(const NodePtr& root);

// Execute a program against a slot vector. Missing slots read as VOID, which
// is the same behaviour as eval() with a missing env entry.
Box runProgram(const Program& prog, const std::vector<Box>& slots);

// Convenience: build the slot vector from an Env using the program's symbol
// table, then run.
Box runProgram(const Program& prog, const Env& env);

// The compiled-closure backend. Each node becomes a std::function that closes
// over its children, so evaluation is a chain of direct calls with no
// instruction dispatch and no operand stack. Slots are passed by reference
// and are indexed the same way as the bytecode slots.
using Closure = std::function<Box(std::vector<Box>&)>;

class ClosureProgram {
public:
    ClosureProgram() = default;
    Closure root;
    std::vector<std::string> symbols;
    std::vector<std::string> callees;
    size_t nodeCount = 0;
    int slotFor(const std::string& name);
    int calleeFor(const std::string& name);
};

ClosureProgram compileClosure(const NodePtr& root);

Box runClosure(const ClosureProgram& prog, std::vector<Box>& slots);
Box runClosure(const ClosureProgram& prog, const Env& env);

// Number of bytecode instructions emitted for a tree, without keeping the
// program. Handy for sizing discussion.
size_t compiledSize(const NodePtr& root);

} // namespace jitx

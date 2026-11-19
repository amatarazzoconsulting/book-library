// MIT License - (c) Anthony Matarazzo 2026
//
// bytecode.cpp - AST to stack bytecode, the VM, and the closure backend.

#include "bytecode.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace jitx {

const char* opcodeName(Opcode op) {
    switch (op) {
        case Opcode::PUSH_CONST: return "PUSH_CONST";
        case Opcode::PUSH_SLOT: return "PUSH_SLOT";
        case Opcode::ADD: return "ADD";
        case Opcode::SUB: return "SUB";
        case Opcode::MUL: return "MUL";
        case Opcode::DIV: return "DIV";
        case Opcode::MOD: return "MOD";
        case Opcode::AND: return "AND";
        case Opcode::OR: return "OR";
        case Opcode::XOR: return "XOR";
        case Opcode::SHL: return "SHL";
        case Opcode::SHR: return "SHR";
        case Opcode::EQ: return "EQ";
        case Opcode::NE: return "NE";
        case Opcode::LT: return "LT";
        case Opcode::LE: return "LE";
        case Opcode::GT: return "GT";
        case Opcode::GE: return "GE";
        case Opcode::NEG: return "NEG";
        case Opcode::NOT: return "NOT";
        case Opcode::BNOT: return "BNOT";
        case Opcode::DUP: return "DUP";
        case Opcode::JUMP: return "JUMP";
        case Opcode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case Opcode::JUMP_IF_TRUE: return "JUMP_IF_TRUE";
        case Opcode::CALL: return "CALL";
        case Opcode::POP: return "POP";
        case Opcode::HALT: return "HALT";
    }
    return "?";
}

//---------------------------------------------------------------------------
// Program tables
//---------------------------------------------------------------------------

int Program::slotFor(const std::string& name) {
    for (size_t i = 0; i < symbols.size(); ++i)
        if (symbols[i] == name) return static_cast<int>(i);
    symbols.push_back(name);
    return static_cast<int>(symbols.size() - 1);
}

int Program::constFor(const Box& value) {
    for (size_t i = 0; i < consts.size(); ++i)
        if (consts[i] == value && consts[i].type() == value.type())
            return static_cast<int>(i);
    consts.push_back(value);
    return static_cast<int>(consts.size() - 1);
}

int Program::calleeFor(const std::string& name) {
    for (size_t i = 0; i < callees.size(); ++i)
        if (callees[i] == name) return static_cast<int>(i);
    callees.push_back(name);
    return static_cast<int>(callees.size() - 1);
}

std::string Program::disassemble() const {
    std::ostringstream os;
    for (size_t pc = 0; pc < code.size(); ++pc) {
        const Instr& in = code[pc];
        os << (pc < 10 ? "  " : " ") << pc << "  " << opcodeName(in.op);
        switch (in.op) {
            case Opcode::PUSH_CONST:
                os << "  #" << in.arg << "  ; " << consts[static_cast<size_t>(in.arg)].toString();
                break;
            case Opcode::PUSH_SLOT:
                os << "  s" << in.arg << "  ; " << symbols[static_cast<size_t>(in.arg)];
                break;
            case Opcode::JUMP:
            case Opcode::JUMP_IF_FALSE:
            case Opcode::JUMP_IF_TRUE:
                os << "  ->" << in.arg;
                break;
            case Opcode::CALL:
                os << "  " << callees[static_cast<size_t>(in.arg)] << " argc=" << in.argc;
                break;
            default:
                break;
        }
        os << '\n';
    }
    return os.str();
}

//---------------------------------------------------------------------------
// Compiler
//---------------------------------------------------------------------------

namespace {

struct Emitter {
    Program& p;

    void emit(Opcode op, int64_t arg = 0, int argc = 0) {
        p.code.push_back(Instr{op, arg, argc});
    }

    size_t here() const { return p.code.size(); }

    void patch(size_t at, size_t target) {
        p.code[at].arg = static_cast<int64_t>(target);
    }
};

Opcode opcodeFor(Op op) {
    switch (op) {
        case Op::ADD: return Opcode::ADD;
        case Op::SUB: return Opcode::SUB;
        case Op::MUL: return Opcode::MUL;
        case Op::DIV: return Opcode::DIV;
        case Op::MOD: return Opcode::MOD;
        case Op::AND: return Opcode::AND;
        case Op::OR: return Opcode::OR;
        case Op::XOR: return Opcode::XOR;
        case Op::SHL: return Opcode::SHL;
        case Op::SHR: return Opcode::SHR;
        case Op::EQ: return Opcode::EQ;
        case Op::NE: return Opcode::NE;
        case Op::LT: return Opcode::LT;
        case Op::LE: return Opcode::LE;
        case Op::GT: return Opcode::GT;
        case Op::GE: return Opcode::GE;
        case Op::LAND: return Opcode::AND;  // handled specially before this
        case Op::LOR: return Opcode::OR;
    }
    return Opcode::ADD;
}

Opcode opcodeFor(UnOp op) {
    switch (op) {
        case UnOp::NEG: return Opcode::NEG;
        case UnOp::NOT: return Opcode::NOT;
        case UnOp::BNOT: return Opcode::BNOT;
    }
    return Opcode::NEG;
}

void compileNode(const NodePtr& n, Emitter& e) {
    if (!n) {
        e.emit(Opcode::PUSH_CONST, e.p.constFor(Box()));
        return;
    }
    switch (n->kind) {
        case Node::Kind::CONST:
            e.emit(Opcode::PUSH_CONST, e.p.constFor(n->value));
            break;
        case Node::Kind::VAR:
            e.emit(Opcode::PUSH_SLOT, e.p.slotFor(n->name));
            break;
        case Node::Kind::UNARY:
            compileNode(n->a, e);
            e.emit(opcodeFor(n->unop));
            break;
        case Node::Kind::TERNARY: {
            compileNode(n->a, e);
            size_t jf = e.here();
            e.emit(Opcode::JUMP_IF_FALSE);
            compileNode(n->b, e);
            size_t jend = e.here();
            e.emit(Opcode::JUMP);
            e.patch(jf, e.here());
            compileNode(n->c, e);
            e.patch(jend, e.here());
            break;
        }
        case Node::Kind::CALL: {
            for (const auto& arg : n->args) compileNode(arg, e);
            e.emit(Opcode::CALL, e.p.calleeFor(n->name),
                   static_cast<int>(n->args.size()));
            break;
        }
        case Node::Kind::BIN: {
            // Short circuit: keep the left value on the stack as the result
            // when it already decides the answer, otherwise drop it and
            // evaluate the right side.
            if (n->op == Op::LAND || n->op == Op::LOR) {
                compileNode(n->a, e);
                e.emit(Opcode::DUP);
                size_t jmp = e.here();
                e.emit(n->op == Op::LAND ? Opcode::JUMP_IF_FALSE
                                         : Opcode::JUMP_IF_TRUE);
                e.emit(Opcode::POP);
                compileNode(n->b, e);
                e.patch(jmp, e.here());
                break;
            }
            compileNode(n->a, e);
            compileNode(n->b, e);
            e.emit(opcodeFor(n->op));
            break;
        }
    }
}

} // namespace

Program compile(const NodePtr& root) {
    Program p;
    if (!root) return p;
    Emitter e{p};
    compileNode(root, e);
    e.emit(Opcode::HALT);
    return p;
}

size_t compiledSize(const NodePtr& root) {
    return compile(root).instructionCount();
}

//---------------------------------------------------------------------------
// VM
//---------------------------------------------------------------------------

namespace {

Box applyBin(Opcode op, const Box& l, const Box& r) {
    switch (op) {
        case Opcode::ADD: return l + r;
        case Opcode::SUB: return l - r;
        case Opcode::MUL: return l * r;
        case Opcode::DIV: return l / r;
        case Opcode::MOD: return l % r;
        case Opcode::AND: return l & r;
        case Opcode::OR: return l | r;
        case Opcode::XOR: return l ^ r;
        case Opcode::SHL: return l << static_cast<int>(r.toInt64());
        case Opcode::SHR: return l >> static_cast<int>(r.toInt64());
        case Opcode::EQ: return Box(l == r);
        case Opcode::NE: return Box(l != r);
        case Opcode::LT: return Box(l < r);
        case Opcode::LE: return Box(l <= r);
        case Opcode::GT: return Box(l > r);
        case Opcode::GE: return Box(l >= r);
        default: return Box();
    }
}

Box applyUn(Opcode op, const Box& v) {
    switch (op) {
        case Opcode::NEG: return Box(-v.toDouble());
        case Opcode::NOT: return Box(!v.toBool());
        case Opcode::BNOT: return ~v;
        default: return Box();
    }
}

} // namespace

Box runProgram(const Program& prog, const std::vector<Box>& slots) {
    std::vector<Box> stack;
    stack.reserve(32);
    std::vector<Box> args;
    size_t pc = 0;
    size_t guard = 0;
    const size_t kMaxSteps = 1u << 22;

    while (pc < prog.code.size()) {
        if (++guard > kMaxSteps) return Box();  // runaway program
        const Instr& in = prog.code[pc];
        switch (in.op) {
            case Opcode::PUSH_CONST:
                stack.push_back(prog.consts[static_cast<size_t>(in.arg)]);
                ++pc;
                break;
            case Opcode::PUSH_SLOT: {
                size_t s = static_cast<size_t>(in.arg);
                stack.push_back(s < slots.size() ? slots[s] : Box());
                ++pc;
                break;
            }
            case Opcode::ADD: case Opcode::SUB: case Opcode::MUL:
            case Opcode::DIV: case Opcode::MOD: case Opcode::AND:
            case Opcode::OR: case Opcode::XOR: case Opcode::SHL:
            case Opcode::SHR: case Opcode::EQ: case Opcode::NE:
            case Opcode::LT: case Opcode::LE: case Opcode::GT:
            case Opcode::GE: {
                if (stack.size() < 2) return Box();
                Box r = stack.back();
                stack.pop_back();
                Box l = stack.back();
                stack.pop_back();
                stack.push_back(applyBin(in.op, l, r));
                ++pc;
                break;
            }
            case Opcode::NEG: case Opcode::NOT: case Opcode::BNOT: {
                if (stack.empty()) return Box();
                Box v = stack.back();
                stack.pop_back();
                stack.push_back(applyUn(in.op, v));
                ++pc;
                break;
            }
            case Opcode::DUP:
                if (stack.empty()) return Box();
                stack.push_back(stack.back());
                ++pc;
                break;
            case Opcode::JUMP:
                pc = static_cast<size_t>(in.arg);
                break;
            case Opcode::JUMP_IF_TRUE: {
                if (stack.empty()) return Box();
                bool c = stack.back().toBool();
                stack.pop_back();
                if (c) pc = static_cast<size_t>(in.arg);
                else ++pc;
                break;
            }
            case Opcode::JUMP_IF_FALSE: {
                if (stack.empty()) return Box();
                bool c = stack.back().toBool();
                stack.pop_back();
                if (!c) pc = static_cast<size_t>(in.arg);
                else ++pc;
                break;
            }
            case Opcode::CALL: {
                args.clear();
                for (int i = 0; i < in.argc; ++i) {
                    if (stack.empty()) return Box();
                    args.push_back(stack.back());
                    stack.pop_back();
                }
                // Arguments were pushed left to right, so reverse them.
                std::reverse(args.begin(), args.end());
                stack.push_back(
                    callBuiltin(prog.callees[static_cast<size_t>(in.arg)], args));
                ++pc;
                break;
            }
            case Opcode::POP:
                if (!stack.empty()) stack.pop_back();
                ++pc;
                break;
            case Opcode::HALT:
                return stack.empty() ? Box() : stack.back();
        }
    }
    return stack.empty() ? Box() : stack.back();
}

Box runProgram(const Program& prog, const Env& env) {
    std::vector<Box> slots(prog.symbols.size());
    for (size_t i = 0; i < prog.symbols.size(); ++i) {
        auto it = env.find(prog.symbols[i]);
        if (it != env.end()) slots[i] = it->second;
    }
    return runProgram(prog, slots);
}

//---------------------------------------------------------------------------
// Closure backend
//---------------------------------------------------------------------------

int ClosureProgram::slotFor(const std::string& name) {
    for (size_t i = 0; i < symbols.size(); ++i)
        if (symbols[i] == name) return static_cast<int>(i);
    symbols.push_back(name);
    return static_cast<int>(symbols.size() - 1);
}

int ClosureProgram::calleeFor(const std::string& name) {
    for (size_t i = 0; i < callees.size(); ++i)
        if (callees[i] == name) return static_cast<int>(i);
    callees.push_back(name);
    return static_cast<int>(callees.size() - 1);
}

namespace {

Closure build(const NodePtr& n, ClosureProgram& prog) {
    ++prog.nodeCount;
    if (!n) {
        return [](std::vector<Box>&) { return Box(); };
    }
    switch (n->kind) {
        case Node::Kind::CONST: {
            Box v = n->value;
            return [v](std::vector<Box>&) { return v; };
        }
        case Node::Kind::VAR: {
            int slot = prog.slotFor(n->name);
            return [slot](std::vector<Box>& s) {
                return static_cast<size_t>(slot) < s.size()
                           ? s[static_cast<size_t>(slot)]
                           : Box();
            };
        }
        case Node::Kind::UNARY: {
            Closure a = build(n->a, prog);
            UnOp op = n->unop;
            return [a, op](std::vector<Box>& s) -> Box {
                Box v = a(s);
                switch (op) {
                    case UnOp::NEG: return Box(-v.toDouble());
                    case UnOp::NOT: return Box(!v.toBool());
                    case UnOp::BNOT: return ~v;
                }
                return Box();
            };
        }
        case Node::Kind::TERNARY: {
            Closure c = build(n->a, prog);
            Closure t = build(n->b, prog);
            Closure f = build(n->c, prog);
            return [c, t, f](std::vector<Box>& s) -> Box {
                return c(s).toBool() ? t(s) : f(s);
            };
        }
        case Node::Kind::CALL: {
            std::vector<Closure> argv;
            argv.reserve(n->args.size());
            for (const auto& arg : n->args) argv.push_back(build(arg, prog));
            std::string name = n->name;
            return [argv, name](std::vector<Box>& s) -> Box {
                std::vector<Box> a;
                a.reserve(argv.size());
                for (const auto& f : argv) a.push_back(f(s));
                return callBuiltin(name, a);
            };
        }
        case Node::Kind::BIN: {
            Closure l = build(n->a, prog);
            Closure r = build(n->b, prog);
            Op op = n->op;
            if (op == Op::LAND)
                return [l, r](std::vector<Box>& s) -> Box {
                    Box lv = l(s);
                    return lv.toBool() ? r(s) : lv;
                };
            if (op == Op::LOR)
                return [l, r](std::vector<Box>& s) -> Box {
                    Box lv = l(s);
                    return lv.toBool() ? lv : r(s);
                };
            return [l, r, op](std::vector<Box>& s) -> Box {
                Box lv = l(s);
                Box rv = r(s);
                switch (op) {
                    case Op::ADD: return lv + rv;
                    case Op::SUB: return lv - rv;
                    case Op::MUL: return lv * rv;
                    case Op::DIV: return lv / rv;
                    case Op::MOD: return lv % rv;
                    case Op::AND: return lv & rv;
                    case Op::OR: return lv | rv;
                    case Op::XOR: return lv ^ rv;
                    case Op::SHL: return lv << static_cast<int>(rv.toInt64());
                    case Op::SHR: return lv >> static_cast<int>(rv.toInt64());
                    case Op::EQ: return Box(lv == rv);
                    case Op::NE: return Box(lv != rv);
                    case Op::LT: return Box(lv < rv);
                    case Op::LE: return Box(lv <= rv);
                    case Op::GT: return Box(lv > rv);
                    case Op::GE: return Box(lv >= rv);
                    default: return Box();
                }
            };
        }
    }
    return [](std::vector<Box>&) { return Box(); };
}

} // namespace

ClosureProgram compileClosure(const NodePtr& root) {
    ClosureProgram prog;
    prog.root = build(root, prog);
    return prog;
}

Box runClosure(const ClosureProgram& prog, std::vector<Box>& slots) {
    if (!prog.root) return Box();
    return prog.root(slots);
}

Box runClosure(const ClosureProgram& prog, const Env& env) {
    std::vector<Box> slots(prog.symbols.size());
    for (size_t i = 0; i < prog.symbols.size(); ++i) {
        auto it = env.find(prog.symbols[i]);
        if (it != env.end()) slots[i] = it->second;
    }
    return runClosure(prog, slots);
}

} // namespace jitx

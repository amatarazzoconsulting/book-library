// MIT License - (c) Anthony Matarazzo 2026
//
// demo.cpp - short tour of all four algorithms. Every section prints a
// labelled result so the output can be compared against the book.

#include "ast.hpp"
#include "box.hpp"
#include "bytecode.hpp"
#include "optimizer.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace jitx;

static void rule(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

int main() {
    std::cout << "JIT Engine with API Interface - demo\n";

    //------------------------------------------------------------------
    rule("1. Box tagged value type");
    //------------------------------------------------------------------
    Box n(42);
    Box d(3.5);
    Box s(std::string("hello"));
    Box flag(true);
    Box vec = Box::makeVector();
    for (int i = 1; i <= 4; ++i) vec.push_back(Box(static_cast<int64_t>(i * i)));

    std::cout << "int box     : " << n.toString() << " (" << n.typeName() << ")\n";
    std::cout << "double box  : " << d.toString() << " (" << d.typeName() << ")\n";
    std::cout << "string box  : " << s.toString() << " (" << s.typeName()
              << ", len " << s.size() << ")\n";
    std::cout << "bool box    : " << flag.toString() << " (" << flag.typeName() << ")\n";
    std::cout << "vector box  : " << vec.toString() << " elems "
              << vec.at(0).toString() << "," << vec.at(1).toString() << ","
              << vec.at(2).toString() << "," << vec.at(3).toString() << "\n";
    std::cout << "7 / 2       : " << (Box(int64_t{7}) / Box(int64_t{2})).toString()
              << "  (division always promotes to double)\n";
    std::cout << "7 % 2       : " << (Box(int64_t{7}) % Box(int64_t{2})).toString() << "\n";
    std::cout << "5 << 3      : " << (Box(int64_t{5}) << 3).toString() << "\n";
    std::cout << "copy shares : " << (Box(Box(s)).toString() == s.toString() ? "yes" : "no")
              << "  (shared_ptr refcount, no deep copy)\n";

    //------------------------------------------------------------------
    rule("2. Expression AST builder");
    //------------------------------------------------------------------
    NodePtr expr = parse("(x + 2) * 3 - 1");
    std::cout << "source      : (x + 2) * 3 - 1\n";
    std::cout << "s-expr      : " << print(expr) << "\n";
    std::cout << "infix       : " << toInfix(expr) << "\n";
    std::cout << "nodes       : " << nodeCount(expr) << "\n";
    Env env{{"x", Box(int64_t{10})}};
    std::cout << "x = 10      : " << eval(expr, env).toString() << "\n";

    NodePtr built = Mul(Add(Var("a"), Var("b")), Const(Box(int64_t{2})));
    std::cout << "built by API: " << print(built) << "\n";

    //------------------------------------------------------------------
    rule("3. Optimizer passes");
    //------------------------------------------------------------------
    NodePtr raw = parse("(y * 2) + (3 + 4) + 0 + (z * 1) + (y * 2)");
    std::cout << "before      : " << print(raw) << "\n";
    for (OptLevel lv : {OptLevel::O0, OptLevel::O1, OptLevel::O2, OptLevel::O3}) {
        Optimizer opt(lv);
        NodePtr out = opt.run(raw);
        size_t rewrites = 0;
        for (const auto& st : opt.report()) rewrites += st.rewrites;
        std::cout << levelName(lv) << " -> " << print(out) << "\n";
        std::cout << "      nodes " << nodeCount(raw) << " -> " << nodeCount(out)
                  << ", " << rewrites << " rewrite(s)\n";
    }
    Optimizer o3(OptLevel::O3);
    NodePtr best = o3.run(raw);
    Env env2{{"y", Box(int64_t{5})}, {"z", Box(int64_t{7})}};
    std::cout << "O3 result   : " << eval(best, env2).toString()
              << "  (unoptimized " << eval(raw, env2).toString() << ")\n";

    //------------------------------------------------------------------
    rule("4. Bytecode VM and compiled closure");
    //------------------------------------------------------------------
    NodePtr prog = parse("clamp(x * 2 + 1, 0, 100)");
    Program bc = compile(prog);
    std::cout << "expr        : " << print(prog) << "\n";
    std::cout << "instructions: " << bc.instructionCount()
              << ", constants " << bc.consts.size()
              << ", slots " << bc.slotCount() << "\n";
    std::cout << "disassembly :\n" << bc.disassemble();

    ClosureProgram cl = compileClosure(prog);
    Env env3{{"x", Box(int64_t{30})}};
    std::cout << "x = 30\n";
    std::cout << "  tree walk : " << eval(prog, env3).toString() << "\n";
    std::cout << "  bytecode  : " << runProgram(bc, env3).toString() << "\n";
    std::cout << "  closure   : " << runClosure(cl, env3).toString() << "\n";

    // Same program, three engines, over a range of inputs.
    std::cout << "sweep x = 0..5:\n";
    for (int x = 0; x <= 5; ++x) {
        Env e{{"x", Box(static_cast<int64_t>(x))}};
        std::cout << "  x=" << x << " tree=" << eval(prog, e).toString()
                  << " vm=" << runProgram(bc, e).toString()
                  << " closure=" << runClosure(cl, e).toString() << "\n";
    }

    std::cout << "\ndone\n";
    return 0;
}

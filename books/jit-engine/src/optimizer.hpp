// MIT License - (c) Anthony Matarazzo 2026
//
// optimizer.hpp - optimizing passes over the expression AST.
//
// This is the jit_optimizer.h layer. Passes, in the order the level enables
// them:
//
//   O0  nothing at all
//   O1  constant folding, algebraic simplification, strength reduction
//   O2  O1 plus common subexpression elimination
//   O3  O2 plus reassociation and a final folding sweep
//
// Every pass is a pure function on the tree: it returns a new tree and never
// mutates the input, so a caller can keep the unoptimized original for
// comparison. The Optimizer also records how many rewrites each pass made so
// a program can report exactly what happened to a formula.

#pragma once

#include "ast.hpp"

#include <string>
#include <vector>

namespace jitx {

enum class OptLevel { O0 = 0, O1 = 1, O2 = 2, O3 = 3 };

const char* levelName(OptLevel l);

struct PassStat {
    std::string name;
    size_t rewrites = 0;
    bool ran = false;
};

class Optimizer {
public:
    explicit Optimizer(OptLevel level = OptLevel::O2);

    OptLevel level() const { return level_; }
    void setLevel(OptLevel l) { level_ = l; }

    // Run the pipeline for the configured level.
    NodePtr run(const NodePtr& root);

    // Individual passes, exposed so tests and the teaching program can call
    // one at a time.
    static NodePtr constantFold(const NodePtr& n, size_t* hits = nullptr);
    static NodePtr simplify(const NodePtr& n, size_t* hits = nullptr);
    static NodePtr strengthReduce(const NodePtr& n, size_t* hits = nullptr);
    static NodePtr eliminateCommon(const NodePtr& n, size_t* hits = nullptr);
    static NodePtr reassociate(const NodePtr& n, size_t* hits = nullptr);

    // Per-pass report from the most recent run().
    const std::vector<PassStat>& report() const { return report_; }
    std::string reportText() const;

private:
    OptLevel level_;
    std::vector<PassStat> report_;
};

} // namespace jitx

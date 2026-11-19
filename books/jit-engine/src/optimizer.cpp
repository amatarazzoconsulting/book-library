// MIT License - (c) Anthony Matarazzo 2026
//
// optimizer.cpp - the five rewriting passes.

#include "optimizer.hpp"

#include <cmath>
#include <sstream>
#include <unordered_map>

namespace jitx {

const char* levelName(OptLevel l) {
    switch (l) {
        case OptLevel::O0: return "O0";
        case OptLevel::O1: return "O1";
        case OptLevel::O2: return "O2";
        case OptLevel::O3: return "O3";
    }
    return "?";
}

Optimizer::Optimizer(OptLevel level) : level_(level) {}

//---------------------------------------------------------------------------
// Helpers
//---------------------------------------------------------------------------

namespace {

bool isConstZero(const NodePtr& n) {
    return n && n->kind == Node::Kind::CONST && n->value.isNumeric() &&
           n->value.toDouble() == 0.0;
}

bool isConstOne(const NodePtr& n) {
    return n && n->kind == Node::Kind::CONST && n->value.isNumeric() &&
           n->value.toDouble() == 1.0;
}

// A subtree can evaluate to VOID: an unbound variable, an unknown builtin,
// or a division by zero. Identities that delete an operand are only valid
// when the survivor cannot be VOID, because VOID + 0 is the double 0.0 while
// VOID alone is still VOID. Proving "cannot be VOID" is a three line check,
// so the passes ask rather than guess.
bool mayBeVoid(const NodePtr& n) {
    if (!n) return true;
    switch (n->kind) {
        case Node::Kind::CONST:
            return n->value.type() == Box::Type::VOID;
        case Node::Kind::VAR:
        case Node::Kind::CALL:
            return true;
        case Node::Kind::BIN:
            if (n->op == Op::DIV || n->op == Op::MOD) return true;
            return mayBeVoid(n->a) || mayBeVoid(n->b);
        case Node::Kind::UNARY:
            return mayBeVoid(n->a);
        case Node::Kind::TERNARY:
            return mayBeVoid(n->b) || mayBeVoid(n->c);
    }
    return true;
}

bool isPowerOfTwo(int64_t v, int* shift) {
    if (v <= 0) return false;
    int s = 0;
    while ((int64_t{1} << s) < v) ++s;
    if ((int64_t{1} << s) != v) return false;
    *shift = s;
    return true;
}

// Rebuild a node with new children, keeping the same kind and payload.
NodePtr withChildren(const NodePtr& n, NodePtr a, NodePtr b, NodePtr c) {
    if (!n) return nullptr;
    auto r = std::make_shared<Node>(*n);
    r->a = std::move(a);
    r->b = std::move(b);
    r->c = std::move(c);
    r->args.clear();
    for (const auto& arg : n->args) r->args.push_back(arg);
    return r;
}

// Rewrite every child, then let the caller decide about the node itself.
NodePtr mapChildren(const NodePtr& n, NodePtr (*f)(const NodePtr&, size_t*),
                    size_t* hits) {
    if (!n) return nullptr;
    if (n->kind == Node::Kind::CONST || n->kind == Node::Kind::VAR) return n;
    if (n->kind == Node::Kind::CALL) {
        std::vector<NodePtr> args;
        args.reserve(n->args.size());
        for (const auto& arg : n->args) args.push_back(f(arg, hits));
        auto r = std::make_shared<Node>(*n);
        r->args = std::move(args);
        return r;
    }
    return withChildren(n, f(n->a, hits), f(n->b, hits), f(n->c, hits));
}

} // namespace

//---------------------------------------------------------------------------
// Pass 1: constant folding
//---------------------------------------------------------------------------

NodePtr Optimizer::constantFold(const NodePtr& n, size_t* hits) {
    if (!n) return nullptr;
    NodePtr t = mapChildren(n, &Optimizer::constantFold, hits);

    if (t->kind == Node::Kind::BIN && t->a && t->b &&
        t->a->kind == Node::Kind::CONST && t->b->kind == Node::Kind::CONST) {
        const Box& l = t->a->value;
        const Box& r = t->b->value;
        // Leave division by zero for runtime so the failure mode is visible
        // rather than baked into the tree.
        if ((t->op == Op::DIV || t->op == Op::MOD) && r.toDouble() == 0.0)
            return t;
        Box out;
        bool folded = true;
        switch (t->op) {
            case Op::ADD: out = l + r; break;
            case Op::SUB: out = l - r; break;
            case Op::MUL: out = l * r; break;
            case Op::DIV: out = l / r; break;
            case Op::MOD: out = l % r; break;
            case Op::AND: out = l & r; break;
            case Op::OR: out = l | r; break;
            case Op::XOR: out = l ^ r; break;
            case Op::SHL: out = l << static_cast<int>(r.toInt64()); break;
            case Op::SHR: out = l >> static_cast<int>(r.toInt64()); break;
            case Op::EQ: out = Box(l == r); break;
            case Op::NE: out = Box(l != r); break;
            case Op::LT: out = Box(l < r); break;
            case Op::LE: out = Box(l <= r); break;
            case Op::GT: out = Box(l > r); break;
            case Op::GE: out = Box(l >= r); break;
            // LAND and LOR return the deciding operand, not a bool: the
            // tree walker, the VM and the closure backend all do, so folding
            // them to a bool here would change both the value and the type.
            case Op::LAND: out = l.toBool() ? r : l; break;
            case Op::LOR: out = l.toBool() ? l : r; break;
            default: folded = false; break;
        }
        if (folded) {
            if (hits) ++*hits;
            return Const(out);
        }
    }

    if (t->kind == Node::Kind::UNARY && t->a &&
        t->a->kind == Node::Kind::CONST) {
        if (hits) ++*hits;
        switch (t->unop) {
            case UnOp::NEG: return Const(Box(-t->a->value.toDouble()));
            case UnOp::NOT: return Const(Box(!t->a->value.toBool()));
            case UnOp::BNOT: return Const(~t->a->value);
        }
    }

    if (t->kind == Node::Kind::TERNARY && t->a &&
        t->a->kind == Node::Kind::CONST) {
        if (hits) ++*hits;
        return t->a->value.toBool() ? t->b : t->c;
    }

    return t;
}

//---------------------------------------------------------------------------
// Pass 2: algebraic simplification (identities)
//---------------------------------------------------------------------------

NodePtr Optimizer::simplify(const NodePtr& n, size_t* hits) {
    if (!n) return nullptr;
    NodePtr t = mapChildren(n, &Optimizer::simplify, hits);
    if (t->kind != Node::Kind::BIN) return t;

    const NodePtr& l = t->a;
    const NodePtr& r = t->b;

    // The identities that drop an operand need the survivor to be known
    // non-VOID, so each one is guarded by !mayBeVoid().
    switch (t->op) {
        case Op::ADD:
            if (isConstZero(r) && !mayBeVoid(l)) { if (hits) ++*hits; return l; }
            if (isConstZero(l) && !mayBeVoid(r)) { if (hits) ++*hits; return r; }
            break;
        case Op::SUB:
            if (isConstZero(r) && !mayBeVoid(l)) { if (hits) ++*hits; return l; }
            // x - x is only safe to drop when x is not a call: a call could
            // have side effects, and it could be VOID or NaN.
            if (sameTree(l, r) && l->kind != Node::Kind::CALL &&
                !mayBeVoid(l)) {
                if (hits) ++*hits;
                return Const(Box(int64_t{0}));
            }
            break;
        case Op::MUL:
            if (isConstZero(l) || isConstZero(r)) {
                if (hits) ++*hits;
                return Const(Box(int64_t{0}));
            }
            if (isConstOne(r) && !mayBeVoid(l)) { if (hits) ++*hits; return l; }
            if (isConstOne(l) && !mayBeVoid(r)) { if (hits) ++*hits; return r; }
            break;
        case Op::DIV:
            if (isConstOne(r) && !mayBeVoid(l)) { if (hits) ++*hits; return l; }
            break;
        // No identity rules for OR and XOR. Box bitwise operators return the
        // U64 kind, so "x | 0 -> x" would hand back whatever kind x had, and
        // when x is VOID the two forms differ outright (VOID vs 0). Keeping
        // the node costs one instruction and keeps every backend in step.
        case Op::AND:
            if (isConstZero(l) || isConstZero(r)) {
                if (hits) ++*hits;
                return Const(Box(uint64_t{0}));
            }
            break;
        case Op::LAND:
            if (l->kind == Node::Kind::CONST) {
                if (hits) ++*hits;
                return l->value.toBool() ? r : l;
            }
            break;
        case Op::LOR:
            if (l->kind == Node::Kind::CONST) {
                if (hits) ++*hits;
                return l->value.toBool() ? l : r;
            }
            break;
        default:
            break;
    }
    return t;
}

//---------------------------------------------------------------------------
// Pass 3: strength reduction
//---------------------------------------------------------------------------

NodePtr Optimizer::strengthReduce(const NodePtr& n, size_t* hits) {
    if (!n) return nullptr;
    NodePtr t = mapChildren(n, &Optimizer::strengthReduce, hits);
    if (t->kind != Node::Kind::BIN) return t;

    // x * 2^n  ->  x << n. Exact for integers, and Box::operator<< scales by
    // the same power of two for a floating operand, so the rewrite is safe
    // whatever x turns out to be at run time.
    //
    // Division and modulo are deliberately NOT reduced to shifts. A right
    // shift is a logical shift here, so (-5) / 2 would become a huge positive
    // number instead of -2.5, and (a % 2) -> (a & 1) is wrong for negatives
    // too. Reporting that gap is more useful than a silently wrong answer.
    int shift = 0;
    if (t->op == Op::MUL && t->b && t->b->kind == Node::Kind::CONST &&
        t->b->value.isInteger() && isPowerOfTwo(t->b->value.toInt64(), &shift)) {
        if (hits) ++*hits;
        return Bin(Op::SHL, t->a, Const(Box(int64_t{shift})));
    }
    return t;
}

//---------------------------------------------------------------------------
// Pass 4: common subexpression elimination
//---------------------------------------------------------------------------

namespace {

void collect(NodePtr& n, std::unordered_map<std::string, NodePtr>& seen,
             size_t* hits) {
    if (!n) return;
    // Recurse into children first so the deepest repeat wins.
    if (n->kind == Node::Kind::CALL) {
        for (auto& arg : n->args) collect(arg, seen, hits);
    } else {
        collect(n->a, seen, hits);
        collect(n->b, seen, hits);
        collect(n->c, seen, hits);
    }
    if (n->kind == Node::Kind::CONST || n->kind == Node::Kind::VAR) return;

    std::string key = print(n);
    auto it = seen.find(key);
    if (it == seen.end()) {
        seen.emplace(std::move(key), n);
    } else if (!sameTree(it->second, n)) {
        seen[key] = n;
    } else {
        // Same shape twice: share the first copy.
        if (n.get() != it->second.get()) {
            if (hits) ++*hits;
            n = it->second;
        }
    }
}

} // namespace

NodePtr Optimizer::eliminateCommon(const NodePtr& n, size_t* hits) {
    if (!n) return nullptr;
    // Work on a copy so the caller's tree is untouched.
    NodePtr root = std::make_shared<Node>(*n);
    std::unordered_map<std::string, NodePtr> seen;
    collect(root, seen, hits);
    return root;
}

//---------------------------------------------------------------------------
// Pass 5: reassociation
//---------------------------------------------------------------------------

namespace {

// (a + c1) + c2  ->  a + (c1 + c2), so the next folding sweep collapses the
// constants. Same for SUB with a negated constant and MUL with constants.
NodePtr reassocImpl(const NodePtr& n, size_t* hits) {
    if (!n) return nullptr;
    NodePtr t = mapChildren(n, &reassocImpl, hits);
    if (t->kind != Node::Kind::BIN) return t;

    const bool leftIsAdd = t->a && t->a->kind == Node::Kind::BIN &&
                           (t->a->op == Op::ADD || t->a->op == Op::SUB);
    if (!leftIsAdd) return t;
    if (!(t->b && t->b->kind == Node::Kind::CONST)) return t;
    if (!(t->a->b && t->a->b->kind == Node::Kind::CONST)) return t;

    if (t->op == Op::ADD && t->a->op == Op::ADD) {
        Box sum = t->a->b->value + t->b->value;
        if (hits) ++*hits;
        return Bin(Op::ADD, t->a->a, Const(sum));
    }
    if (t->op == Op::ADD && t->a->op == Op::SUB) {
        Box sum = t->b->value - t->a->b->value;
        if (hits) ++*hits;
        return Bin(Op::ADD, t->a->a, Const(sum));
    }
    if (t->op == Op::SUB && t->a->op == Op::ADD) {
        Box sum = t->a->b->value - t->b->value;
        if (hits) ++*hits;
        return Bin(Op::ADD, t->a->a, Const(sum));
    }
    if (t->op == Op::SUB && t->a->op == Op::SUB) {
        Box sum = Box(Box(int64_t{0}) - t->a->b->value) - t->b->value;
        if (hits) ++*hits;
        return Bin(Op::ADD, t->a->a, Const(sum));
    }
    return t;
}

} // namespace

NodePtr Optimizer::reassociate(const NodePtr& n, size_t* hits) {
    return reassocImpl(n, hits);
}

//---------------------------------------------------------------------------
// Pipeline
//---------------------------------------------------------------------------

NodePtr Optimizer::run(const NodePtr& root) {
    report_.clear();
    if (!root) return nullptr;

    if (level_ == OptLevel::O0) {
        report_.push_back({"constant folding", 0, false});
        return root;
    }

    NodePtr t = root;

    // O1 tier.
    {
        size_t h = 0;
        t = constantFold(t, &h);
        report_.push_back({"constant folding", h, true});
    }
    {
        size_t h = 0;
        t = simplify(t, &h);
        report_.push_back({"algebraic simplification", h, true});
    }
    {
        size_t h = 0;
        t = strengthReduce(t, &h);
        report_.push_back({"strength reduction", h, true});
    }

    if (level_ >= OptLevel::O2) {
        size_t h = 0;
        t = eliminateCommon(t, &h);
        report_.push_back({"common subexpression", h, true});
    }

    if (level_ >= OptLevel::O3) {
        size_t h = 0;
        t = reassociate(t, &h);
        report_.push_back({"reassociation", h, true});
        size_t h2 = 0;
        t = constantFold(t, &h2);
        report_.push_back({"final folding sweep", h2, true});
    }

    return t;
}

std::string Optimizer::reportText() const {
    std::ostringstream os;
    os << "level " << levelName(level_) << '\n';
    for (const auto& s : report_) {
        os << "  " << s.name << ": ";
        if (!s.ran) os << "skipped";
        else os << s.rewrites << " rewrite(s)";
        os << '\n';
    }
    return os.str();
}

} // namespace jitx

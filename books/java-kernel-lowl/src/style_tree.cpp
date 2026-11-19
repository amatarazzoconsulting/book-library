// MIT License - (c) Anthony Matarazzo 2026
//
// style_tree.cpp - element tree, selector matching, cascade and inheritance.

#include "style_tree.hpp"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <stdexcept>

namespace lowl {
namespace {

std::string trim(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) --end;
    return text.substr(begin, end - begin);
}

std::vector<std::string> split_ws(const std::string& text) {
    std::vector<std::string> parts;
    std::string current;
    for (const char c : text) {
        if (std::isspace(static_cast<unsigned char>(c)) != 0) {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) parts.push_back(current);
    return parts;
}

std::string escape_text(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        switch (c) {
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '&': out += "&amp;"; break;
            case '"': out += "&quot;"; break;
            default: out.push_back(c);
        }
    }
    return out;
}

}  // namespace

// --- StyleRule --------------------------------------------------------------

StyleRule::StyleRule(std::string selector, std::size_t order)
    : selector_(std::move(selector)), order_(order) {
    compile();
}

void StyleRule::compile() {
    const std::vector<std::string> tokens = split_ws(selector_);
    if (tokens.empty()) throw std::invalid_argument("StyleRule: empty selector");
    compounds_.clear();
    child_combinator_.clear();
    specificity_ = 0;

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        bool child = false;
        std::string body = token;
        if (body == ">") {
            // "div > p" tokenises to ["div", ">", "p"].
            if (i == 0 || i + 1 >= tokens.size()) throw std::invalid_argument("StyleRule: dangling '>'");
            child = true;
            ++i;
            body = tokens[i];
        }
        Compound compound;
        if (!parse_compound(body, compound)) throw std::invalid_argument("StyleRule: bad compound '" + body + "'");
        compounds_.push_back(compound);
        child_combinator_.push_back(child);
        if (!compound.id.empty()) specificity_ += 100;
        specificity_ += static_cast<int>(compound.classes.size()) * 10;
        if (!compound.tag.empty() && compound.tag != "*") specificity_ += 1;
    }
}

bool StyleRule::parse_compound(const std::string& text, Compound& out) {
    std::size_t i = 0;
    while (i < text.size() && text[i] != '.' && text[i] != '#') out.tag.push_back(text[i++]);
    while (i < text.size()) {
        const char kind = text[i++];
        std::string name;
        while (i < text.size() && text[i] != '.' && text[i] != '#') name.push_back(text[i++]);
        if (name.empty()) return false;
        if (kind == '.') {
            out.classes.push_back(name);
        } else if (kind == '#') {
            if (!out.id.empty()) return false;
            out.id = name;
        } else {
            return false;
        }
    }
    return true;
}

bool StyleRule::compound_matches(const Compound& compound, const ElementData& element) {
    if (!compound.tag.empty() && compound.tag != "*" && compound.tag != element.tag) return false;
    if (!compound.id.empty() && compound.id != element.id) return false;
    for (const std::string& name : compound.classes) {
        if (std::find(element.classes.begin(), element.classes.end(), name) == element.classes.end()) return false;
    }
    return true;
}

bool StyleRule::matches(const ElementData& element, const std::vector<const ElementData*>& ancestors) const {
    if (compounds_.empty()) return false;
    const std::size_t last = compounds_.size() - 1;
    if (!compound_matches(compounds_[last], element)) return false;

    // Walk remaining compounds right to left over the ancestor chain.
    std::size_t index = ancestors.size();
    for (std::size_t c = last; c > 0; --c) {
        const bool direct = child_combinator_[c];
        const Compound& want = compounds_[c - 1];
        bool found = false;
        while (index > 0) {
            --index;
            if (compound_matches(want, *ancestors[index])) {
                found = true;
                break;
            }
            if (direct) break;  // child combinator only allows one step
        }
        if (!found) return false;
    }
    return true;
}

StyleRule& StyleRule::set(const std::string& property, const std::string& value) {
    if (property.empty()) throw std::invalid_argument("StyleRule::set: empty property");
    declarations_[property] = value;
    return *this;
}

// --- Element ----------------------------------------------------------------

Element::Element(std::string tag, std::string id) {
    if (tag.empty()) throw std::invalid_argument("Element: empty tag");
    data_.tag = std::move(tag);
    data_.id = std::move(id);
}

void Element::set_id(std::string id) {
    data_.id = std::move(id);
    mark_dirty();
}

void Element::add_class(const std::string& name) {
    if (name.empty()) return;
    if (has_class(name)) return;
    data_.classes.push_back(name);
    mark_dirty();
}

void Element::remove_class(const std::string& name) {
    const auto it = std::find(data_.classes.begin(), data_.classes.end(), name);
    if (it == data_.classes.end()) return;
    data_.classes.erase(it);
    mark_dirty();
}

bool Element::has_class(const std::string& name) const {
    return std::find(data_.classes.begin(), data_.classes.end(), name) != data_.classes.end();
}

void Element::set_attribute(const std::string& name, std::string value) {
    if (name.empty()) throw std::invalid_argument("Element::set_attribute: empty name");
    data_.attributes[name] = std::move(value);
    mark_dirty();
}

std::string Element::get_attribute(const std::string& name) const {
    const auto it = data_.attributes.find(name);
    return it == data_.attributes.end() ? std::string() : it->second;
}

bool Element::has_attribute(const std::string& name) const { return data_.attributes.count(name) != 0; }

void Element::set_style(const std::string& property, std::string value) {
    if (property.empty()) throw std::invalid_argument("Element::set_style: empty property");
    inline_[property] = std::move(value);
    mark_dirty();
}

std::string Element::inline_style(const std::string& property) const {
    const auto it = inline_.find(property);
    return it == inline_.end() ? std::string() : it->second;
}

void Element::append_child(std::unique_ptr<Element> child) {
    if (!child) throw std::invalid_argument("append_child: null child");
    if (child->parent_ != nullptr) throw std::invalid_argument("append_child: child already attached");
    child->parent_ = this;
    child->mark_dirty();
    children_.push_back(std::move(child));
}

bool Element::remove_child(Element* child) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() == child) {
            (*it)->parent_ = nullptr;
            children_.erase(it);
            return true;
        }
    }
    return false;
}

Element* Element::first_child() const { return children_.empty() ? nullptr : children_.front().get(); }

Element* Element::next_sibling() const {
    if (parent_ == nullptr) return nullptr;
    const auto& siblings = parent_->children_;
    for (std::size_t i = 0; i + 1 < siblings.size(); ++i) {
        if (siblings[i].get() == this) return siblings[i + 1].get();
    }
    return nullptr;
}

std::size_t Element::depth() const {
    std::size_t level = 0;
    for (const Element* node = parent_; node != nullptr; node = node->parent_) ++level;
    return level;
}

std::size_t Element::subtree_size() const {
    std::size_t total = 1;
    for (const auto& child : children_) total += child->subtree_size();
    return total;
}

void Element::set_inner_text(std::string text) { inner_text_ = std::move(text); }

std::string Element::serialize() const {
    std::string out = "<" + data_.tag;
    if (!data_.id.empty()) out += " id=\"" + escape_text(data_.id) + "\"";
    if (!data_.classes.empty()) {
        out += " class=\"";
        for (std::size_t i = 0; i < data_.classes.size(); ++i) {
            if (i != 0) out += " ";
            out += escape_text(data_.classes[i]);
        }
        out += "\"";
    }
    std::vector<std::string> names;
    names.reserve(data_.attributes.size());
    for (const auto& entry : data_.attributes) names.push_back(entry.first);
    std::sort(names.begin(), names.end());
    for (const std::string& name : names) {
        out += " " + name + "=\"" + escape_text(data_.attributes.at(name)) + "\"";
    }
    if (!inline_.empty()) {
        out += " style=\"";
        std::vector<std::string> properties;
        properties.reserve(inline_.size());
        for (const auto& entry : inline_) properties.push_back(entry.first);
        std::sort(properties.begin(), properties.end());
        for (std::size_t i = 0; i < properties.size(); ++i) {
            if (i != 0) out += "; ";
            out += properties[i] + ": " + inline_.at(properties[i]);
        }
        out += "\"";
    }
    out += ">";
    if (!inner_text_.empty()) out += escape_text(inner_text_);
    for (const auto& child : children_) out += child->serialize();
    out += "</" + data_.tag + ">";
    return out;
}

void Element::mark_dirty() {
    // A change invalidates this element and every descendant's computed style.
    dirty_ = true;
    for (const auto& child : children_) child->mark_dirty();
}

void Element::clear_dirty() {
    dirty_ = false;
    for (const auto& child : children_) child->clear_dirty();
}

// --- StyleTree --------------------------------------------------------------

StyleTree::StyleTree() {
    // CSS initial values plus the standard inheritance set.
    set_default("display", "inline");
    set_default("color", "#000000");
    set_default("font-size", "16px");
    set_default("font-weight", "400");
    set_default("line-height", "normal");
    set_default("margin", "0px");
    set_default("padding", "0px");
    set_default("opacity", "1");

    set_inherited("color");
    set_inherited("font-size");
    set_inherited("font-weight");
    set_inherited("line-height");
    set_inherited("font-family");
}

void StyleTree::set_root(std::unique_ptr<Element> root) {
    if (!root) throw std::invalid_argument("set_root: null root");
    root_ = std::move(root);
    root_->detach_from_parent();
    root_->mark_dirty();
}

std::unique_ptr<Element> StyleTree::create_element(const std::string& tag, const std::string& id) {
    return std::unique_ptr<Element>(new Element(tag, id));
}

Element* StyleTree::append(Element* parent, std::unique_ptr<Element> child) {
    if (parent == nullptr) {
        if (root_ != nullptr) throw std::invalid_argument("append: root already set");
        Element* raw = child.get();
        set_root(std::move(child));
        return raw;
    }
    Element* raw = child.get();
    parent->append_child(std::move(child));
    return raw;
}

namespace {

void collect_by_id(const Element* node, const std::string& id, Element*& found) {
    if (found != nullptr) return;
    if (node->id() == id) {
        found = const_cast<Element*>(node);
        return;
    }
    for (const auto& child : node->children()) collect_by_id(child.get(), id, found);
}

void collect_matching(const Element* node, const std::function<bool(const Element&)>& predicate,
                      std::vector<Element*>& out) {
    if (predicate(*node)) out.push_back(const_cast<Element*>(node));
    for (const auto& child : node->children()) collect_matching(child.get(), predicate, out);
}

}  // namespace

Element* StyleTree::by_id(const std::string& id) const {
    Element* found = nullptr;
    if (root_ != nullptr) collect_by_id(root_.get(), id, found);
    return found;
}

std::vector<Element*> StyleTree::by_class(const std::string& name) const {
    std::vector<Element*> out;
    if (root_ == nullptr) return out;
    collect_matching(root_.get(), [&name](const Element& e) { return e.has_class(name); }, out);
    return out;
}

std::vector<Element*> StyleTree::by_tag(const std::string& tag) const {
    std::vector<Element*> out;
    if (root_ == nullptr) return out;
    collect_matching(root_.get(), [&tag](const Element& e) { return e.tag() == tag; }, out);
    return out;
}

std::vector<Element*> StyleTree::query(const std::string& selector) const {
    StyleRule probe(selector);
    std::vector<Element*> out;
    if (root_ == nullptr) return out;

    // Pre-order walk carrying the ancestor chain so descendant selectors work.
    // Children are pushed in reverse so the pop order is document order.
    struct Frame {
        const Element* node;
        std::vector<const ElementData*> ancestors;
    };
    std::vector<Frame> stack;
    stack.push_back(Frame{root_.get(), {}});
    while (!stack.empty()) {
        Frame frame = std::move(stack.back());
        stack.pop_back();
        if (probe.matches(frame.node->data(), frame.ancestors)) out.push_back(const_cast<Element*>(frame.node));
        std::vector<const ElementData*> chain = frame.ancestors;
        chain.push_back(&frame.node->data());
        const auto& children = frame.node->children();
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            stack.push_back(Frame{it->get(), chain});
        }
    }
    return out;
}

void StyleTree::add_rule(StyleRule rule) {
    if (rule.declarations().empty()) throw std::invalid_argument("add_rule: rule has no declarations");
    rules_.push_back(std::move(rule));
    invalidate_all();
}

void StyleTree::clear_rules() {
    rules_.clear();
    invalidate_all();
}

void StyleTree::set_inherited(const std::string& property, bool inherited) { inherited_[property] = inherited; }

bool StyleTree::is_inherited(const std::string& property) const {
    const auto it = inherited_.find(property);
    return it != inherited_.end() && it->second;
}

void StyleTree::set_default(const std::string& property, const std::string& value) { defaults_[property] = value; }

void StyleTree::invalidate(Element* element) {
    if (element == nullptr) return;
    element->mark_dirty();
    cache_.erase(element);
}

void StyleTree::invalidate_all() {
    if (root_ == nullptr) return;
    root_->mark_dirty();
    cache_.clear();
}

std::vector<const ElementData*> StyleTree::ancestry(const Element* element) const {
    std::vector<const ElementData*> chain;
    for (const Element* node = element->parent(); node != nullptr; node = node->parent()) {
        chain.push_back(&node->data());
    }
    std::reverse(chain.begin(), chain.end());  // root first
    return chain;
}

void StyleTree::recompute(Element* element) {
    std::map<std::string, std::string>& out = cache_[element];
    out.clear();

    // 1. Cascade: collect every matching declaration with (specificity, order).
    struct Candidate {
        int specificity;
        std::size_t order;
        const std::string* value;
    };
    std::unordered_map<std::string, Candidate> winners;
    const std::vector<const ElementData*> chain = ancestry(element);
    for (const StyleRule& rule : rules_) {
        ++stats_.rule_visits;
        if (!rule.matches(element->data(), chain)) continue;
        for (const auto& declaration : rule.declarations()) {
            const Candidate candidate{rule.specificity(), rule.order(), &declaration.second};
            const auto it = winners.find(declaration.first);
            if (it == winners.end() || candidate.specificity > it->second.specificity ||
                (candidate.specificity == it->second.specificity && candidate.order >= it->second.order)) {
                winners[declaration.first] = candidate;
            }
        }
    }
    for (const auto& entry : winners) out[entry.first] = *entry.second.value;

    // 2. Inline styles beat every rule.
    for (const auto& entry : element->inline_styles()) out[entry.first] = entry.second;

    // 3. Inheritance: inherited properties fall back to the parent's computed value.
    if (element->parent() != nullptr) {
        for (const auto& entry : inherited_) {
            if (!entry.second) continue;
            if (out.count(entry.first) != 0) continue;
            const std::map<std::string, std::string>& parent_style = computed(element->parent());
            const auto parent_value = parent_style.find(entry.first);
            if (parent_value == parent_style.end()) continue;
            out[entry.first] = parent_value->second;
            ++stats_.inheritance_pulls;
        }
    }

    // 4. Defaults for anything still unset.
    for (const auto& entry : defaults_) {
        if (out.count(entry.first) == 0) out[entry.first] = entry.second;
    }
    ++stats_.recomputed;
}

const std::map<std::string, std::string>& StyleTree::computed(Element* element) {
    if (element == nullptr) throw std::invalid_argument("computed: null element");
    const auto cached = cache_.find(element);
    if (cached != cache_.end() && !element->dirty()) {
        ++stats_.cache_hits;
        return cached->second;
    }
    recompute(element);
    element->clear_dirty();
    return cache_[element];
}

std::string StyleTree::computed_value(Element* element, const std::string& property) {
    const auto& style = computed(element);
    const auto it = style.find(property);
    return it == style.end() ? std::string() : it->second;
}

double StyleTree::computed_number(Element* element, const std::string& property, double fallback) {
    const std::string text = trim(computed_value(element, property));
    if (text.empty()) return fallback;
    errno = 0;
    char* end = nullptr;
    const double value = std::strtod(text.c_str(), &end);
    if (end == nullptr || end == text.c_str()) return fallback;
    return value;
}

std::size_t StyleTree::node_count() const { return root_ == nullptr ? 0 : root_->subtree_size(); }

std::size_t StyleTree::max_depth() const {
    if (root_ == nullptr) return 0;
    std::size_t best = 0;
    std::vector<std::pair<const Element*, std::size_t>> stack;
    stack.emplace_back(root_.get(), 1);
    while (!stack.empty()) {
        const auto frame = stack.back();
        stack.pop_back();
        best = std::max(best, frame.second);
        for (const auto& child : frame.first->children()) stack.emplace_back(child.get(), frame.second + 1);
    }
    return best;
}

}  // namespace lowl

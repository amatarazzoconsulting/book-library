// MIT License - (c) Anthony Matarazzo 2026
//
// style_tree.hpp - the DOM element tree with CSS inheritance and cascade.
//
// lowl.java.dom.Document / Element sit on top of this.  Each element carries
// inline declarations; a style sheet adds rules whose selectors are matched
// against the element (tag, .class, #id, descendant, child).  Computed values
// are produced by the cascade (specificity, then source order), then inherited
// properties are pulled from the parent, then defaults fill the rest.  Because
// inherited values only flow down, a computed style can be memoised per element
// and invalidated with a subtree dirty flag instead of a full re-walk.

#ifndef LOWL_STYLE_TREE_HPP
#define LOWL_STYLE_TREE_HPP

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace lowl {

// A declaration value.  Kept as text, like CSS, plus a numeric view for
// computed geometry ("20px" -> 20.0).
struct StyleValue {
    std::string text;
    bool is_inherited = false;
};

// Everything an element needs to answer selector matching.
struct ElementData {
    std::string tag;
    std::string id;
    std::vector<std::string> classes;
    std::unordered_map<std::string, std::string> attributes;
};

class StyleRule {
public:
    // selector examples: "p", ".card", "#main", "div p", "div > p", "div.note p"
    explicit StyleRule(std::string selector, std::size_t order = 0);

    const std::string& selector() const { return selector_; }
    std::size_t order() const { return order_; }
    int specificity() const { return specificity_; }
    const std::map<std::string, std::string>& declarations() const { return declarations_; }

    StyleRule& set(const std::string& property, const std::string& value);
    // True when this rule's selector matches `element` in the given ancestry.
    bool matches(const ElementData& element, const std::vector<const ElementData*>& ancestors) const;

private:
    struct Compound {
        std::string tag;                   // empty means any tag
        std::string id;                    // empty means any id
        std::vector<std::string> classes;  // all must be present
    };

    void compile();
    static bool compound_matches(const Compound& compound, const ElementData& element);
    static bool parse_compound(const std::string& text, Compound& out);

    std::string selector_;
    std::size_t order_;
    int specificity_ = 0;
    std::vector<Compound> compounds_;  // left to right, last is the subject
    std::vector<bool> child_combinator_;  // combinator before compound i
    std::map<std::string, std::string> declarations_;
};

class Element {
public:
    Element(std::string tag, std::string id = "");

    const std::string& tag() const { return data_.tag; }
    const std::string& id() const { return data_.id; }
    const ElementData& data() const { return data_; }

    void set_id(std::string id);
    void add_class(const std::string& name);
    void remove_class(const std::string& name);
    bool has_class(const std::string& name) const;
    void set_attribute(const std::string& name, std::string value);
    std::string get_attribute(const std::string& name) const;
    bool has_attribute(const std::string& name) const;

    void set_style(const std::string& property, std::string value);
    std::string inline_style(const std::string& property) const;
    const std::unordered_map<std::string, std::string>& inline_styles() const { return inline_; }

    void append_child(std::unique_ptr<Element> child);
    bool remove_child(Element* child);
    Element* parent() const { return parent_; }
    const std::vector<std::unique_ptr<Element>>& children() const { return children_; }
    std::size_t child_count() const { return children_.size(); }
    Element* first_child() const;
    Element* next_sibling() const;
    std::size_t depth() const;
    std::size_t subtree_size() const;

    void set_inner_text(std::string text);
    const std::string& inner_text() const { return inner_text_; }
    std::string serialize() const;

    void mark_dirty();
    bool dirty() const { return dirty_; }
    void clear_dirty();
    void detach_from_parent() { parent_ = nullptr; }

private:
    ElementData data_;
    Element* parent_ = nullptr;
    std::vector<std::unique_ptr<Element>> children_;
    std::unordered_map<std::string, std::string> inline_;
    std::string inner_text_;
    bool dirty_ = true;
};

class StyleTree {
public:
    StyleTree();

    Element* root() { return root_.get(); }
    const Element* root() const { return root_.get(); }

    void set_root(std::unique_ptr<Element> root);
    // Owned factory: the returned pointer is valid until the tree is destroyed.
    static std::unique_ptr<Element> create_element(const std::string& tag, const std::string& id = "");
    Element* append(Element* parent, std::unique_ptr<Element> child);
    Element* by_id(const std::string& id) const;
    std::vector<Element*> by_class(const std::string& name) const;
    std::vector<Element*> by_tag(const std::string& tag) const;
    std::vector<Element*> query(const std::string& selector) const;

    void add_rule(StyleRule rule);
    std::size_t rule_count() const { return rules_.size(); }
    void clear_rules();
    // Marks a property as inherited (CSS `inherit` set).
    void set_inherited(const std::string& property, bool inherited = true);
    bool is_inherited(const std::string& property) const;
    void set_default(const std::string& property, const std::string& value);

    // Cascade + inheritance + defaults.  Memoised until the element is dirty.
    const std::map<std::string, std::string>& computed(Element* element);
    std::string computed_value(Element* element, const std::string& property);
    double computed_number(Element* element, const std::string& property, double fallback = 0.0);

    void invalidate(Element* element);
    void invalidate_all();

    struct Stats {
        std::size_t recomputed = 0;     // elements whose style was rebuilt
        std::size_t cache_hits = 0;
        std::size_t rule_visits = 0;    // selector match attempts
        std::size_t inheritance_pulls = 0;
    };
    Stats stats() const { return stats_; }
    void reset_stats() { stats_ = Stats{}; }
    std::size_t node_count() const;
    std::size_t max_depth() const;

private:
    std::vector<const ElementData*> ancestry(const Element* element) const;
    void recompute(Element* element);

    std::unique_ptr<Element> root_;
    std::vector<StyleRule> rules_;
    std::unordered_map<Element*, std::map<std::string, std::string>> cache_;
    std::unordered_map<std::string, bool> inherited_;
    std::unordered_map<std::string, std::string> defaults_;
    Stats stats_;
};

}  // namespace lowl

#endif  // LOWL_STYLE_TREE_HPP

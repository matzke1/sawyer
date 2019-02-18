// A single small header file defines the tree API.
#include <Sawyer/Tree.h>

#include <Sawyer/Assert.h>
#include <iostream>
#include <string>
#include <vector>

using namespace Sawyer;

#define COMPILE_ERROR(X) X

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Declare an AST leaf node which has no children.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Leaf: public Tree::Node {
    // Data members, member types, and member functions are declared like normal.
    int value_;

public:
    // Constructors are normal, at least for leaf nodes, as are copying and assignment. No special destructor is needed.
    Leaf() {}

    explicit Leaf(int value)
        : value_(value) {}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Declare an internal node that points to a first and second leaf node.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PairOfLeaves: public Tree::Node {
public:
    // The pointers to the children are declared with a special pointer type. You can only use these types for data members,
    // and only in classes derived from Tree::Node.
    Tree::ChildEdge<Leaf> first;
    Tree::ChildEdge<Leaf> second;

    // The child pointers don't have default constructors, therefore you must define a default constructor for PairOfLeaves.
    // The order of their declarations (and thus initializations) defines the order of the traversals we'll see later. I.e.,
    // when traversing a Tree and we get to a PairOfLeaves node, the traversal will follow 'first' and then 'second' since
    // that's the order they're declared.
    PairOfLeaves()
        : first(this), second(this) {}
    
    // Additional constructors are permitted
    PairOfLeaves(const std::shared_ptr<Leaf> &first, const std::shared_ptr<Leaf> &second)
        : first(this, first), second(this, second) {}

    // No special destructor is necessary, although you can create one if you have other data to destroy.  Destroying an AST
    // node automatically unlinks the children from the node and resets their parent pointers to null.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Constructing a node with child pointers initializes the child pointers to null.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_childptr_init() {
    // Like all good C++ programs, pointers should be wrapped in varous <memory> header file types. The Tree itself will use
    // std::shared_ptr to link parents to children, and std::weak_ptr to link children to parents. From the user's perspective,
    // if you use std::shared_ptr for all your nodes you'll be in good shape.
    auto node = std::make_shared<PairOfLeaves>();

    // Pointers are always initialized in good C++ programming. We actually get this for free since Tree is using pointer types
    // declared in the <memory> header file.
    ASSERT_always_require(node->first == nullptr);
    ASSERT_always_require(node->second == nullptr);
    ASSERT_always_require(node->parent == nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Assignment of node as the child of a parent adjusts the child's parent pointer.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_parent_adjustment() {
    auto a = std::make_shared<Leaf>();
    auto b = std::make_shared<Leaf>();
    auto pair = std::make_shared<PairOfLeaves>();

    // Assign a child to the parent
    pair->first = a;
    ASSERT_always_require(pair->first == a);
    ASSERT_always_require(a->parent == pair);           // parent was set automatically
    ASSERT_always_require(b->parent == nullptr);

    // Assign a different child adjusts the parent of the old and new child
    pair->first = b;
    ASSERT_always_require(pair->first == b);
    ASSERT_always_require(a->parent == nullptr);        // parent was cleared automatically
    ASSERT_always_require(b->parent == pair);

    // Assigning the same child has no effect
    pair->first = b;
    ASSERT_always_require(pair->first == b);
    ASSERT_always_require(a->parent == nullptr);
    ASSERT_always_require(b->parent == pair);

    // Assigning null changes the old child's parent
    pair->first = nullptr;
    ASSERT_always_require(pair->first == nullptr);
    ASSERT_always_require(a->parent == nullptr);
    ASSERT_always_require(b->parent == nullptr);

    // Assigning null to a null pointer has no effect
    pair->first = nullptr;
    ASSERT_always_require(pair->first == nullptr);
    ASSERT_always_require(a->parent == nullptr);
    ASSERT_always_require(b->parent == nullptr);

    // Assign child again
    pair->first = a;
    ASSERT_always_require(pair->first == a);
    ASSERT_always_require(a->parent == pair);
    ASSERT_always_require(b->parent == nullptr);

    // Clear by using reset()
    pair->first.reset();
    ASSERT_always_require(pair->first == nullptr);
    ASSERT_always_require(a->parent == nullptr);
    ASSERT_always_require(b->parent == nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: A node cannot be attached to more than one point in the tree.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_multi_parent() {
    auto a = std::make_shared<Leaf>();
    auto pair = std::make_shared<PairOfLeaves>();

    pair->first = a;

    // Attempting to attach "a" to some other point in a tree will throw an exception and not change anything.  If node "a"
    // were allowed to be set as both pair->first and pair-second, then a traversal of the tree would reach node "a" two times,
    // which is a violation of the definition of "tree".
    try {
        pair->second = a;
        ASSERT_always_require(!"should not get here");
    } catch (const Tree::ConsistencyException &error) {
        ASSERT_always_require(pair->first == a);
        ASSERT_always_require(a->parent == pair);
        ASSERT_always_require(pair->second == nullptr);
        ASSERT_always_require(error.child == a);
    }

    // A node cannot be attached to a different tree. If node "a" were allowed to be attached to two different nodes, then it
    // would need to parent pointers. Having two parent pointers violates the definition of "tree".
    auto otherTree = std::make_shared<PairOfLeaves>();
    try {
        otherTree->first = a;
        ASSERT_always_require(!"should not get here");
    } catch (const Tree::ConsistencyException &error) {
        ASSERT_always_require(pair->first == a);
        ASSERT_always_require(a->parent == pair);
        ASSERT_always_require(otherTree->first == nullptr);
        ASSERT_always_require(error.child == a);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Illegal to assign to parent.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_parent_assignment() {
#if 0 // Intentionally does not compile.
    auto a = std::make_shared<Leaf>();
    auto pair = std::make_shared<PairOfLeaves>();

    // Assigning directly to the parent is prohibited because it is often ambiguous (as in this example, where we can't tell
    // whether the intent is to make a the first or second child of the pair.
    COMPILE_ERROR(a->parent = pair); // Tree::ParentPtr::operator=(Tree::Node*) is private (could be a better message in C++11)

    // Setting a parent to null is also disallowed since implementing it would make the library significantly more complicated.
    COMPILE_ERROR(a->parent = nullptr); // Tree::ParentPtr::operator=(Tree::Node*) is private (could be a better message in C++11)
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: All child pointers are also available in a vector for iteration.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_child_vector() {
    auto a = std::make_shared<Leaf>();
    auto b = std::make_shared<Leaf>();
    auto pair = std::make_shared<PairOfLeaves>();

    // The number of children is determined by the declaration, not whether they are null pointers or not.
    ASSERT_always_require(pair->children.size() == 2);
    ASSERT_always_require(pair->children[0] == nullptr); // pair->children[0], a.k.a. pair->first  (see below)
    ASSERT_always_require(pair->children[1] == nullptr); // pair->children[1], a.k.a. pair->second (see below)

    // The order of the children are the same as the order they're declared in the class, not the order they're assigned.
    pair->second = b;
    pair->first = a;
    ASSERT_always_require(pair->children.size() == 2);
    ASSERT_always_require(pair->children[0] == a);
    ASSERT_always_require(pair->children[1] == b);

    // Removing the child from the parent removes it from the vector without changing the vector's size.
    pair->first = nullptr;
    ASSERT_always_require(pair->children.size() == 2);
    ASSERT_always_require(pair->children[0] == nullptr);
    ASSERT_always_require(pair->children[1] == b);

    // Child vectors operate as expected (although they're not actual std::vector (we'll see why shortly)
    auto pair2 = std::make_shared<PairOfLeaves>();
    ASSERT_always_require(pair->children.size() == 2);
    ASSERT_always_require(pair->children.max_size() >= 2);
    ASSERT_always_require(pair->children.capacity() >= 2);
    ASSERT_always_require(!pair->children.empty());
    pair->children.reserve(10);                         // permitted since it doesn't modify values
    ASSERT_always_require(pair->children[0] == nullptr);
    ASSERT_always_require(pair->children.at(0) == nullptr);
    ASSERT_always_require(pair->children.front() == nullptr);
    ASSERT_always_require(pair->children.back() == b);
    ASSERT_always_require(!(pair->children == pair2->children));
    ASSERT_always_require(pair->children != pair2->children);
    ASSERT_always_require(pair->children <= pair2->children || pair->children > pair2->children);
    ASSERT_always_require(pair->children < pair2->children || pair->children >= pair2->children);
    pair->children.shrink_to_fit();                     // permitted since it doesn't modify values
    //ASSERT_always_require(pair->children.data() != nullptr); -- not implemented

    // TODO: [Robb Matzke 2018-12-16]: iterators are not yet implemented
    std::cerr <<"Node::children iterators are not implemented yet\n";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Child vector is not modifiable by the user.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_const_child_vector() {
#if 0 // These intentionally don't compile
    auto pair = std::make_shared<PairOfLeaves>();
    auto pair2 = std::make_shared<PairOfLeaves>();

    // We don't allow modifications to the vector of child pointers for the same reasons we don't allow setting the
    // parent pointers explicitly. Namely, allowing it would substantially complicate the implementation and is not necessary.
    COMPILE_ERROR(pair->children.resize(10));           // class Tree::NodeVector has no member named 'resize'
    COMPILE_ERROR(pair->children[0] = nullptr);            // lvalue required as left operand of assignment
    COMPILE_ERROR(pair->children.at(0) = nullptr);         // lvalue required as left operand of assignment
    COMPILE_ERROR(pair->children.front() = nullptr);       // lvalue required as left operand of assignment
    COMPILE_ERROR(pair->children.back() = nullptr);        // lvalue required as left operand of assignment
    COMPILE_ERROR(pair->children.assign(i, nullptr));      // class Tree::NodeVector has no member named 'assign'
    COMPILE_ERROR(pair->children.push_back(nullptr));      // Tree::NodeVector::push_back is private
    COMPILE_ERROR(pair->children.pop_back(nullptr));       // class Tree::NodeVector has no member named 'pop_back'
    COMPILE_ERROR(pair->children.insert());             // class Tree::NodeVector has no member named 'insert'
    COMPILE_ERROR(pair->children.swap(pair->children)); // class Tree::NodeVector has no member named 'swap'
    COMPILE_ERROR(pair->children.clear());              // class Tree::NodeVector has no member named 'clear'
    COMPILE_ERROR(std::swap(pair->children, pair2->children)); // undefined reference to std::swap<Tree::NodeVector>
#if __cplusplus >= 201103L
    COMPILE_ERROR(pair->children.data()[0] = nullptr);     // assignment of read-only location
    COMPILE_ERROR(pair->children.emplace());            // class Tree::NodeVector has no member named 'emplace'
    COMPILE_ERROR(pair->children.emplace_back());       // class Tree::NodeVector has no member named 'emplace_back'
#endif
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Creating a cycle in the tree is not permitted.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A simple node that just points to another node.
class PtrNode: public Tree::Node {
public:
    Tree::ChildEdge<Tree::Node> child;

    PtrNode()
        : child(this) {}
};

static void test_cycle() {
    auto a = std::make_shared<PtrNode>();
    auto b = std::make_shared<PtrNode>();

    // A node cannot be a child of itself (singleton cycle) since this violates the definition of "tree".
    try {
        a->child = a;
        ASSERT_not_reachable("self-parent should have failed");
    } catch (const Tree::ConsistencyException &e) {
        ASSERT_always_require(a->child == nullptr);
        ASSERT_always_require(a->parent == nullptr);
    }

    // It is also illegal to make a larger cycle.
    a->child = b;
    try {
        b->child = a;
        ASSERT_not_reachable("cycle should have been prevented");
    } catch (const Tree::ConsistencyException &e) {
        ASSERT_always_require(a->child == b);
        ASSERT_always_require(b->parent == a);
        ASSERT_always_require(b->child == nullptr);
        ASSERT_always_require(a->parent == nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: AST nodes with children are not copyable.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_noncopyable() {
#if 0 // Intentionally doesn't compile
    auto a = std::make_shared<Leaf>();
    auto b = std::make_shared<Leaf>();
    auto p1 = std::make_shared<PairOfLeaves>(a, b);
    auto p2 = std::make_shared<PairOfLeaves>();

    // It makes no sense to copy an AST node since it's an ill-defined operation. Users might reasonably assume any of these
    // semantics:
    //   1. The child AST are recursively copied, a so-called "deep copy".
    //   2. The copy is shallow, but p2 will be given null child pointers since nodes a and b are already attached to p1
    //      and can only have one parent.
    //   3. The copy is shallow, and a and b have been moved from p1 to p2; p1's children are now nulls.
    // Instead, all of these semantics can be implemented as member functions with more descriptive names than operator=.
    COMPILE_ERROR(*p2 = *p1); // Tree::Node::operator=(const Tree::Node&) is private (C++11 could have a better error)
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Child and parent pointers are implicitly convertible to bool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_convert_to_bool() {
    auto pair = std::make_shared<PairOfLeaves>();

    // Child pointers are convertible to Boolean
    if (pair->first)
        ASSERT_not_reachable("test failed");

    while (pair->first)
        ASSERT_not_reachable("test failed");

    for (size_t i=0; pair->first; ++i)
        ASSERT_not_reachable("test failed");

    int i = 0;
    do {
        ASSERT_always_require(i == 0);
        ++i;
    } while (pair->first);

    ASSERT_always_require(pair->first ? 0 : 1);

    bool b1 = (bool) pair->first;
    ASSERT_always_require(!b1);

    b1 = (bool) pair->first;
    ASSERT_always_require(!b1);

    // Parent pointers are convetible to Boolean
    if (pair->parent)
        ASSERT_not_reachable("test failed");

    while (pair->parent)
        ASSERT_not_reachable("test failed");

    for (size_t i=0; pair->parent; ++i)
        ASSERT_not_reachable("test failed");

    i = 0;
    do {
        ASSERT_always_require(i == 0);
        ++i;
    } while (pair->parent);

    ASSERT_always_require(pair->parent ? 0 : 1);

    bool b2 = (bool) pair->parent;
    ASSERT_always_require(!b2);

    b2 = (bool) pair->parent;
    ASSERT_always_require(!b2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Pointers are not implicitly convertible to integers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_convert_to_int() {
#if 0 // Intentionally does not compile
    auto a = std::make_shared<Leaf>();

    // Implicit conversion to bool is easy to get wrong, resulting in such classes also implicitly converting to integers,
    // especially before C++11. This implementation avoids that error.
    COMPILE_ERROR(int b = a);

    static int array[5];
    COMPILE_ERROR(int c = array[a]);
    COMPILE_ERROR(int d = array[a+0]);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Child pointers can be used in logic expressions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_ptr_logic() {
    auto pair = std::make_shared<PairOfLeaves>();
    auto p2 = std::make_shared<PairOfLeaves>();

    // Child pointers in logic expressions
    ASSERT_always_require(pair->first == pair->second);
    ASSERT_always_require(pair->first <= pair->second);
    ASSERT_always_require(pair->first >= pair->second);
    ASSERT_always_require((pair->first && pair->second) == (pair->first || pair->second));
    ASSERT_always_require(!(pair->first < pair->second) && !(pair->first > pair->second));

    // Parent pointers in logic expressions
    ASSERT_always_require(pair->parent == p2->parent);
    ASSERT_always_require(pair->parent <= p2->parent);
    ASSERT_always_require(pair->parent >= p2->parent);
    ASSERT_always_require((pair->parent && p2->parent) == (pair->parent || p2->parent));
    ASSERT_always_require(!(pair->parent < p2->parent) && !(pair->parent > p2->parent));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Node types can be derived from other node types.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Create a small class hiearchy for expression trees. It contains one class that's the base type for all expression nodes, and
// two subtypes: one for variables and another for binary operations like "a+b" and "a-b".
class ExprBase: public Tree::Node {
public:
    Tree::ChildEdge<Leaf> leaf; // just something to demo that child ptrs can appear at multiple levels in the class hierarchy
    ExprBase(): leaf(this) {}
};

class Variable: public ExprBase {
public:
    std::string name;
    Variable() {}
    explicit Variable(const std::string &name): name(name) {}
};

class BinaryExpr: public ExprBase {
public:
    enum Operator { PLUS, MINUS };
    Operator op;
    Tree::ChildEdge<ExprBase> lhs, rhs;
    BinaryExpr()
        : op(PLUS), lhs(this), rhs(this) {}
    BinaryExpr(Operator op, const std::shared_ptr<ExprBase> &lhs, const std::shared_ptr<ExprBase> &rhs)
        : op(op), lhs(this, lhs), rhs(this, rhs) {}
};

static void test_derived_classes() {
    // Create the expression: (x + y) - z
    auto x = std::make_shared<Variable>("x");
    auto y = std::make_shared<Variable>("y");
    auto z = std::make_shared<Variable>("z");
    auto sum = std::make_shared<BinaryExpr>(BinaryExpr::PLUS, x, y);
    auto diff = std::make_shared<BinaryExpr>(BinaryExpr::MINUS, sum, z);

    ASSERT_always_require(sum->lhs == x);
    ASSERT_always_require(sum->rhs == y);
    ASSERT_always_require(diff->lhs == sum);
    ASSERT_always_require(diff->rhs == z);
    ASSERT_always_require(x->parent == sum);
    ASSERT_always_require(y->parent == sum);
    ASSERT_always_require(z->parent == diff);
    ASSERT_always_require(sum->parent == diff);
    ASSERT_always_require(diff->parent == nullptr);

    // Children of a super class are ordered before children of derived classes. This is just the order-of-initialization
    // rule applied across the class hierarchy.
    ASSERT_always_require(sum->children.size() == 3);
    ASSERT_always_require(sum->children[0] == nullptr);                                 // data member "leaf"
    ASSERT_always_require(sum->children[1] == x);
    ASSERT_always_require(sum->children[2] == y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: A derived pointer can be assigned to a base pointer.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_derived_ptr_assign() {
    auto derived = std::make_shared<Variable>();
    auto add = std::make_shared<BinaryExpr>();

    // Derived pointer assigned to base pointer
    add->lhs = derived;                                                                 // down cast
    ASSERT_always_require(add->lhs == derived);
    ASSERT_always_require(derived->parent == add);

    // The child edge 'add->lhs' is convertible to a shared pointer
    std::shared_ptr<ExprBase> base = add->lhs;                                          // down cast
    ASSERT_always_require(base == derived);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Base pointers can be up-cast
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_upcast() {
    auto add = std::make_shared<BinaryExpr>(BinaryExpr::PLUS, std::make_shared<Variable>("x"), std::make_shared<Variable>("y"));

    std::shared_ptr<ExprBase> raw = add->lhs;                                           // down cast
    std::shared_ptr<Variable> derived = std::dynamic_pointer_cast<Variable>(raw);       // up cast
    ASSERT_always_require(derived != nullptr);
    
    // What about smart pointers? It turns out that it's only possible to override dynamic casts for std::shared_ptr, and
    // that's only in C++11 and later. Therefore we're stuck with a mild annoyance. Good C++ code generally avoids dynamic
    // casts anyway, so probably not that big a deal.

#if 0 // [Robb Matzke 2019-02-17]
    derived = std::dynamic_pointer_cast<Variable>(add->lhs); // doesn't compile although we'd like it to.
#else
    derived = std::dynamic_pointer_cast<Variable>(add->lhs.shared());
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Pre-post downward (child-following) traversals
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void test_downward_traversal() {
    // Create the expression: (x + y) - z
    auto x = std::make_shared<Variable>("x");
    auto y = std::make_shared<Variable>("y");
    auto z = std::make_shared<Variable>("z");
    auto sum = std::make_shared<BinaryExpr>(BinaryExpr::PLUS, x, y);
    auto diff = std::make_shared<BinaryExpr>(BinaryExpr::MINUS, sum, z);

    // Make a list of the traversal steps. Each step is an event and the affected node.
    std::vector<std::pair<Tree::TraversalEvent, Tree::NodePtr > > processed;
    diff->traverse([&processed](const Tree::NodePtr &node, Tree::TraversalEvent event) -> Tree::TraversalAction {
            processed.push_back(std::make_pair(event, node));
            return Tree::CONTINUE;
        });

    ASSERT_always_require(processed.size() == 10);      // enter and leave each node exactly once
    ASSERT_always_require(processed[0].first == Tree::ENTER && processed[0].second == diff);
    ASSERT_always_require(processed[1].first == Tree::ENTER && processed[1].second == sum);
    ASSERT_always_require(processed[2].first == Tree::ENTER && processed[2].second == x);
    ASSERT_always_require(processed[3].first == Tree::LEAVE && processed[3].second == x);
    ASSERT_always_require(processed[4].first == Tree::ENTER && processed[4].second == y);
    ASSERT_always_require(processed[5].first == Tree::LEAVE && processed[5].second == y);
    ASSERT_always_require(processed[6].first == Tree::LEAVE && processed[6].second == sum);
    ASSERT_always_require(processed[7].first == Tree::ENTER && processed[7].second == z);
    ASSERT_always_require(processed[8].first == Tree::LEAVE && processed[8].second == z);
    ASSERT_always_require(processed[9].first == Tree::LEAVE && processed[9].second == diff);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Upward traversals (following parent pointers) are identical to downward traversals, just a different function name.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void test_upward_traversal() {
    // Create the expression: (x + y) - z
    auto x = std::make_shared<Variable>("x");
    auto y = std::make_shared<Variable>("y");
    auto z = std::make_shared<Variable>("z");
    auto sum = std::make_shared<BinaryExpr>(BinaryExpr::PLUS, x, y);
    auto diff = std::make_shared<BinaryExpr>(BinaryExpr::MINUS, sum, z);

    // Make a list of the traversal steps. Each step is an event and the affected node.
    std::vector<std::pair<Tree::TraversalEvent, Tree::NodePtr > > processed;
    x->traverseParents([&processed](const Tree::NodePtr &node, Tree::TraversalEvent event) {
            processed.push_back(std::make_pair(event, node));
            return Tree::CONTINUE;
        });

    ASSERT_always_require(processed.size() == 6);
    ASSERT_always_require(processed[0].first == Tree::ENTER && processed[0].second == x);
    ASSERT_always_require(processed[1].first == Tree::ENTER && processed[1].second == sum);
    ASSERT_always_require(processed[2].first == Tree::ENTER && processed[2].second == diff);
    ASSERT_always_require(processed[3].first == Tree::LEAVE && processed[3].second == diff);
    ASSERT_always_require(processed[4].first == Tree::LEAVE && processed[4].second == sum);
    ASSERT_always_require(processed[5].first == Tree::LEAVE && processed[5].second == x);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Search for nodes by following child pointers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_find_downward() {
    // Create the expression: (x + y) - z
    auto x = std::make_shared<Variable>("x");
    auto y = std::make_shared<Variable>("y");
    auto z = std::make_shared<Variable>("z");
    auto sum = std::make_shared<BinaryExpr>(BinaryExpr::PLUS, x, y);
    auto diff = std::make_shared<BinaryExpr>(BinaryExpr::MINUS, sum, z);

    // Find first node that's a variable. TODO: demo this with C++03 functor classes and functions
    Tree::NodePtr found = diff->find([](const Tree::NodePtr &node) {
            return std::dynamic_pointer_cast<Variable>(node);
        });
    ASSERT_always_require(found == x);

    // Since this happens so frequently and is verbose in C++03, there's a shortcut.
    auto var = diff->findType<Variable>();
    ASSERT_always_require(var == x);

    // Find first node that's a variable with the name "y". TODO: demo C++03
    found = diff->find([](const Tree::NodePtr &node) {
            std::shared_ptr<Variable> v = std::dynamic_pointer_cast<Variable>(node);
            return v && v->name == "y";
        });
    ASSERT_always_require(found == y);

    // Since finding a node of a particular type that satisfies some predicate is so common, there's a shortcut. TODO: demo C++03
    var = diff->findType<Variable>([](const std::shared_ptr<Variable> &v) { return v->name == "y"; });
    ASSERT_always_require(var == y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Search for nodes by following parent pointers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_find_upward() {
    // Create the expression: (x + y) - z
    auto x = std::make_shared<Variable>("x");
    auto y = std::make_shared<Variable>("y");
    auto z = std::make_shared<Variable>("z");
    auto sum = std::make_shared<BinaryExpr>(BinaryExpr::PLUS, x, y);
    auto diff = std::make_shared<BinaryExpr>(BinaryExpr::MINUS, sum, z);

    // Find closest ancestor that's a BinaryExpr. TODO: demo this with C++03 functor classes and functions
    Tree::NodePtr found = x->findParent([](const Tree::NodePtr &node) {
            return std::dynamic_pointer_cast<BinaryExpr>(node); });
    ASSERT_always_require(found == sum);

    // Since this happens frequently and is quite verbose for C++03, there's a shortcut.
    std::shared_ptr<BinaryExpr> bin = x->findParentType<BinaryExpr>();
    ASSERT_always_require(bin == sum);

    // Find closest ancestor that's a BinaryExpr of type MINUS. TODO: demo C++03.
    found = x->findParent([](const Tree::NodePtr &node) {
            std::shared_ptr<BinaryExpr> bin = std::dynamic_pointer_cast<BinaryExpr>(node);
            return bin && bin->op == BinaryExpr::MINUS;
        });
    ASSERT_always_require(found == diff);

    // Since finding a node of a particular type that satisfies a predicate is so common, there's a shortcut. TODO: demo c++03
    bin = x->findParentType<BinaryExpr>([](const std::shared_ptr<BinaryExpr> &node) {
            return node->op == BinaryExpr::MINUS;
        });
    ASSERT_always_require(bin == diff);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: The AST supports iteration in addition to traversal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_iteration() {
#if 0 // [Robb Matzke 2018-12-16]: Not implemented yet
    // Create the expression: (x + y) - z
    auto x = std::make_shared<Variable>("x");
    auto y = std::make_shared<Variable>("y");
    auto z = std::make_shared<Variable>("z");
    auto sum = std::make_shared<BinaryExpr>(BinaryExpr::PLUS, x, y);
    auto diff = std::make_shared<BinaryExpr>(BinaryExpr::MINUS, sum, z);

    std::vector<Tree::Node> processed;
    for (Tree::Iterator iter = diff->begin(); iter != diff->end(); ++iter) {
        processed.push_back(*iter);
        if (*iter == sum)
            iter.skipChildren();
    }
    ASSERT_always_require(processed.size() == 3);
    ASSERT_always_require(processed[0] == diff);
    ASSERT_always_require(processed[1] == sum);
    ASSERT_always_require(processed[2] == z);
#else
    std::cerr <<"test_iteration is not implemented yet\n";
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: A node can support an array of children.
//
// As in ROSE's ROSETTA, arrays are intended to be stored in a separate node that contains only the array. Doing it this way
// simplifies the semantics of traversals and child numbering. ROSETTA doesn't enforce this, and the nodes that violate this
// practice are confusing, so this API just doesn't even allow it.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Create a node whose sole purpose is to store a vector of pointers to child nodes. We start with something simple: a node
// that stores an array of Leaf nodes that we declared way back at the top of this source file.
typedef Tree::ListNode<Leaf> LeafList;

// Here's a node that contains three children, one of which is the LeafList.
class ExprAndList: public Tree::Node {
public:
    Tree::ChildEdge<ExprBase> firstExpr;                // children[0]
    Tree::ChildEdge<LeafList> leafList;                 // children[1] no matter how many leaves there are
    Tree::ChildEdge<ExprBase> secondExpr;               // children[2]

    // Best practice for fewest surprises is to always (or never) allocate list nodes in their parent constructor.
    ExprAndList()
        : firstExpr(this), leafList(this, std::make_shared<LeafList>()), secondExpr(this) {}
};

// The Tree::ListNode class is final (even in C++03) since its only purpose is to store a list of children.
#if 0 // Intentionally doesn't compile
COMPILE_ERROR(class BadListNode: public Tree::ListNode<Leaf> {} foo);
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Operations on a list of children adjust parents
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_node_list() {
    auto top = std::make_shared<ExprAndList>();
    ASSERT_always_require(top->children.size() == 3);
    ASSERT_always_require(top->children[0] == nullptr);                   // firstExpr
    ASSERT_always_require(top->children[1] == top->leafList);
    ASSERT_always_require(top->children[2] == nullptr);                   // secondExpr

    std::shared_ptr<LeafList> leaves = top->leafList;
    ASSERT_always_require(leaves != nullptr);
    ASSERT_always_require(leaves->parent == top);
    ASSERT_always_require(leaves->children.size() == 0);

    // As seen earlier with regular nodes, the children[i] are Tree::Node* rather than the derived Leaf*. Therefore doing
    // something like
    //   top->children[1]->size()
    // wont compile even though
    ASSERT_always_require(top->children[1] == leaves);
    // they're the same pointers, just different types
    
    // As with assigning to AST child data members in plain nodes, modifying the child list of a list node modifies the parent
    // pointers in the affected children.  E.g., adding a leaf node to the list makes the leaf's parent be the list.  The list
    // node implements most of the std::vector API. I'm using "at" instead of operator[] because it looks better with pointers,
    // and it's safer although slightly slower.
    auto a = std::make_shared<Leaf>();
    leaves->push_back(a);
    ASSERT_always_require(leaves->at(0) == a);
    ASSERT_always_require(a->parent == leaves);

    // The leaves have the correct type
    std::shared_ptr<Leaf> a2 = leaves->at(0);
    ASSERT_always_require(a == a2);

    // Assigning a different leaf adjusts parent pointers
    auto b = std::make_shared<Leaf>();
    leaves->setAt(0, b);
    ASSERT_always_require(leaves->at(0) == b);
    ASSERT_always_require(a->parent == nullptr);
    ASSERT_always_require(b->parent == leaves);

    // Assigning the same leaf does nothing
    leaves->setAt(0, b);
    ASSERT_always_require(leaves->at(0) == b);
    ASSERT_always_require(a->parent == nullptr);
    ASSERT_always_require(b->parent == leaves);

    // Assigning a null pointer removes the leaf
    leaves->setAt(0, nullptr);
    ASSERT_always_require(leaves->at(0) == nullptr);
    ASSERT_always_require(a->parent == nullptr);
    ASSERT_always_require(b->parent == nullptr);

    // Assigning null again does nothing
    leaves->setAt(0, nullptr);
    ASSERT_always_require(leaves->at(0) == nullptr);
    ASSERT_always_require(a->parent == nullptr);
    ASSERT_always_require(b->parent == nullptr);

    // The children are also available with the usual 'children' data member, which is how iteration works. All the usual
    // Node::children stuff is available for you to use, but of course you cannot modify that data member (see earlier test).
    ASSERT_always_require(leaves->children.size() == leaves->size());

    // Normally you won't have a variable like "leaves" pointing to the list node; instead you'll access it through the list
    // node's parent like the lhs of the following '=='. This is a good reason why top's constructor allocated leafList.
    ASSERT_always_require(top->leafList->size() == leaves->size());
    ASSERT_always_require(top->children[1]->children[0] == leaves->at(0));
    // top->children[1]->size() doesn't compile since children[i] are type Tree::Node*, not subclasses thereof.
    // top->leafList->size() of course works fine since leafList's type is implicitly Leaf*
    // leaves->children[i] also returns Tree::Node* rather than Leaf*

    
    // TODO: [Robb Matzke 2018-12-16] we need lots of tests for this node yet. Work in progress.
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main() {
    test_childptr_init();
    test_parent_adjustment();
    test_multi_parent();
    test_parent_assignment();
    test_child_vector();
    test_const_child_vector();
    test_cycle();
    test_noncopyable();
    test_convert_to_bool();
    test_convert_to_int();
    test_ptr_logic();
    test_derived_classes();
    test_derived_ptr_assign();
    test_upcast();
    test_downward_traversal();
    test_upward_traversal();
    test_find_downward();
    test_find_upward();
    test_iteration();
    test_node_list();
}

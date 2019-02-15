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
    // The pointers to the children are declared with a special pointer type. You can only use these types for data members
    // of classes derived from Tree::Node.
    Tree::ChildPtr<Leaf> first;
    Tree::ChildPtr<Leaf> second;

    // The child pointers don't have default constructors, therefore you must define a default constructor for PairOfLeaves.
    // The order of their declarations (and thus initializations) defines the order of the traversals we'll see later.
    PairOfLeaves()
        : first(this), second(this) {}
    
    // Additional constructors are permitted
    PairOfLeaves(Leaf *first, Leaf *second)
        : first(this, first), second(this, second) {}

    // No special destructor is necessary, although you can create one if you have other data to destroy.  Destroying an AST
    // node automatically unlinks the children from the node and resets their parent pointers to null.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Constructing a node with child pointers initializes the child pointers to null.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_childptr_init() {
    PairOfLeaves *node = new PairOfLeaves;
    ASSERT_always_require(node->first == NULL);
    ASSERT_always_require(node->second == NULL);
    ASSERT_always_require(node->parent == NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Assignment of node as the child of a parent adjusts the child's parent pointer.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_parent_adjustment() {
    Leaf *a = new Leaf;
    Leaf *b = new Leaf;
    PairOfLeaves *pair = new PairOfLeaves;

    // Assign a child to the parent
    pair->first = a;
    ASSERT_always_require(pair->first == a);
    ASSERT_always_require(a->parent == pair);
    ASSERT_always_require(b->parent == NULL);

    // Assign a different child adjusts the parent of the old and new child
    pair->first = b;
    ASSERT_always_require(pair->first == b);
    ASSERT_always_require(a->parent == NULL);
    ASSERT_always_require(b->parent == pair);

    // Assigning the same child has no effect
    pair->first = b;
    ASSERT_always_require(pair->first == b);
    ASSERT_always_require(a->parent == NULL);
    ASSERT_always_require(b->parent == pair);

    // Assigning null changes the old child's parent
    pair->first = NULL;
    ASSERT_always_require(pair->first == NULL);
    ASSERT_always_require(a->parent == NULL);
    ASSERT_always_require(b->parent == NULL);

    // Assigning null to a null pointer has no effect
    pair->first = NULL;
    ASSERT_always_require(pair->first == NULL);
    ASSERT_always_require(a->parent == NULL);
    ASSERT_always_require(b->parent == NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: A node cannot be attached to more than one point in the tree.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_multi_parent() {
    Leaf *a = new Leaf;
    PairOfLeaves *pair = new PairOfLeaves;

    pair->first = a;

    // Attempting to attach "a" to some other point in a tree will throw an exception and not change anything.
    try {
        pair->second = a;
        ASSERT_always_require(!"should not get here");
    } catch (const Tree::MultiParentException &error) {
        ASSERT_always_require(pair->first == a);
        ASSERT_always_require(a->parent == pair);
        ASSERT_always_require(pair->second == NULL);
        ASSERT_always_require(error.node == a);
        ASSERT_always_require(error.extraParent == pair);
    }

    // A node cannot be attached to a different tree
    PairOfLeaves *otherTree = new PairOfLeaves;
    try {
        otherTree->first = a;
        ASSERT_always_require(!"should not get here");
    } catch (const Tree::MultiParentException &error) {
        ASSERT_always_require(pair->first == a);
        ASSERT_always_require(a->parent == pair);
        ASSERT_always_require(otherTree->first == NULL);
        ASSERT_always_require(error.node == a);
        ASSERT_always_require(error.extraParent == otherTree);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Illegal to assign to parent.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_parent_assignment() {
#if 0 // Intentionally does not compile.
    Leaf *a = new Leaf;
    PairOfLeaves *pair = new PairOfLeaves;

    // Assigning directly to the parent is prohibited because it is often ambiguous (as in this example, where we can't tell
    // whether the intent is to make a the first or second child of the pair.
    COMPILE_ERROR(a->parent = pair); // Tree::ParentPtr::operator=(Tree::Node*) is private (could be a better message in C++11)

    // Setting a parent to null is also disallowed since implementing it would make the library significantly more complicated.
    COMPILE_ERROR(a->parent = NULL); // Tree::ParentPtr::operator=(Tree::Node*) is private (could be a better message in C++11)
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: All child pointers are also available in a vector for iteration.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_child_vector() {
    Leaf *a = new Leaf;
    Leaf *b = new Leaf;
    PairOfLeaves *pair = new PairOfLeaves;

    // The number of children is determined by the declaration, not whether they are null pointers or not.
    ASSERT_always_require(pair->children.size() == 2);
    ASSERT_always_require(pair->children[0] == NULL);
    ASSERT_always_require(pair->children[1] == NULL);

    // The order of the children are the same as the order they're declared in the class, not the order they're assigned.
    pair->second = b;
    pair->first = a;
    ASSERT_always_require(pair->children.size() == 2);
    ASSERT_always_require(pair->children[0] == a);
    ASSERT_always_require(pair->children[1] == b);

    // Removing the child from the parent removes it from the vector without changing the vector's size.
    pair->first = NULL;
    ASSERT_always_require(pair->children.size() == 2);
    ASSERT_always_require(pair->children[0] == NULL);
    ASSERT_always_require(pair->children[1] == b);

    // Child vectors operate as expected (although they're not actual std::vector (we'll see why shortly)
    PairOfLeaves *pair2 = new PairOfLeaves;
    ASSERT_always_require(pair->children.size() == 2);
    ASSERT_always_require(pair->children.max_size() >= 2);
    ASSERT_always_require(pair->children.capacity() >= 2);
    ASSERT_always_require(!pair->children.empty());
    pair->children.reserve(10);                         // permitted since it doesn't modify values
    ASSERT_always_require(pair->children[0] == NULL);
    ASSERT_always_require(pair->children.at(0) == NULL);
    ASSERT_always_require(pair->children.front() == NULL);
    ASSERT_always_require(pair->children.back() == b);
    ASSERT_always_require(!(pair->children == pair2->children));
    ASSERT_always_require(pair->children != pair2->children);
    ASSERT_always_require(pair->children <= pair2->children || pair->children > pair2->children);
    ASSERT_always_require(pair->children < pair2->children || pair->children >= pair2->children);
#if __cplusplus >= 201103L
    pair->children.shrink_to_fit();                     // permitted since it doesn't modify values
    ASSERT_always_require(pair->children.data() != NULL);
#endif

    // TODO: [Robb Matzke 2018-12-16]: iterators are not yet implemented
    std::cerr <<"Node::children iterators are not implemented yet\n";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Child vector is not modifiable by the user.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_const_child_vector() {
#if 0 // These intentionally don't compile
    PairOfLeaves *pair = new PairOfLeaves;
    PairOfLeaves *pair2 = new PairOfLeaves;

    // We don't allow modifications to the vector of child pointers for the same reasons we don't allow setting the
    // parent pointers explicitly. Namely, allowing it would substantially complicate the implementation and is not necessary.
    COMPILE_ERROR(pair->children.resize(10));           // class Tree::NodeVector has no member named 'resize'
    COMPILE_ERROR(pair->children[0] = NULL);            // lvalue required as left operand of assignment
    COMPILE_ERROR(pair->children.at(0) = NULL);         // lvalue required as left operand of assignment
    COMPILE_ERROR(pair->children.front() = NULL);       // lvalue required as left operand of assignment
    COMPILE_ERROR(pair->children.back() = NULL);        // lvalue required as left operand of assignment
    COMPILE_ERROR(pair->children.assign(i, NULL));      // class Tree::NodeVector has no member named 'assign'
    COMPILE_ERROR(pair->children.push_back(NULL));      // Tree::NodeVector::push_back is private
    COMPILE_ERROR(pair->children.pop_back(NULL));       // class Tree::NodeVector has no member named 'pop_back'
    COMPILE_ERROR(pair->children.insert());             // class Tree::NodeVector has no member named 'insert'
    COMPILE_ERROR(pair->children.swap(pair->children)); // class Tree::NodeVector has no member named 'swap'
    COMPILE_ERROR(pair->children.clear());              // class Tree::NodeVector has no member named 'clear'
    COMPILE_ERROR(std::swap(pair->children, pair2->children)); // undefined reference to std::swap<Tree::NodeVector>
#if __cplusplus >= 201103L
    COMPILE_ERROR(pair->children.data()[0] = NULL);     // assignment of read-only location
    COMPILE_ERROR(pair->children.emplace());            // class Tree::NodeVector has no member named 'emplace'
    COMPILE_ERROR(pair->children.emplace_back());       // class Tree::NodeVector has no member named 'emplace_back'
#endif
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Assigning a node as a child of itself is not permitted.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Creating a cycle in the tree is not permitted.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: AST nodes with children are not copyable.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_noncopyable() {
#if 0 // Intentionally doesn't compile
    Leaf *a = new Leaf;
    Leaf *b = new Leaf;
    PairOfLeaves *p1 = new PairOfLeaves(a, b);
    PairOfLeaves *p2 = new PairOfLeaves;

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
    PairOfLeaves *pair = new PairOfLeaves;

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

    bool b1 = pair->first;
    ASSERT_always_require(!b1);

    b1 = pair->first;
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

    bool b2 = pair->parent;
    ASSERT_always_require(!b2);

    b2 = pair->parent;
    ASSERT_always_require(!b2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Pointers are not implicitly convertible to integers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_convert_to_int() {
#if 0 // Intentionally does not compile
    Leaf *a = new Leaf;

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
    PairOfLeaves *pair = new PairOfLeaves;
    PairOfLeaves *p2 = new PairOfLeaves;

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

// First, create a small class hiearchy for expression trees. It contains one class that's the base type for all expression
// nodes, and two subtypes: one for variables and another for binary operations like "a+b" and "a-b".
class ExprBase: public Tree::Node {
public:
    Tree::ChildPtr<Leaf> leaf; // just something to demo that child ptrs can appear at multiple levels in the class hierarchy
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
    Tree::ChildPtr<ExprBase> lhs, rhs;
    BinaryExpr()
        : op(PLUS), lhs(this), rhs(this) {}
    BinaryExpr(Operator op, ExprBase *lhs, ExprBase *rhs)
        : op(op), lhs(this, lhs), rhs(this, rhs) {}
};

static void test_derived_classes() {
    // Create the expression: (x + y) - z
    Variable *x = new Variable("x");
    Variable *y = new Variable("y");
    Variable *z = new Variable("z");
    BinaryExpr *sum = new BinaryExpr(BinaryExpr::PLUS, x, y);
    BinaryExpr *diff = new BinaryExpr(BinaryExpr::MINUS, sum, z);

    ASSERT_always_require(sum->lhs == x);
    ASSERT_always_require(sum->rhs == y);
    ASSERT_always_require(diff->lhs == sum);
    ASSERT_always_require(diff->rhs == z);
    ASSERT_always_require(x->parent == sum);
    ASSERT_always_require(y->parent == sum);
    ASSERT_always_require(z->parent == diff);
    ASSERT_always_require(sum->parent == diff);
    ASSERT_always_require(diff->parent == NULL);

    // Children of a super class are ordered before children of derived classes. This is just the order-of-initialization
    // rule applied across the class hierarchy.
    ASSERT_always_require(sum->children.size() == 3);
    ASSERT_always_require(sum->children[0] == NULL);                   // leaf
    ASSERT_always_require(sum->children[1] == x);
    ASSERT_always_require(sum->children[2] == y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: A derived pointer can be assigned to a base pointer.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_derived_ptr_assign() {
    Variable *derived = new Variable;
    BinaryExpr *add = new BinaryExpr;

    // Derived pointer assigned to base pointer
    add->lhs = derived;
    ASSERT_always_require(add->lhs == derived);
    ASSERT_always_require(derived->parent == add);

    // The smart base pointer is convertible to a raw base pointer
    ExprBase *raw1 = add->lhs;
    ASSERT_always_require(raw1 == derived);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Base pointers can be up-cast
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_upcast() {
    BinaryExpr *add = new BinaryExpr(BinaryExpr::PLUS, new Variable("x"), new Variable("y"));

    // Obviously raw pointers work
    ExprBase *raw = add->lhs;
    Variable *derived = dynamic_cast<Variable*>(raw);
    ASSERT_always_require(derived != NULL);
    
    // What about smart pointers? It turns out that it's only possible to override dynamic casts for std::shared_ptr, and
    // that's only in C++11 and later. Therefore we're stuck with a mild annoyance. Good C++ code generally avoids dynamic
    // casts anyway, so probably not that big a deal.

#if 0 
    derived = dynamic_cast<Variable*>(add->lhs); // doesn't compile although we'd like it to.
#else
    derived = dynamic_cast<Variable*>(add->lhs.raw());
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Pre-post downward (child-following) traversals using C++11 functor classes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Since Visitor1 becomes a template argument later, it must be at this scope.
// TODO: instead of node->traverse(visitor), also implement visitor->traverse(node) since that would alleviate this requirement.
class Visitor1 {
public:
    std::vector<std::pair<Tree::TraversalEvent, Tree::Node*> > processed;

    Tree::TraversalAction operator()(Tree::Node *node, Tree::TraversalEvent event) {
        processed.push_back(std::make_pair(event, node)); // events are ENTER and LEAVE
        return Tree::CONTINUE; // other choices are SKIP_CHILDREN and ABORT
    }
};

static void test_downward_traversal() {
    // Create the expression: (x + y) - z
    Variable *x = new Variable("x");
    Variable *y = new Variable("y");
    Variable *z = new Variable("z");
    BinaryExpr *sum = new BinaryExpr(BinaryExpr::PLUS, x, y);
    BinaryExpr *diff = new BinaryExpr(BinaryExpr::MINUS, sum, z);

    // This tests the general mechanism using Tree::Node::traverse and passing a functor with an operator() that takes two
    // arguments: one describes the traversal event as either entering or leaving the node, and the other is the node under
    // consideration. Other traversals can be built on top of a pre-post traversal.
    Visitor1 prePostVisitor;
    diff->traverse(prePostVisitor);

    ASSERT_always_require(prePostVisitor.processed.size() == 10);      // enter and leave each node exactly once
    ASSERT_always_require(prePostVisitor.processed[0].first == Tree::ENTER && prePostVisitor.processed[0].second == diff);
    ASSERT_always_require(prePostVisitor.processed[1].first == Tree::ENTER && prePostVisitor.processed[1].second == sum);
    ASSERT_always_require(prePostVisitor.processed[2].first == Tree::ENTER && prePostVisitor.processed[2].second == x);
    ASSERT_always_require(prePostVisitor.processed[3].first == Tree::LEAVE && prePostVisitor.processed[3].second == x);
    ASSERT_always_require(prePostVisitor.processed[4].first == Tree::ENTER && prePostVisitor.processed[4].second == y);
    ASSERT_always_require(prePostVisitor.processed[5].first == Tree::LEAVE && prePostVisitor.processed[5].second == y);
    ASSERT_always_require(prePostVisitor.processed[6].first == Tree::LEAVE && prePostVisitor.processed[6].second == sum);
    ASSERT_always_require(prePostVisitor.processed[7].first == Tree::ENTER && prePostVisitor.processed[7].second == z);
    ASSERT_always_require(prePostVisitor.processed[8].first == Tree::LEAVE && prePostVisitor.processed[8].second == z);
    ASSERT_always_require(prePostVisitor.processed[9].first == Tree::LEAVE && prePostVisitor.processed[9].second == diff);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Downward traversals using lambdas instead of function objects
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_downward_traversal_lambda() {
#if __cplusplus >= 201103L
    // Create the expression: (x + y) - z
    Variable *x = new Variable("x");
    Variable *y = new Variable("y");
    Variable *z = new Variable("z");
    BinaryExpr *sum = new BinaryExpr(BinaryExpr::PLUS, x, y);
    BinaryExpr *diff = new BinaryExpr(BinaryExpr::MINUS, sum, z);

    // Does the same thing as test_downward_traversal except with less total code and concentrated all in one place.
    std::vector<std::pair<Tree::TraversalEvent, Tree::Node*> > processed;
    diff->traverse([&processed](Tree::Node *node, Tree::TraversalEvent event) -> Tree::TraversalAction {
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
#else
    std::cerr <<"test_downward_traversal_lambda requires C++11 or later\n";
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Upward traversals (following parent pointers) are identical to downward traversals, just a different function name.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Since Visitor2 becomes a template argument later, it must be at this scope.
// TODO: instead of node->traverse(visitor), also implement visitor->traverse(node) since that would alleviate this requirement.
class Visitor2 {
public:
    std::vector<std::pair<Tree::TraversalEvent, Tree::Node*> > processed;

    Tree::TraversalAction operator()(Tree::Node *node, Tree::TraversalEvent event) {
        processed.push_back(std::make_pair(event, node));
        return Tree::CONTINUE;
    }
};

static void test_upward_traversal() {
    // Create the expression: (x + y) - z
    Variable *x = new Variable("x");
    Variable *y = new Variable("y");
    Variable *z = new Variable("z");
    BinaryExpr *sum = new BinaryExpr(BinaryExpr::PLUS, x, y);
    BinaryExpr *diff = new BinaryExpr(BinaryExpr::MINUS, sum, z);

    Visitor2 prePostVisitor;
    x->traverseParents(prePostVisitor);

    ASSERT_always_require(prePostVisitor.processed.size() == 6);
    ASSERT_always_require(prePostVisitor.processed[0].first == Tree::ENTER && prePostVisitor.processed[0].second == x);
    ASSERT_always_require(prePostVisitor.processed[1].first == Tree::ENTER && prePostVisitor.processed[1].second == sum);
    ASSERT_always_require(prePostVisitor.processed[2].first == Tree::ENTER && prePostVisitor.processed[2].second == diff);
    ASSERT_always_require(prePostVisitor.processed[3].first == Tree::LEAVE && prePostVisitor.processed[3].second == diff);
    ASSERT_always_require(prePostVisitor.processed[4].first == Tree::LEAVE && prePostVisitor.processed[4].second == sum);
    ASSERT_always_require(prePostVisitor.processed[5].first == Tree::LEAVE && prePostVisitor.processed[5].second == x);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Upward traversals using lambdas.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_upward_traversal_lambda() {
#if __cplusplus >= 201103L
    // Create the expression: (x + y) - z
    Variable *x = new Variable("x");
    Variable *y = new Variable("y");
    Variable *z = new Variable("z");
    BinaryExpr *sum = new BinaryExpr(BinaryExpr::PLUS, x, y);
    BinaryExpr *diff = new BinaryExpr(BinaryExpr::MINUS, sum, z);

    // Same as test_upward_traversal except a less code and contrated all in one place.
    std::vector<std::pair<Tree::TraversalEvent, Tree::Node*> > processed;
    x->traverseParents([&processed](Tree::Node *node, Tree::TraversalEvent event) {
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
#else
    std::cerr <<"test_upward_traversal_lambda requires C++11 or later\n";
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Search for nodes by following child pointers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_find_downward() {
#if __cplusplus >= 201103L
    // Create the expression: (x + y) - z
    Variable *x = new Variable("x");
    Variable *y = new Variable("y");
    Variable *z = new Variable("z");
    BinaryExpr *sum = new BinaryExpr(BinaryExpr::PLUS, x, y);
    BinaryExpr *diff = new BinaryExpr(BinaryExpr::MINUS, sum, z);

    // Find first node that's a variable. TODO: demo this with C++03 functor classes and functions
    Tree::Node *found = diff->find([](Tree::Node *node) { return dynamic_cast<Variable*>(node); });
    ASSERT_always_require(found == x);

    // Since this happens so frequently and is verbose in C++03, there's a shortcut.
    Variable *var = diff->findType<Variable>();
    ASSERT_always_require(var == x);

    // Find first node that's a variable with the name "y". TODO: demo C++03
    found = diff->find([](Tree::Node *node) {
            Variable *v = dynamic_cast<Variable*>(node);
            return v && v->name == "y";
        });
    ASSERT_always_require(found == y);

    // Since finding a node of a particular type that satisfies some predicate is so common, there's a shortcut. TODO: demo C++03
    var = diff->findType<Variable>([](Variable *v) { return v->name == "y"; });
    ASSERT_always_require(var == y);
#else
    std::cerr <<"test_find_downward requires C++11 or later\n";
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Search for nodes by following parent pointers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_find_upward() {
#if __cplusplus >= 201103L
    // Create the expression: (x + y) - z
    Variable *x = new Variable("x");
    Variable *y = new Variable("y");
    Variable *z = new Variable("z");
    BinaryExpr *sum = new BinaryExpr(BinaryExpr::PLUS, x, y);
    BinaryExpr *diff = new BinaryExpr(BinaryExpr::MINUS, sum, z);

    // Find closest ancestor that's a BinaryExpr. TODO: demo this with C++03 functor classes and functions
    Tree::Node *found = x->findParent([](Tree::Node *node) { return dynamic_cast<BinaryExpr*>(node); });
    ASSERT_always_require(found == sum);

    // Since this happens frequently and is quite verbose for C++03, there's a shortcut.
    BinaryExpr *bin = x->findParentType<BinaryExpr>();
    ASSERT_always_require(bin == sum);

    // Find closest ancestor that's a BinaryExpr of type MINUS. TODO: demo C++03.
    found = x->findParent([](Tree::Node *node) {
            BinaryExpr *bin = dynamic_cast<BinaryExpr*>(node);
            return bin && bin->op == BinaryExpr::MINUS;
        });
    ASSERT_always_require(found == diff);

    // Since finding a node of a particular type that satisfies a predicate is so common, there's a shortcut. TODO: demo c++03
    bin = x->findParentType<BinaryExpr>([](BinaryExpr *node) { return node->op == BinaryExpr::MINUS; });
    ASSERT_always_require(bin == diff);
#else
    std::cerr <<"test_find_upward requires C++11 or later\n";
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: The AST supports iteration in addition to traversal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_iteration() {
#if 0 // [Robb Matzke 2018-12-16]: Not implemented yet
    // Create the expression: (x + y) - z
    Variable *x = new Variable("x");
    Variable *y = new Variable("y");
    Variable *z = new Variable("z");
    BinaryExpr *sum = new BinaryExpr(BinaryExpr::PLUS, x, y);
    BinaryExpr *diff = new BinaryExpr(BinaryExpr::MINUS, sum, z);

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
// practice are confusing.  So this API just doesn't even allow it.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Create a node whose sole purpose is to store a vector of pointers to child nodes. We start with something simple: a node
// that stores an array of Leaf nodes that we declared way back at the top of this source file.
typedef Tree::NodeList<Leaf> LeafList;

// Here's a node that contains three children, one of which is the LeafList.
class ExprAndList: public Tree::Node {
public:
    Tree::ChildPtr<ExprBase> firstExpr;                  // children[0]
    Tree::ChildPtr<LeafList> leafList;                   // children[1] no matter how many leaves there are
    Tree::ChildPtr<ExprBase> secondExpr;                 // children[2]

    // Best practice for fewest surprises is to always (or never) allocate list nodes in their parent constructor.
    ExprAndList()
        : firstExpr(this), leafList(this, new LeafList), secondExpr(this) {}
};

// The Tree::NodeList class is final (even in C++03) since its only purpose is to store a list of children.
#if 0 // Intentionally doesn't compile
COMPILE_ERROR(class BadListNode: public Tree::NodeList<Leaf> {} foo);
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEST: Operations on a list of children adjust parents
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void test_node_list() {
    ExprAndList *top = new ExprAndList;
    ASSERT_always_require(top->children.size() == 3);
    ASSERT_always_require(top->children[0] == NULL);                   // firstExpr
    ASSERT_always_require(top->children[1] == top->leafList);
    ASSERT_always_require(top->children[2] == NULL);                   // secondExpr

    LeafList *leaves = top->leafList;
    ASSERT_always_require(leaves != NULL);
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
    Leaf *a = new Leaf;
    leaves->push_back(a);
    ASSERT_always_require(leaves->at(0) == a);
    ASSERT_always_require(a->parent == leaves);

    // The leaves have the correct type
    Leaf *a2 = leaves->at(0);
    ASSERT_always_require(a == a2);

    // Assigning a different leaf adjusts parent pointers
    Leaf *b = new Leaf;
    leaves->at(0) = b;
    ASSERT_always_require(leaves->at(0) == b);
    ASSERT_always_require(a->parent == NULL);
    ASSERT_always_require(b->parent == leaves);

    // Assigning the same leaf does nothing
    leaves->at(0) = b;
    ASSERT_always_require(leaves->at(0) == b);
    ASSERT_always_require(a->parent == NULL);
    ASSERT_always_require(b->parent == leaves);

    // Assigning a null pointer removes the leaf
    leaves->at(0) = NULL;
    ASSERT_always_require(leaves->at(0) == NULL);
    ASSERT_always_require(a->parent == NULL);
    ASSERT_always_require(b->parent == NULL);

    // Assigning null again does nothing
    leaves->at(0) = NULL;
    ASSERT_always_require(leaves->at(0) == NULL);
    ASSERT_always_require(a->parent == NULL);
    ASSERT_always_require(b->parent == NULL);


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
    test_noncopyable();
    test_convert_to_bool();
    test_convert_to_int();
    test_ptr_logic();
    test_derived_classes();
    test_derived_ptr_assign();
    test_upcast();
    test_downward_traversal();
    test_downward_traversal_lambda();
    test_upward_traversal();
    test_upward_traversal_lambda();
    test_find_downward();
    test_find_upward();
    test_iteration();
    test_node_list();
}

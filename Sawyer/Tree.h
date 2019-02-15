#ifndef Sawyer_Tree_H
#define Sawyer_Tree_H

#include <Sawyer/Assert.h>
#include <cassert>
#include <stdexcept>
#include <vector>

namespace Sawyer {

/** Tree data structure.
 *
 *  This name space contains the build blocks for relating objects in a tree-like way. The children are referenced either
 *  by name or iteratively, and each object has a single parent. The parent pointers are adjusted automatically to ensure
 *  consistency. Objects can also point to children that are somewhere else in the tree (or not even in the tree at all).
 *
 *  This type is well suited for representing such things as abstract syntax trees.
 *
 *  Although the implementation is intended to be efficient in time and space, the primary goal is safety. When the two goals
 *  are in conflict, safety takes precedence. Other data structures are better suited to those cases when the user doesn't
 *  require parent pointers or doesn't need their consistency to be enforced by the API.
 *
 *  NOTICE: This API is experimental and still under development. */
namespace Tree {

class Node;
class ChildPtrBase;

/** Traversal event bit flags. */
enum TraversalEvent {
    ENTER = 0x1,                                        /**< Traversal has just entered the node under consideration. */
    LEAVE = 0x2                                         /**< Traversal has just left the node under consideration. */
};

/** Traversal actions. */
enum TraversalAction {
    CONTINUE,                                           /**< Continue with the traversal. */
    SKIP_CHILDREN,                                      /**< For enter events, do not traverse into the node's children. */
    ABORT                                               /**< Abort the traversal immediately. */
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Exceptions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Exceptions related to trees. */
class Exception: public std::runtime_error {
public:
    /** Default constroctor. */
    explicit Exception(const std::string &what)
        : std::runtime_error(what) {}

    ~Exception() throw () {};
};

/** Exception for multiple parents.
 *
 *  This exception is thrown when a an attempt is mde to add a node to a tree but the node is already part of some tree. */
class MultiParentException: public Exception {
public:
    /** Node whose single-parent invariant was violated. */
    Node *node;

    /** The parent that cause the single-parent invariant to be violated. */
    Node *extraParent;

    /** Construct a new exception.
     *
     *  The @p node is the node whose single-parent invariant was violated and the @p extraParent is the parent that violated
     *  the invariant. */
    MultiParentException(Node *node, Node *extraParent, const std::string &what = "node already has a different parent")
        : Exception(what), node(node), extraParent(extraParent) {}

    ~MultiParentException() throw () {}
};

/** Exception for null pointer dereferences.
 *
 *  This exception is thrown if a null pointer is dereferenced. */
class NullDereferenceException: public Exception {
public:
    /** Construct a new exception. */
    NullDereferenceException(const std::string &what = "null pointer dereference")
        : Exception(what) {}

    ~NullDereferenceException() throw () {}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal classes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Base class for ChildPtr template classes in order to friendly access Node.
class ChildPtrBase {
protected:
    // Make the child point at the parent
    void setParent(Node *parent, Node *child);

    // Make the parent point at the child
    void setChild(Node *parent, size_t index, Node *child);

    // Declare a new child of the parent and set it to point to the child.
    size_t newChildPtr(Node *parent, Node *child);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pointers from child to parent.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Pointer from child to parent.
 *
 *  This class is used to point from a child to a parent in a way that allows the caller to use it like a regular pointer but
 *  not be able to assign anything to it.  The parent pointer of a child can only be changed by assigning the child to the
 *  parent node, or changed to null by assigning something else to the child's pointer. */
class ParentPtr {
    Node *parent_;

private:
    friend class ChildPtrBase;

    // Used internally
    ParentPtr& operator=(Node *parent) { // DO NOT MAKE THIS PUBLIC
        parent_ = parent;
        return *this;
    }
    
public:
    /** Default constructor for null pointer. */
    ParentPtr()
        : parent_(NULL) {}

    ~ParentPtr() throw () {}

    /** Pointer to parent, possibly null. */
    Node* operator->() {
        return parent_;
    }

    /** Reference to parent node.
     *
     *  Throws a @ref NullDereferenceException if this pointer is null. */
    Node& operator*() {
        if (NULL == parent_)
            throw NullDereferenceException();
        return *parent_;
    }

    /** Convert smart pointer to raw pointer.
     *
     *  This is probably only needed for casting since the standard doesn't allow overriding cast operators. */
    Node* raw() const {
        return parent_;
    }
    
    operator Node*() {
        return parent_;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pointers from parent to child
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Pointer from parent node to child node.
 *
 *  These special pointer types are data members of a parent tree node and point to a child whose type derives from T. Whenever
 *  the pointer's value changes, the affected children's parent pointers are updated. The pointer assures that a child node
 *  has exactly one parent. */
template<class T>
class ChildPtr: public ChildPtrBase {
public:
    /** Type of object pointed to. The type of the child node. */
    typedef T Child;

private:
    Node *parent_;
    Child *child_;
    size_t index_;

public:
    ChildPtr(): parent_(NULL), child_(NULL) {}

    /** Constructor.
     *
     *  When the object containing a ChildPtr data member is constructed, it should call this constructor and pass itself
     *  as the parent. A child can be assigned immediately, or at a later time. If the child is non-null then it must have
     *  a null parent pointer. */
    explicit ChildPtr(Node *parent, Child *child = NULL)
        : parent_(parent), child_(child) {
        assert(parent != NULL);
        if (child && child->parent != NULL)
            throw MultiParentException(child, parent);
        index_ = newChildPtr(parent, child);
        if (child)
            setParent(parent, child);
    }

    /** Destructor unlinks child. */
    ~ChildPtr() throw () {
        clear();
    }

    /** Set child pointer to null.
     *
     *  After this call, this pointer will be null, and if this pointer's previous value was non-null then the parent
     *  of that child is set to null.
     *
     *  Assigning a null pointer does the same thing. */
    void clear() throw () {
        if (child_) {
            if (parent_)
                setChild(parent_, index_, NULL);
            setParent(NULL, child_);
            child_ = NULL;
        }
    }

    /** Assign a new child to the pointer.
     *
     *  Makes this pointer point to the @p newChild, which may be null. If this pointer previously pointed to some other child,
     *  that child's parent pointer is set to null. If the @p newChild is non-null, then its parent pointer is changed to point
     *  to the parent node containing this pointer. A @ref MultiParentException is thrown if @p newChild already has a parent,
     *  in which case no changes are made. */
    ChildPtr& operator=(Child *newChild) {
        if (newChild != child_) {
            if (newChild && newChild->parent != NULL)
                throw MultiParentException(newChild, parent_);

            clear();
            if (newChild)
                setParent(parent_, newChild);
            setChild(parent_, index_, newChild);
            child_ = newChild;
        }
        return *this;
    }

    /** Pointer to the child, or null. */
    Child* operator->() {
        return child_;
    }

    /** Reference to the child node.
     *
     *  This pointer must not be null or else a @ref NullDereferenceException is thrown. */
    Child& operator*() {
        if (NULL == child_)
            throw NullDereferenceException();
        return *child_;
    }

    /** Convert smart pointer to raw pointer.
     *
     *  This is probably only needed for casting since the standard doesn't allow overriding cast operators. */
    T* raw() const {
        return child_;
    }

    operator T*() {
        return child_;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vector-like type that only its friends can modify.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class NodeVector {
    std::vector<Node*> elmts_;
public:
    // Iterators
    // TODO: [Robb Matzke 2018-12-16]

    // Capacity
    size_t size() const { return elmts_.size(); }
    size_t max_size() const { return elmts_.max_size(); }
    size_t capacity() const { return elmts_.capacity(); }
    bool empty() const { return elmts_.empty(); }
    void reserve(size_t n) { elmts_.reserve(n); }
#if __cplusplus >= 201103L
    void shrink_to_fit() { elmts_.shrink_to_fit(); }
#endif

    // Const element access
    Node* at(size_t i) const { return elmts_.at(i); }
    Node* operator[](size_t i) const { return elmts_[i]; }
    Node* front() const { return elmts_.front(); }
    Node* back() const { return elmts_.back(); }
#if __cplusplus >= 201103L
    Node* const* data() const noexcept { return elmts_.data(); }
#endif
    const std::vector<Node*>& elmts() const { return elmts_; }

private:
    // Modifiers (private and only those we actually need)
    friend class ChildPtrBase;
    void setNode(size_t i, Node *node) { elmts_.at(i) = node; }
    void push_back(Node *node) { elmts_.push_back(node); }
    void swap(NodeVector&) { assert(!"swap method not supported"); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Base class for tree nodes.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Base class for tree nodes. */
class Node {
    friend class ChildPtrBase;

private:
    // nodes are not copyable
    Node(const Node&) { assert(0); }
    Node& operator=(const Node&) { ASSERT_not_reachable("not copyable"); }
    
public:
    Node() {}

    /** Pointer to parent node. */
    ParentPtr parent;

    virtual ~Node() {}

    /** Pointers to all children in declaration order. */
    NodeVector children;

private:
    enum Direction { UPWARD, DOWNWARD };

    // Traverse the tree from this node invoking the functor along the way.
    template<class Functor>
    TraversalAction traverseImpl(Functor &functor, Direction direction) {
        switch (TraversalAction action = functor(this, ENTER)) {
            case CONTINUE:
                if (DOWNWARD == direction) {
                    for (size_t i=0; i<children.size() && CONTINUE==action; ++i) {
                        if (children[i])
                            action = children[i]->traverseImpl(functor, direction);
                    }
                } else if (parent) {
                    action = parent->traverseImpl(functor, direction);
                }
                // fall through
            case SKIP_CHILDREN:
                if (ABORT != action && (action = functor(this, LEAVE)) == SKIP_CHILDREN)
                    action = CONTINUE;
                // fall through
            default:
                return action;
        }
    }

    // Find the first node of specified type satisfying the predicate.
    template<class T, class Predicate>
    T* findImpl(Predicate predicate, Direction direction) {
        struct Visitor { // Use only C++03, no lambdas
            Predicate &predicate;
            T *found;
            Visitor(Predicate &p): predicate(p), found(NULL) {}
            TraversalAction operator()(Node *node, TraversalEvent event) {
                if (ENTER == event) {
                    T *typedNode = dynamic_cast<T*>(node);
                    if (typedNode && predicate(typedNode))
                        found = typedNode;
                }
                return found ? ABORT : CONTINUE;
            }
        } visitor(predicate);
        traverseImpl(visitor, direction);
        return visitor.found;
    }
        


public:
    /** Traverse the tree starting at this node and following child pointers.
     *
     *  The @p functor takes two arguments: the tree node under consideration, and a TraversalEvent that indicates whether the
     *  traversal is entering or leaving the node.  The functor must return a @ref TraversalAction to describe what should
     *  happen next.
     *
     *  If the functor called by the node enter event returns @ref SKIP_CHILDREN, then none of the children are traversed and
     *  the next call to the functor is for leaving that same node.
     *
     *  If any call to the functor returns ABORT, then the traversal is immediately aborted.
     *
     *  If any functor returns ABORT, then the @ref traverse function also returns ABORT. Otherwise the @ref traverse function
     *  returns CONTINUE.
     *
     * @{ */
    template<class Functor>
    TraversalAction traverse(const Functor &functor) {
        return traverseImpl<const Functor>(functor, DOWNWARD);
    }

    template<class Functor>
    TraversalAction traverse(Functor &functor) {
        return traverseImpl<Functor>(functor, DOWNWARD);
    }
    /** @} */

    /** Traverse the tree by following parent pointers.
     *
     *  Other than following pointers from children to parents, this traversal is identical to the downward @ref traverse
     *  method.
     *
     * @{ */
    template<class Functor>
    TraversalAction traverseParents(const Functor &functor) {
        return traverseImpl<const Functor>(functor, UPWARD);
    }

    template<class Functor>
    TraversalAction traverseParents(Functor &functor) {
        return traverseImpl<Functor>(functor, UPWARD);
    }
    /** @} */

    /** Traverse an tree to find the first node satisfying the predicate.
     *
     * @{ */
    template<class Predicate>
    Node* find(const Predicate &predicate) {
        return findImpl<Node, const Predicate>(predicate, DOWNWARD);
    }

    template<class Predicate>
    Node* find(Predicate &predicate) {
        return findImpl<Node, Predicate>(predicate, DOWNWARD);
    }
    /** @} */

    /** Find first child that's the specified type. */
    template<class T>
    T* findType() {
        struct True {
            bool operator()(T*) { return true; }
        };
        return findImpl<T>(True(), DOWNWARD);
    }

    /** Find first child of specified type satisfying the predicate.
     *
     * @{ */
    template<class T, class Predicate>
    T* findType(const Predicate &predicate) {
        return findImpl<T, const Predicate>(predicate, DOWNWARD);
    }

    template<class T, class Predicate>
    T* findType(Predicate &predicate) {
        return findImpl<T, Predicate>(predicate, DOWNWARD);
    }
    /** @} */

    /** Find closest ancestor that satifies the predicate.
     *
     * @{ */
    template<class Predicate>
    Node* findParent(const Predicate &predicate) {
        return findImpl<Node, const Predicate>(predicate, UPWARD);
    }

    template<class Predicate>
    Node* findParent(Predicate &predicate) {
        return findImpl<Node, Predicate>(predicate, UPWARD);
    }
    /** @} */

    /** Find closest ancestor of specified type. */
    template<class T>
    T* findParentType() {
        struct True {
            bool operator()(T*) { return true; }
        };
        return findImpl<T>(True(), UPWARD);
    }

    /** Find closest ancestor of specified type that satisfies the predicate.
     *
     * @{ */
    template<class T, class Predicate>
    T* findParentType(const Predicate &predicate) {
        return findImpl<T, const Predicate>(predicate, UPWARD);
    }

    template<class T, class Predicate>
    T* findParentType(Predicate &predicate) {
        return findImpl<T, Predicate>(predicate, UPWARD);
    }
    /** @} */
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node that's only a list of child nodes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Helper class to make another class final since we can't assume C++11 is available.
template<class Derived>
class MakeFinal {
    friend Derived;
    MakeFinal() {} // intentionaly private
};

template<class T>
class NodeList: public Node, public ChildPtrBase, virtual MakeFinal<NodeList<T> > {
public:
    NodeList() {}

    std::vector<ChildPtr<T> > nodes;

    // TODO: [Robb Matzke 2018-12-16] implement almost the entire std::vector API here. To think about: instead of implementing
    // it here and the entire non-modifying part of the API a second time in NodeVector, maybe we can just implement it once?
    // OTOH, the non-modifying API is trivial here since we can delegate to NodeVector.

    size_t size() const { return children.size(); }

    T* at(size_t i) const { return nodes.at(i).raw(); }
    ChildPtr<T>& at(size_t i) { return nodes.at(i); }
    T* operator[](size_t i) const { return nodes[i].raw(); }
    ChildPtr<T>& operator[](size_t i) { return nodes[i]; }
    
    void push_back(T *newChild) { // child may be null
        nodes.push_back(ChildPtr<T>(this));
        nodes.back() = newChild;
    }
};

} // namespace
} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Non member function overloads
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool operator==(const Sawyer::Tree::NodeVector&, const Sawyer::Tree::NodeVector&);
bool operator!=(const Sawyer::Tree::NodeVector&, const Sawyer::Tree::NodeVector&);
bool operator< (const Sawyer::Tree::NodeVector&, const Sawyer::Tree::NodeVector&);
bool operator<=(const Sawyer::Tree::NodeVector&, const Sawyer::Tree::NodeVector&);
bool operator> (const Sawyer::Tree::NodeVector&, const Sawyer::Tree::NodeVector&);
bool operator>=(const Sawyer::Tree::NodeVector&, const Sawyer::Tree::NodeVector&);

namespace std {
// intentionally not defined; swap not allowed
template<> void swap<Sawyer::Tree::NodeVector>(Sawyer::Tree::NodeVector &a, Sawyer::Tree::NodeVector &b);
} // namespace

#endif

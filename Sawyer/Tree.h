#ifndef Sawyer_Tree_H
#define Sawyer_Tree_H

#include <Sawyer/Assert.h>
#include <Sawyer/Optional.h>

#include <memory>
#include <stdexcept>
#include <vector>

#if 1 // DEBUGGING [Robb Matzke 2019-02-18]
#include <iostream>
#endif

namespace Sawyer {
namespace Tree {

class Node;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                   ____            _                 _   _
//                  |  _ \  ___  ___| | __ _ _ __ __ _| |_(_) ___  _ __  ___
//                  | | | |/ _ \/ __| |/ _` | '__/ _` | __| |/ _ \| '_ \/ __|
//                  | |_| |  __/ (__| | (_| | | | (_| | |_| | (_) | | | \__ |
//                  |____/ \___|\___|_|\__,_|_|  \__,_|\__|_|\___/|_| |_|___/
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
// Exception declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Exceptions for tree-related operations. */
class Exception: public std::runtime_error {
public:
    Exception(const std::string &mesg)
        : std::runtime_error(mesg) {}
};

/** Exception if tree consistency would be violated.
 *
 *  If the user attempts to modify the connectivity of a tree in a way that would cause it to become inconsistent, then this
 *  exception is thrown. Examples of inconsistency are:
 *
 *  @li Attempting to insert a node in such a way that it would have two different parents.
 *
 *  @li Attempting to insert a node at a position that would cause it to be a sibling of itself.
 *
 *  @li Attempting to insert a node in such a way that it would become its own parent.
 *
 *  @li Attempting to insert a node in such a way as to cause a cycle in the connectivity. */
class ConsistencyException: public Exception {
public:
    std::shared_ptr<Node> child;                        /**< Child node that was being modified. */

    ConsistencyException(const std::shared_ptr<Node> &child,
                         const std::string &mesg = "attempt to attach child that already has a different parent")
        : Exception(mesg), child(child) {}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChildEdgeBase declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Non-template base class for parent-to-child edges in the tree.
class ChildEdgeBase {
protected:
    Node *container_;                                   // non-null ptr to node that's the source of this edge
    const size_t idx_;                                  // index of the child edge from the parent's perspective

    explicit ChildEdgeBase(Node *container);
    ChildEdgeBase(Node *container, const std::shared_ptr<Node> &child);

    void assign(const std::shared_ptr<Node> &child);
    std::shared_ptr<Node> get() const;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChildEdge declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** An edge from a parent to a child. */
template<class T>
class ChildEdge final: private ChildEdgeBase {
public:
    /** Points to no child. */
    explicit ChildEdge(Node *container);

    /** Constructor that points to a child. */
    ChildEdge(Node *container, const std::shared_ptr<T> &child);

    ChildEdge(const ChildEdge&) = delete;               // just for now

    /** Point to a child node. */
    ChildEdge& operator=(const std::shared_ptr<T> &child) {
        this->assign(child);
        return *this;
    }

    /** Cause this edge to point to no child. */
    void reset() {
        this->assign(nullptr);
    }

    /** Obtain shared pointer. */
    std::shared_ptr<T> operator->() const {
        return child();
    }

    /** Obtain pointed-to node. */
    T& operator*() {
        ASSERT_not_null(child());
        return *child();
    }

    /** Conversion to bool. */
    explicit operator bool() const {
        return this->get() != nullptr;
    }

    /** Pointer to the child. */
    std::shared_ptr<T> child() const;

    /** Implicit conversion to shared pointer. */
    operator std::shared_ptr<T>() const {
        return child();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ParentEdge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Edge pointing from child to parent. */
class ParentEdge final {
private:
    Node* parent_;

public:
    ParentEdge()
        : parent_(nullptr) {}

    /** Obtain shared pointer. */
    std::shared_ptr<Node> operator->() {
        return parent();
    }

    /** Obtain pointed-to node. */
    Node& operator*() {
        ASSERT_not_null(parent_);
        return *parent();
    }
    
    /** Conversion to bool. */
    explicit operator bool() const {
        return parent_ != nullptr;
    }

    /** Relation.
     *
     * @{ */
    bool operator==(const ParentEdge &other) const { return parent_ == other.parent_; }
    bool operator!=(const ParentEdge &other) const { return parent_ != other.parent_; }
    bool operator< (const ParentEdge &other) const { return parent_ <  other.parent_; }
    bool operator<=(const ParentEdge &other) const { return parent_ <= other.parent_; }
    bool operator> (const ParentEdge &other) const { return parent_ >  other.parent_; }
    bool operator>=(const ParentEdge &other) const { return parent_ >= other.parent_; }
    /** @} */

public:
    // internal
    std::shared_ptr<Node> parent() const;

    // internal
    void set(Node *parent) {
        parent_ = parent;
    }

    // internal
    void reset() {
        parent_ = nullptr;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Children declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Vector of parent-to-child pointers.
 *
 *  This object has an API similar to <tt>const std::vector<std::shared_ptr<Node>></tt>. */
class Children final {
private:
    Node *container_;
    std::vector<std::shared_ptr<Node> > children_;

public:
    Children(Node *container);
    Children(const Children&) = delete;
    Children& operator=(const Children&) = delete;

    //----------------------------------------
    //           Read-only API
    //----------------------------------------
public:
    /** Number of nodes in vector. */
    size_t size() const {
        return children_.size();
    }

    /** Maximum potential size. */
    size_t max_size() const {
        return children_.max_size();
    }

    /** Size of allocated storage. */
    size_t capacity() const {
        return children_.capacity();
    }

    /** Empty predicate. */
    bool empty() const {
        return children_.empty();
    }

    /** Request change in capacity. */
    void reserve(size_t n) {
        children_.reserve(n);
    }

    /** Request container to reduce capacity. */
    void shrink_to_fit() {
        children_.shrink_to_fit();
    }

    /** Child pointer at index, checked. */
    const std::shared_ptr<Node> at(size_t idx) const {
        ASSERT_require(idx < children_.size());
        return children_.at(idx);
    }

    /** Child pointer at index, unchecked. */
    const std::shared_ptr<Node> operator[](size_t idx) const {
        ASSERT_require(idx < children_.size());
        return children_[idx];
    }

    /** First child pointer. */
    const std::shared_ptr<Node> front() const {
        ASSERT_forbid(empty());
        return children_.front();
    }

    /** Last child pointer. */
    const std::shared_ptr<Node> back() const {
        ASSERT_forbid(empty());
        return children_.back();
    }

    /** The actual underlying vector of child pointers. */
    const std::vector<std::shared_ptr<Node> >& elmts() const {
        return children_;
    }

    /** Relations.
     *
     * @{ */
    bool operator==(const Children &other) const { return children_ == other.children_; }
    bool operator!=(const Children &other) const { return children_ != other.children_; }
    bool operator< (const Children &other) const { return children_ <  other.children_; }
    bool operator<=(const Children &other) const { return children_ <= other.children_; }
    bool operator> (const Children &other) const { return children_ >  other.children_; }
    bool operator>=(const Children &other) const { return children_ >= other.children_; }
    /** @} */

    //----------------------------------------
    //              Modifying API
    //----------------------------------------
public:
    /** Cause the indicated parent-child edge to point to a new child.
     *
     *  The old value, if any will be removed from the tree and its parent pointer reset. The new child will be added to the
     *  tree at the specified parent-to-child edge and its parent pointer adjusted to point to the node that now owns it. As with
     *  direct assignment of pointers to the parent-to-child edge data members, it is illegal to attach a node to a tree in such
     *  a way that the structure would no longer be a tree, and in such cases no changes are made an an exception is thrown.
     *
     * @{ */
    void setAt(size_t idx, const std::shared_ptr<Node> &newChild);
    void setAt(size_t idx, nullptr_t) {
        setAt(idx, std::shared_ptr<Node>());
    }
    /** @} */

    //----------------------------------------
    //              Internal stuff
    //----------------------------------------
public: // FIXME[Robb Matzke 2019-02-18]: These should be private
    // Check that the newChild is able to be inserted replacing the oldChild. If not, throw exception. Either or both of the
    // children may be null pointers, but 'parent' must be non-null.
    void checkInsertionConsistency(const std::shared_ptr<Node> &newChild, const std::shared_ptr<Node> &oldChild, Node *parent);

    // Insert a new child edge at the specified position in the list. Consistency checks are performed and the child's parent
    // pointer is adjusted.
    void insertAt(size_t idx, const std::shared_ptr<Node> &child);

    // Remove one of the child edges. If the edge pointed to a non-null child node, then that child's parent pointer is reset.
    void eraseAt(size_t idx);
    
    // Add a new parent-child-edge and return its index
    size_t appendEdge(const std::shared_ptr<Node> &child);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Base class for @ref Tree nodes. */
class Node: public std::enable_shared_from_this<Node> {
private:
    enum TraversalDirection { TRAVERSE_UPWARD, TRAVERSE_DOWNWARD };

public:
    ParentEdge parent;                                  /**< Pointer to the parent node, if any. */
    Children children;                                  /**< Vector of pointers to children. */

public:
    Node(): children(this) {}
    virtual ~Node() {}
    
    // Nodes are not copyable since doing so would cause the children (which are not copied) to have two parents. Instead, we
    // will provide mechanisms for copying nodes without their children (shallow copy) or recursively copying an entire tree
    // (deep copy).
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

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
     *  returns CONTINUE. */
    template<class Functor>
    TraversalAction traverse(Functor functor) {
        return traverseImpl<Functor>(TRAVERSE_DOWNWARD, functor);
    }

    /** Traverse the tree by following parent pointers.
     *
     *  Other than following pointers from children to parents, this traversal is identical to the downward @ref traverse
     *  method. */
    template<class Functor>
    TraversalAction traverseParents(Functor functor) {
        return traverseImpl<Functor>(TRAVERSE_UPWARD, functor);
    }

    /** Traverse an tree to find the first node satisfying the predicate. */
    template<class Predicate>
    std::shared_ptr<Node> find(Predicate predicate) {
        return findImpl<Node, Predicate>(TRAVERSE_DOWNWARD, predicate);
    }

    /** Find first child that's the specified type. */
    template<class T>
    std::shared_ptr<T> findType() {
        return findImpl<T>(TRAVERSE_DOWNWARD, [](const std::shared_ptr<T>&) { return true; });
    }

    /** Find first child of specified type satisfying the predicate. */
    template<class T, class Predicate>
    std::shared_ptr<T> findType(Predicate predicate) {
        return findImpl<T, Predicate>(TRAVERSE_DOWNWARD, predicate);
    }

    /** Find closest ancestor that satifies the predicate. */
    template<class Predicate>
    std::shared_ptr<Node> findParent(Predicate predicate) {
        return findImpl<Node, Predicate>(TRAVERSE_UPWARD, predicate);
    }

    /** Find closest ancestor of specified type. */
    template<class T>
    std::shared_ptr<T> findParentType() {
        return findImpl<T>(TRAVERSE_UPWARD, [](const std::shared_ptr<T>&) { return true; });
    }

    /** Find closest ancestor of specified type that satisfies the predicate. */
    template<class T, class Predicate>
    std::shared_ptr<T> findParentType(Predicate predicate) {
        return findImpl<T, Predicate>(TRAVERSE_UPWARD, predicate);
    }

private:
    template<class Functor>
    TraversalAction traverseImpl(TraversalDirection, Functor);

    template<class T, class Predicate>
    std::shared_ptr<T> findImpl(TraversalDirection, Predicate);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ListNode declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** A node containing only a list of children.
 *
 *  This class is used for nodes whose sole purpose is to hold a list of child nodes. Rather than having any dedicated data
 *  members, it accesses the @c children member directly in order to store the ordered list of child pointers. New classes
 *  cannot be derived from this class since doing so would enable the derived class to have additional ChildPtr data members
 *  that would interfere with the @c children list.
 *
 *  Although the @c children data member provides a read-only API for accessing the children, we also need to provde an API
 *  that can modify that list.  The entire @c children API is available also from this node directly so that the reading and
 *  writing APIs can be invoked consistently on this object. */
template<class T>
class ListNode final: public Node {
public:
    //----------------------------------------
    // Read-only API delegated to 'children'
    //----------------------------------------

    /** Number of children. */
    size_t size() const {
        return children.size();
    }

    /** Maximum size. */
    size_t max_size() const {
        return children.max_size();
    }

    /** Capacity. */
    size_t capacity() const {
        return children.capacity();
    }

    /** Empty predicate. */
    bool empty() const {
        return children.empty();
    }

    /** Reserve space for more children. */
    void reserve(size_t n) {
        children.reserve(n);
    }

    /** Shrink reservation. */
    void shrink_to_fit() {
        children.shrink_to_fit();
    }

    /** Child at specified index. */
    const std::shared_ptr<T> at(size_t i) const {
        return std::dynamic_pointer_cast<T>(children.at(i));
    }

    /** Child at specified index. */
    const std::shared_ptr<T> operator[](size_t i) const {
        return std::dynamic_pointer_cast<T>(children[i]);
    }

    /** First child, if any. */
    const std::shared_ptr<T> front() const {
        return std::dynamic_pointer_cast<T>(children.front());
    }

    /** Last child, if any. */
    const std::shared_ptr<T> back() const {
        return std::dynamic_pointer_cast<T>(children.back());
    }

    /** Vector of all children. */
    std::vector<std::shared_ptr<T> > elmts() const;

    /** Find the index for the specified node.
     *
     *  Finds the index for the first child at or after @p startAt and returns its index. Returns nothing if the specified
     *  node is not found. */
    Optional<size_t> index(const std::shared_ptr<T> &node, size_t startAt = 0) const;

    //----------------------------------------
    //              Modifying API
    //----------------------------------------

    /** Append a child pointer. */
    void push_back(const std::shared_ptr<T> &newChild) {
        children.insertAt(children.size(), newChild);
    }

    /** Make child edge point to a different child.
     *
     * @{ */
    void setAt(size_t i, std::shared_ptr<T> &child) {
        children.setAt(i, child);
    }
    void setAt(size_t i, nullptr_t) {
        children.setAt(i, nullptr);
    }
    /** @} */
    
    
    /** Insert the node at the specified index.
     *
     *  The node must not already have a parent. The index must be greater than or equal to zero and less than or equal to the
     *  current number of nodes. Upon return, the node that was inserted will be found at index @p i. */
    void insertAt(size_t i, std::shared_ptr<T> &newChild) {
        children.insertAt(i, newChild);
    }
    
    /** Erase node at specified index.
     *
     *  If the index is out of range then nothing happens. */
    void eraseAt(size_t i) {
        children.eraseAt(i);
    }

    /** Erase the first occurrence of the specified child.
     *
     *  Erases the first occurrence of the specified child at or after the starting index.
     *
     *  If a child was erased, then return the index of the erased child. */
    Optional<size_t> erase(const std::shared_ptr<T> &toErase, size_t startAt = 0);
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                        ____      _       _   _
//                       |  _ \ ___| | __ _| |_(_) ___  _ __  ___
//                       | |_) / _ \ |/ _` | __| |/ _ \| '_ \/ __|
//                       |  _ <  __/ | (_| | |_| | (_) | | | \__ |
//                       |_| \_\___|_|\__,_|\__|_|\___/|_| |_|___/
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ChildEdge<T> and ChildEdge<U>
template<class T, class U> bool operator==(const ChildEdge<T> &lhs, const ChildEdge<U> &rhs) noexcept { return lhs.child() == rhs.child(); }
template<class T, class U> bool operator!=(const ChildEdge<T> &lhs, const ChildEdge<U> &rhs) noexcept { return lhs.child() != rhs.child(); }
template<class T, class U> bool operator< (const ChildEdge<T> &lhs, const ChildEdge<U> &rhs) noexcept { return lhs.child() <  rhs.child(); }
template<class T, class U> bool operator<=(const ChildEdge<T> &lhs, const ChildEdge<U> &rhs) noexcept { return lhs.child() <= rhs.child(); }
template<class T, class U> bool operator> (const ChildEdge<T> &lhs, const ChildEdge<U> &rhs) noexcept { return lhs.child() >  rhs.child(); }
template<class T, class U> bool operator>=(const ChildEdge<T> &lhs, const ChildEdge<U> &rhs) noexcept { return lhs.child() >= rhs.child(); }

// ChildEdge<T> and nullptr
template<class T> bool operator==(const ChildEdge<T> &lhs, nullptr_t) noexcept { return lhs.child() == nullptr; }
template<class T> bool operator!=(const ChildEdge<T> &lhs, nullptr_t) noexcept { return lhs.child() != nullptr; }
template<class T> bool operator< (const ChildEdge<T> &lhs, nullptr_t) noexcept { return lhs.child() <  nullptr; }
template<class T> bool operator<=(const ChildEdge<T> &lhs, nullptr_t) noexcept { return lhs.child() <= nullptr; }
template<class T> bool operator> (const ChildEdge<T> &lhs, nullptr_t) noexcept { return lhs.child() >  nullptr; }
template<class T> bool operator>=(const ChildEdge<T> &lhs, nullptr_t) noexcept { return lhs.child() >= nullptr; }

// nullptr and ChildEdge<T>
template<class T> bool operator==(nullptr_t, const ChildEdge<T> &rhs) noexcept { return nullptr == rhs.child(); }
template<class T> bool operator!=(nullptr_t, const ChildEdge<T> &rhs) noexcept { return nullptr != rhs.child(); }
template<class T> bool operator< (nullptr_t, const ChildEdge<T> &rhs) noexcept { return nullptr <  rhs.child(); }
template<class T> bool operator<=(nullptr_t, const ChildEdge<T> &rhs) noexcept { return nullptr <= rhs.child(); }
template<class T> bool operator> (nullptr_t, const ChildEdge<T> &rhs) noexcept { return nullptr >  rhs.child(); }
template<class T> bool operator>=(nullptr_t, const ChildEdge<T> &rhs) noexcept { return nullptr >= rhs.child(); }

// ChildEdge<T> and std::shared_ptr<U>
template<class T, class U> bool operator==(const ChildEdge<T> &lhs, const std::shared_ptr<U> &rhs) noexcept { return lhs.child() == rhs; }
template<class T, class U> bool operator!=(const ChildEdge<T> &lhs, const std::shared_ptr<U> &rhs) noexcept { return lhs.child() != rhs; }
template<class T, class U> bool operator< (const ChildEdge<T> &lhs, const std::shared_ptr<U> &rhs) noexcept { return lhs.child() <  rhs; }
template<class T, class U> bool operator<=(const ChildEdge<T> &lhs, const std::shared_ptr<U> &rhs) noexcept { return lhs.child() <= rhs; }
template<class T, class U> bool operator> (const ChildEdge<T> &lhs, const std::shared_ptr<U> &rhs) noexcept { return lhs.child() >  rhs; }
template<class T, class U> bool operator>=(const ChildEdge<T> &lhs, const std::shared_ptr<U> &rhs) noexcept { return lhs.child() >= rhs; }

// std::shared_ptr<T> and ChildEdge<U>
template<class T, class U> bool operator==(const std::shared_ptr<T> &lhs, const ChildEdge<U> &rhs) noexcept { return lhs == rhs.child(); }
template<class T, class U> bool operator!=(const std::shared_ptr<T> &lhs, const ChildEdge<U> &rhs) noexcept { return lhs != rhs.child(); }
template<class T, class U> bool operator< (const std::shared_ptr<T> &lhs, const ChildEdge<U> &rhs) noexcept { return lhs <  rhs.child(); }
template<class T, class U> bool operator<=(const std::shared_ptr<T> &lhs, const ChildEdge<U> &rhs) noexcept { return lhs <= rhs.child(); }
template<class T, class U> bool operator> (const std::shared_ptr<T> &lhs, const ChildEdge<U> &rhs) noexcept { return lhs >  rhs.child(); }
template<class T, class U> bool operator>=(const std::shared_ptr<T> &lhs, const ChildEdge<U> &rhs) noexcept { return lhs >= rhs.child(); }

// ParentEdge and nullptr
inline bool operator==(const ParentEdge &lhs, nullptr_t) noexcept { return lhs.parent() == nullptr; }
inline bool operator!=(const ParentEdge &lhs, nullptr_t) noexcept { return lhs.parent() != nullptr; }
inline bool operator< (const ParentEdge &lhs, nullptr_t) noexcept { return lhs.parent() <  nullptr; }
inline bool operator<=(const ParentEdge &lhs, nullptr_t) noexcept { return lhs.parent() <= nullptr; }
inline bool operator> (const ParentEdge &lhs, nullptr_t) noexcept { return lhs.parent() >  nullptr; }
inline bool operator>=(const ParentEdge &lhs, nullptr_t) noexcept { return lhs.parent() >= nullptr; }

// nullptr and ParentEdge
inline bool operator==(nullptr_t, const ParentEdge &rhs) noexcept { return nullptr == rhs.parent(); }
inline bool operator!=(nullptr_t, const ParentEdge &rhs) noexcept { return nullptr != rhs.parent(); }
inline bool operator< (nullptr_t, const ParentEdge &rhs) noexcept { return nullptr <  rhs.parent(); }
inline bool operator<=(nullptr_t, const ParentEdge &rhs) noexcept { return nullptr <= rhs.parent(); }
inline bool operator> (nullptr_t, const ParentEdge &rhs) noexcept { return nullptr >  rhs.parent(); }
inline bool operator>=(nullptr_t, const ParentEdge &rhs) noexcept { return nullptr >= rhs.parent(); }

// ParentEdge and std::shared_ptr<T>
template<class T> bool operator==(const ParentEdge &lhs, const std::shared_ptr<T> &rhs) noexcept { return lhs.parent() == rhs; }
template<class T> bool operator!=(const ParentEdge &lhs, const std::shared_ptr<T> &rhs) noexcept { return lhs.parent() != rhs; }
template<class T> bool operator< (const ParentEdge &lhs, const std::shared_ptr<T> &rhs) noexcept { return lhs.parent() <  rhs; }
template<class T> bool operator<=(const ParentEdge &lhs, const std::shared_ptr<T> &rhs) noexcept { return lhs.parent() <= rhs; }
template<class T> bool operator> (const ParentEdge &lhs, const std::shared_ptr<T> &rhs) noexcept { return lhs.parent() >  rhs; }
template<class T> bool operator>=(const ParentEdge &lhs, const std::shared_ptr<T> &rhs) noexcept { return lhs.parent() >= rhs; }

// std::shared_ptr<T> and ParentEdge
template<class T> bool operator==(const std::shared_ptr<T> &lhs, const ParentEdge &rhs) noexcept { return lhs == rhs.parent(); }
template<class T> bool operator!=(const std::shared_ptr<T> &lhs, const ParentEdge &rhs) noexcept { return lhs != rhs.parent(); }
template<class T> bool operator< (const std::shared_ptr<T> &lhs, const ParentEdge &rhs) noexcept { return lhs <  rhs.parent(); }
template<class T> bool operator<=(const std::shared_ptr<T> &lhs, const ParentEdge &rhs) noexcept { return lhs <= rhs.parent(); }
template<class T> bool operator> (const std::shared_ptr<T> &lhs, const ParentEdge &rhs) noexcept { return lhs >  rhs.parent(); }
template<class T> bool operator>=(const std::shared_ptr<T> &lhs, const ParentEdge &rhs) noexcept { return lhs >= rhs.parent(); }

// ParentEdge and ChildEdge<T>
template<class T> bool operator==(const ParentEdge &lhs, const ChildEdge<T> &rhs) noexcept { return lhs.parent() == rhs.child(); }
template<class T> bool operator!=(const ParentEdge &lhs, const ChildEdge<T> &rhs) noexcept { return lhs.parent() != rhs.child(); }
template<class T> bool operator< (const ParentEdge &lhs, const ChildEdge<T> &rhs) noexcept { return lhs.parent() <  rhs.child(); }
template<class T> bool operator<=(const ParentEdge &lhs, const ChildEdge<T> &rhs) noexcept { return lhs.parent() <= rhs.child(); }
template<class T> bool operator> (const ParentEdge &lhs, const ChildEdge<T> &rhs) noexcept { return lhs.parent() >  rhs.child(); }
template<class T> bool operator>=(const ParentEdge &lhs, const ChildEdge<T> &rhs) noexcept { return lhs.parent() >= rhs.child(); }

// ChildEdge<T> and ParentEdge
template<class T> bool operator==(const ChildEdge<T> &lhs, const ParentEdge &rhs) noexcept { return lhs.child() == rhs.parent(); }
template<class T> bool operator!=(const ChildEdge<T> &lhs, const ParentEdge &rhs) noexcept { return lhs.child() != rhs.parent(); }
template<class T> bool operator< (const ChildEdge<T> &lhs, const ParentEdge &rhs) noexcept { return lhs.child() <  rhs.parent(); }
template<class T> bool operator<=(const ChildEdge<T> &lhs, const ParentEdge &rhs) noexcept { return lhs.child() <= rhs.parent(); }
template<class T> bool operator> (const ChildEdge<T> &lhs, const ParentEdge &rhs) noexcept { return lhs.child() >  rhs.parent(); }
template<class T> bool operator>=(const ChildEdge<T> &lhs, const ParentEdge &rhs) noexcept { return lhs.child() >= rhs.parent(); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                        ___                 _                           _        _   _
//                       |_ _|_ __ ___  _ __ | | ___ _ __ ___   ___ _ __ | |_ __ _| |_(_) ___  _ __  ___
//                        | || '_ ` _ \| '_ \| |/ _ \ '_ ` _ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \/ __|
//                        | || | | | | | |_) | |  __/ | | | | |  __/ | | | || (_| | |_| | (_) | | | \__ |
//                       |___|_| |_| |_| .__/|_|\___|_| |_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_|___/
//                                     |_|
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChildEdgeBase implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChildEdgeBase::ChildEdgeBase(Node *container)
    : container_(container), idx_(container->children.appendEdge(nullptr)) {
    ASSERT_not_null(container_);
}

ChildEdgeBase::ChildEdgeBase(Node *container, const std::shared_ptr<Node> &child)
    : container_(container), idx_(container->children.appendEdge(child)) {
    ASSERT_not_null(container_);
}

std::shared_ptr<Node>
ChildEdgeBase::get() const {
    return container_->children[idx_];
}

void
ChildEdgeBase::assign(const std::shared_ptr<Node> &newChild) {
    container_->children.setAt(idx_, newChild);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChildEdge implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
ChildEdge<T>::ChildEdge(Node *container)
    : ChildEdgeBase(container) {
}

template<class T>
ChildEdge<T>::ChildEdge(Node *container, const std::shared_ptr<T> &child)
    : ChildEdgeBase(container, child) {
}

template<class T>
std::shared_ptr<T>
ChildEdge<T>::child() const {
    return std::dynamic_pointer_cast<T>(container_->children[this->idx_]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ParentEdge implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<Node>
ParentEdge::parent() const {
    return parent_ ? parent_->shared_from_this() : std::shared_ptr<Node>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Children implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Children::Children(Node *container)
    : container_(container) {
    ASSERT_not_null(container);
}

size_t
Children::appendEdge(const std::shared_ptr<Node> &child) {
    size_t idx = children_.size();
    children_.push_back(nullptr);
    setAt(idx, child);
    return idx;
}

void
Children::checkInsertionConsistency(const std::shared_ptr<Node> &newChild, const std::shared_ptr<Node> &oldChild, Node *parent) {
    ASSERT_not_null(parent);

    if (newChild && newChild != oldChild) {
        if (newChild->parent != nullptr) {
            if (newChild->parent.parent().get() == parent) {
                throw ConsistencyException(newChild, "node is already a child of the parent");
            } else {
                throw ConsistencyException(newChild, "node is already attached to a tree");
            }
        }

        for (Node *ancestor = parent; ancestor; ancestor = ancestor->parent.parent().get()) {
            if (newChild.get() == ancestor)
                throw ConsistencyException(newChild, "node insertion would introduce a cycle");
        }
    }
}

void
Children::setAt(size_t idx, const std::shared_ptr<Node> &newChild) {
    ASSERT_require(idx < children_.size());
    std::shared_ptr<Node> oldChild = children_[idx];
    checkInsertionConsistency(newChild, oldChild, container_);
    if (oldChild)
        oldChild->parent.reset();
    children_[idx] = newChild;
    if (newChild)
        newChild->parent.set(container_);
}

void
Children::insertAt(size_t idx, const std::shared_ptr<Node> &child) {
    ASSERT_require(idx <= children_.size());
    checkInsertionConsistency(child, nullptr, container_);
    children_.insert(children_.begin() + idx, child);
    if (child)
        child->parent.set(container_);
}

void
Children::eraseAt(size_t idx) {
    ASSERT_require(idx < children_.size());
    if (children_[idx])
        children_[idx]->parent.reset();
    children_.erase(children_.begin() + idx);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Functor>
TraversalAction
Node::traverseImpl(TraversalDirection direction, Functor functor) {
    switch (TraversalAction action = functor(this->shared_from_this(), ENTER)) {
        case CONTINUE:
            if (TRAVERSE_DOWNWARD == direction) {
                for (size_t i=0; i<children.size() && CONTINUE==action; ++i) {
                    if (children[i])
                        action = children[i]->traverseImpl(direction, functor);
                }
            } else if (parent) {
                action = parent->traverseImpl(direction, functor);
            }
            // fall through
        case SKIP_CHILDREN:
            if (ABORT != action && (action = functor(this->shared_from_this(), LEAVE)) == SKIP_CHILDREN)
                action = CONTINUE;
            // fall through
        default:
            return action;
    }
}

template<class T, class Predicate>
std::shared_ptr<T>
Node::findImpl(TraversalDirection direction, Predicate predicate) {
    std::shared_ptr<T> found;
    traverseImpl(direction,
                 [&predicate, &found](const std::shared_ptr<Node> &node, TraversalEvent event) {
                     if (ENTER == event) {
                         std::shared_ptr<T> typedNode = std::dynamic_pointer_cast<T>(node);
                         if (typedNode && predicate(typedNode)) {
                             found = typedNode;
                             return ABORT;
                         }
                     }
                     return CONTINUE;
                 });
    return found;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ListNode implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
std::vector<std::shared_ptr<T> >
ListNode<T>::elmts() const {
    std::vector<std::shared_ptr<T> > retval;
    retval.reserve(children.size());
    for (const std::shared_ptr<Node> &child: children)
        retval.push_back(child ? std::dynamic_pointer_cast<T>(child) : std::shared_ptr<T>());
    return retval;
}

template<class T>
Optional<size_t>
ListNode<T>::index(const std::shared_ptr<T> &node, size_t startAt) const {
    for (size_t i = startAt; i < children.size(); ++i) {
        if (children[i] == node)
            return i;
    }
    return Nothing();
}

template<class T>
Optional<size_t>
ListNode<T>::erase(const std::shared_ptr<T> &toErase, size_t startAt) {
    Optional<size_t> where = index(toErase, startAt);
    if (where)
        children.eraseAt(*where);
    return where;
}

} // namespace
} // namespace

#endif

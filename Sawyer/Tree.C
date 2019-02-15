#include <Sawyer/Tree.h>

namespace Sawyer {
namespace Tree {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChildPtrBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
ChildPtrBase::setParent(Node *parent, Node *child) {
    assert(child);
    child->parent = parent;
}

void
ChildPtrBase::setChild(Node *parent, size_t index, Node *child) {
    assert(parent);
    assert(index < parent->children.size());
    parent->children.setNode(index, child);
}

size_t
ChildPtrBase::newChildPtr(Node *parent, Node *child) {
    assert(parent);
    parent->children.push_back(child);
    return parent->children.size() - 1;
}

} // namespace
} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Non member function overloads
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool operator==(const Sawyer::Tree::NodeVector &a, const Sawyer::Tree::NodeVector &b) {
    return a.elmts() == b.elmts();
}

bool operator!=(const Sawyer::Tree::NodeVector &a, const Sawyer::Tree::NodeVector &b) {
    return a.elmts() != b.elmts();
}

bool operator< (const Sawyer::Tree::NodeVector &a, const Sawyer::Tree::NodeVector &b) {
    return a.elmts() < b.elmts();
}

bool operator<=(const Sawyer::Tree::NodeVector &a, const Sawyer::Tree::NodeVector &b) {
    return a.elmts() <= b.elmts();
}

bool operator> (const Sawyer::Tree::NodeVector &a, const Sawyer::Tree::NodeVector &b) {
    return a.elmts() > b.elmts();
}

bool operator>=(const Sawyer::Tree::NodeVector &a, const Sawyer::Tree::NodeVector &b) {
    return a.elmts() >= b.elmts();
}

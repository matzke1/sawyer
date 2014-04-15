#ifndef Sawyer_Graph_H
#define Sawyer_Graph_H

#include <sawyer/IndexedList.h>
#include <boost/range/iterator_range.hpp>
#if 1 /*DEBUGGING [Robb Matzke 2014-04-21]*/
#include <iomanip>
#endif

namespace Sawyer {
namespace Container {


template<class V, class E>
class Graph {
public:
    typedef V VertexValue;                              /**< User-level data associated with vertices. */
    typedef E EdgeValue;                                /**< User-level data associated with edges. */
    class Vertex;                                       /**< All information about a vertex. User info plus connectivity info. */
    class Edge;                                         /**< All information about an edge. User info plus connectivity info. */

private:
    enum EdgePhase { IN_EDGES=0, OUT_EDGES=1, N_PHASES=2 };
    typedef IndexedList<Edge> EdgeList;
    typedef IndexedList<Vertex> VertexList;

    template<class T>
    class VirtualList {
        VirtualList *next_[N_PHASES];
        VirtualList *prev_[N_PHASES];
        T *node_;
    public:
        VirtualList() {
            reset(NULL);
        }

        void reset(T* node) {
            node_ = node;
            for (size_t i=0; i<N_PHASES; ++i)
                next_[i] = prev_[i] = this;
        }

        bool isHead() const {
            return node_ == NULL;
        }

        bool isSingleton(EdgePhase phase) const {
            ASSERT_this();
            ASSERT_require(phase < N_PHASES);
            ASSERT_require((next_[phase]==this && prev_[phase]==this) || (next_[phase]!=this && prev_[phase]!=this));
            return next_[phase]==this;
        }

        bool isEmpty(EdgePhase phase) const {
            ASSERT_this();
            ASSERT_require(isHead());
            ASSERT_require((next_[phase]==this && prev_[phase]==this) || (next_[phase]!=this && prev_[phase]!=this));
            return next_[phase]==this;
        }

        void insert(EdgePhase phase, VirtualList *newNode) { // insert newNode before this
            ASSERT_this();
            ASSERT_require(phase < N_PHASES);
            ASSERT_not_null(newNode);
            ASSERT_forbid(newNode->isHead());
            ASSERT_require(newNode->isSingleton(phase)); // cannot be part of another sublist already
            prev_[phase]->next_[phase] = newNode;
            newNode->prev_[phase] = prev_[phase];
            prev_[phase] = newNode;
            newNode->next_[phase] = this;
        }

        void remove(EdgePhase phase) {                  // Remove this node from the list
            ASSERT_this();
            ASSERT_require(phase < N_PHASES);
            ASSERT_forbid(isHead());
            prev_[phase]->next_[phase] = next_[phase];
            next_[phase]->prev_[phase] = prev_[phase];
            next_[phase] = prev_[phase] = this;
        }

        VirtualList& next(EdgePhase phase) { return *next_[phase]; }
        const VirtualList& next(EdgePhase phase) const { return *next_[phase]; }
        VirtualList& prev(EdgePhase phase) { return *prev_[phase]; }
        const VirtualList& prev(EdgePhase phase) const { return *prev_[phase]; }

        T& dereference() {                              // Return the Edge to which this VirtualList node belongs
            ASSERT_this();
            ASSERT_forbid(isHead());                    // list head contains no user-data
            return *(T*)this;                           // depends on VirtualList being at the beginning of Edge
        }

        const T& dereference() const {
            ASSERT_this();
            ASSERT_forbid(isHead());
            return *(const T*)this;
        }

#if 1 /*DEBUGGING [Robb Matzke 2014-04-21]*/
        void dump(EdgePhase phase, std::ostream &o) const {
            const VirtualList *cur = this;
            o <<"  " <<std::setw(18) <<"Node"
              <<"\t" <<std::setw(18) <<"This"
              <<"\t" <<std::setw(18) <<"Next"
              <<"\t" <<std::setw(18) <<"Prev\n";
            do {
                o <<"  " <<std::setw(18) <<node_
                  <<"\t" <<std::setw(18) <<cur
                  <<"\t" <<std::setw(18) <<cur->next_[phase]
                  <<"\t" <<std::setw(18) <<cur->prev_[phase] <<"\n";
                cur = cur->next_[phase];
            } while (cur!=this && cur->next_[phase]!=cur);
        }
#endif
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                                  Iterators
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    // Edge iterators always ultimately point to an Edge (or const Edge), but do so either through an EdgeList iterator
    // or a VirtualList<Edge>.  The "phase" of the iterator determines which list is being traversed even though all three
    // lists (graph edges, in edges, and out edges) share the same memory.
    template<class Derived, class Value, class BaseIter>
    class EdgeBaseIterator: public std::iterator<std::bidirectional_iterator_tag, Value> {
        EdgePhase phase_;                               // IN_EDGES, OUT_EDGES or N_PHASES (graph edges)
        BaseIter iter_;                                 // EdgeList::NodeIterator or EdgeList::ConstNodeIterator
        VirtualList<Edge> *vlist_;                      // Used when phase_ is IN_EDGES or OUT_EDGES
    protected:
        friend class Graph;
        EdgeBaseIterator() {}
        EdgeBaseIterator(const EdgeBaseIterator &other): phase_(other.phase_), iter_(other.iter_), vlist_(other.vlist_) {}
        EdgeBaseIterator(const BaseIter &iter): phase_(N_PHASES), iter_(iter), vlist_(NULL) {}
        EdgeBaseIterator(EdgePhase phase, VirtualList<Edge> *vlist): phase_(phase), vlist_(vlist) {}
        template<class BaseIter2> EdgeBaseIterator(EdgePhase phase, const BaseIter2 &iter, VirtualList<Edge> *vlist)
            : phase_(phase), iter_(iter), vlist_(vlist) {}

        Edge& dereference() const {
            return N_PHASES==phase_ ? iter_->value() : vlist_->dereference();
        }

    private:
        Derived* derived() { return static_cast<Derived*>(this); }
        const Derived* derived() const { return static_cast<const Derived*>(this); }

    public:
        Derived& operator=(const Derived &other) {
            phase_ = other.phase_;
            iter_ = other.iter_;
            vlist_ = other.vlist_;
            return *derived();
        }

        Derived& operator++() {
            if (N_PHASES==phase_) {
                ++iter_;
            } else {
                vlist_ = &vlist_->next(phase_);
            }
            return *derived();
        }

        Derived operator++(int) {
            Derived old = *this;
            ++*this;
            return old;
        }

        Derived& operator--() {
            if (N_PHASES==phase_) {
                --iter_;
            } else {
                vlist_ = &vlist_->prev(phase_);
            }
            return *derived();
        }

        Derived operator--(int) {
            Derived old = *this;
            --*this;
            return old;
        }

        // Iterators pointing to the same Edge are considered equal regardless of whether they're traversing the graph edges,
        // in-edges, or out-edges.   Any two end iterators are considered equal regardless of the list they're traversing.
        template<class OtherIter>
        bool operator==(const OtherIter &other) const {
            const Edge *a = NULL;
            if (N_PHASES==phase_) {
                a = iter_.isAtEnd() ? NULL : &iter_->value();
            } else {
                a = vlist_->isHead() ? NULL : &vlist_->dereference();
            }
            const Edge *b = NULL;
            if (N_PHASES==other.phase_) {
                b = other.iter_.isAtEnd() ? NULL : &other.iter_->value();
            } else {
                b = other.vlist_->isHead() ? NULL : &other.vlist_->dereference();
            }
            return a == b;
        }

        template<class OtherIter>
        bool operator!=(const OtherIter &other) const {
            return !(*this==other);
        }
    };

    template<class Derived, class Value, class BaseIter>
    class VertexBaseIterator: public std::iterator<std::bidirectional_iterator_tag, Value> {
        BaseIter base_;                                 // VertexList::NodeIterator or VertexList::ConstNodeIterator
    protected:
        friend class Graph;
        VertexBaseIterator() {}
        VertexBaseIterator(const VertexBaseIterator &other): base_(other.base_) {}
        VertexBaseIterator(const BaseIter &base): base_(base) {}
        Vertex& dereference() const { return base_->value(); }
    public:
        Derived& operator=(const Derived &other) { base_ = other.base_; return *derived(); }
        Derived& operator++() { ++base_; return *derived(); }
        Derived operator++(int) { Derived old=*derived(); ++*this; return old; }
        Derived& operator--() { --base_; return *derived(); }
        Derived operator--(int) { Derived old=*derived(); --*this; return old; }
        template<class OtherIter> bool operator==(const OtherIter &other) const { return base_ == other.base_; }
        template<class OtherIter> bool operator!=(const OtherIter &other) const { return base_ != other.base_; }
    private:
        Derived* derived() { return static_cast<Derived*>(this); }
        const Derived* derived() const { return static_cast<const Derived*>(this); }
    };

public:
    class EdgeNodeIterator: public EdgeBaseIterator<EdgeNodeIterator, Edge, typename EdgeList::NodeIterator> {
        typedef                    EdgeBaseIterator<EdgeNodeIterator, Edge, typename EdgeList::NodeIterator> Super;
    public:
        EdgeNodeIterator() {}
        EdgeNodeIterator(const EdgeNodeIterator &other): Super(other) {}
        Edge& operator*() const { return this->dereference(); }
        Edge* operator->() const { return &this->dereference(); }
    private:
        friend class Graph;
        EdgeNodeIterator(const typename EdgeList::NodeIterator &base): Super(base) {}
        EdgeNodeIterator(EdgePhase phase, VirtualList<Edge> *vlist): Super(phase, vlist) {}
    };

#if 0 // [Robb Matzke 2014-04-21]: not needed yet
    class ConstEdgeNodeIterator: public EdgeBaseIterator<ConstEdgeNodeIterator, const Edge,
                                                         typename EdgeList::ConstNodeIterator> {
        typedef                         EdgeBaseIterator<ConstEdgeNodeIterator, const Edge,
                                                         typename EdgeList::ConstNodeIterator> Super;
    public:
        ConstEdgeNodeIterator() {}
        ConstEdgeNodeIterator(const ConstEdgeNodeIterator &other): Super(other) {}
        const Edge& operator*() const { return this->dereference(); }
        const Edge* operator->() const { return &this->dereference(); }
    private:
        friend class Graph;
        ConstEdgeNodeIterator(const typename EdgeList::ConstNodeIterator &base): Super(base) {}
        ConstEdgeNodeIterator(EdgePhase phase, VirtualList<Edge> *vlist): Super(phase, vlist) {}
    };
#endif

    class EdgeValueIterator: public EdgeBaseIterator<EdgeValueIterator, EdgeValue, typename EdgeList::NodeIterator> {
        typedef                     EdgeBaseIterator<EdgeValueIterator, EdgeValue, typename EdgeList::NodeIterator> Super;
    public:
        EdgeValueIterator() {}
        EdgeValueIterator(const EdgeValueIterator &other): Super(other) {}
        EdgeValueIterator(const EdgeNodeIterator &other): Super(other.phase_, other.iter_, other.vlist_) {}
        EdgeValue& operator*() const { return this->dereference().value(); }
        EdgeValue* operator->() const { return &this->dereference().value(); }
    private:
        friend class Graph;
        EdgeValueIterator(const typename EdgeList::NodeIterator &base): Super(base) {}
        EdgeValueIterator(EdgePhase phase, VirtualList<Edge> *vlist): Super(phase, vlist) {}
    };

#if 0 // [Robb Matzke 2014-04-21]: not needed yet
    class ConstEdgeValueIterator: public EdgeBaseIterator<ConstEdgeValueIterator, const EdgeValue,
                                                          typename EdgeList::ConstNodeIterator> {
        typedef                          EdgeBaseIterator<ConstEdgeValueIterator, const EdgeValue,
                                                          typename EdgeList::ConstNodeIterator> Super;
    public:
        ConstEdgeValueIterator() {}
        ConstEdgeValueIterator(const ConstEdgeValueIterator &other): Super(other) {}
        const EdgeValue& operator*() const { return this->dereference().value(); }
        const EdgeValue* operator->() const { return &this->dereference().value(); }
    private:
        friend class Graph;
        ConstEdgeValueIterator(const typename EdgeList::ConstNodeIterator &base): Super(base) {}
        ConstEdgeValueIterator(EdgePhase phase, VirtualList<Edge> *vlist): Super(phase, vlist) {}
    };
#endif

    class VertexNodeIterator: public VertexBaseIterator<VertexNodeIterator, Vertex, typename VertexList::NodeIterator> {
        typedef                      VertexBaseIterator<VertexNodeIterator, Vertex, typename VertexList::NodeIterator> Super;
    public:
        VertexNodeIterator() {}
        VertexNodeIterator(const VertexNodeIterator &other): Super(other) {}
        Vertex& operator*() const { return this->dereference(); }
        Vertex* operator->() const { return &this->dereference(); }
    private:
        friend class Graph;
        VertexNodeIterator(const typename VertexList::NodeIterator &base): Super(base) {}
    };

#if 0 // [Robb Matzke 2014-04-21]: not needed yet
    class ConstVertexNodeIterator: public VertexBaseIterator<ConstVertexNodeIterator, const Vertex,
                                                             typename VertexList::ConstNodeIterator> {
        typedef                           VertexBaseIterator<ConstVertexNodeIterator, const Vertex,
                                                             typename VertexList::ConstNodeIterator> Super;
    public:
        ConstVertexNodeIterator() {}
        ConstVertexNodeIterator(const ConstVertexNodeIterator &other): Super(other) {}
        const Vertex& operator*() const { return this->dereference(); }
        const Vertex* operator->() const { return &this->dereference(); }
    private:
        friend class Graph;
        ConstVertexNodeIterator(const typename VertexList::ConstNodeIterator &base): Super(base) {}
    };
#endif
        
    class VertexValueIterator: public VertexBaseIterator<VertexValueIterator, VertexValue,
                                                         typename VertexList::NodeIterator> {
        typedef                       VertexBaseIterator<VertexValueIterator, VertexValue,
                                                         typename VertexList::NodeIterator> Super;
    public:
        VertexValueIterator() {}
        VertexValueIterator(const VertexValueIterator &other): Super(other) {}
        VertexValueIterator(const VertexNodeIterator &other): Super(other.base_) {}
        VertexValue& operator*() const { return this->dereference().value(); }
        VertexValue* operator->() const { return &this->dereference().value(); }
    private:
        friend class Graph;
        VertexValueIterator(const typename VertexList::NodeIterator &base): Super(base) {}
    };

#if 0 // [Robb Matzke 2014-04-21]: not needed yet
    class ConstVertexValueIterator: public VertexBaseIterator<ConstVertexValueIterator, const VertexValue,
                                                              typename VertexList::ConstNodeIterator> {
        typedef                            VertexBaseIterator<ConstVertexValueIterator, const VertexValue,
                                                              typename VertexList::ConstNodeIterator> Super;
    public:
        ConstVertexValueIterator() {}
        ConstVertexValueIterator(const ConstVertexValueIterator &other): Super(other) {}
        const VertexValue& operator*() const { return this->dereference().value(); }
        const VertexValue* operator->() const { return &this->dereference().value(); }
    private:
        friend class Graph;
        ConstVertexValueIterator(const typename VertexList::ConstNodeIterator &base): Super(base) {}
    };
#endif


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                                  Storage nodes
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:

    // All information associated with an edge
    class Edge {
        VirtualList<Edge> edgeLists_;                   // links for in- and out-edge sublists; MUST BE FIRST
        EdgeValue value_;                               // user-defined data for each edge
        typename EdgeList::NodeIterator self_;          // always points to itself so we can get to IndexedList::Node
        VertexNodeIterator source_, target_;            // starting and ending points of the edge are always required
    private:
        friend class Graph;
        Edge(const EdgeValue &value, const VertexNodeIterator &source, const VertexNodeIterator &target)
            : value_(value), source_(source), target_(target) {}
    public:
        const size_t& id() const { return self_->id(); }
        const VertexNodeIterator& source() { return source_; }
        const VertexNodeIterator& target() { return target_; }
        EdgeValue& value() { return value_; }
        const EdgeValue& value() const { return value_; }
    };

    // All information associated with a vertex
    class Vertex {
        VertexValue value_;                             // user data for this vertex
        typename VertexList::NodeIterator self_;        // always points to itself so we can get to IndexedList::Node
        VirtualList<Edge> edgeLists_;                   // this is the head node; points to the real edges
    private:
        friend class Graph;
        Vertex(const VertexValue &value): value_(value) {}
    public:
        const size_t& id() const { return self_->id(); }
        boost::iterator_range<EdgeNodeIterator> inEdges() {
            EdgeNodeIterator begin(IN_EDGES, &edgeLists_.next(IN_EDGES));
            EdgeNodeIterator end(IN_EDGES, &edgeLists_);
            return boost::iterator_range<EdgeNodeIterator>(begin, end);
        }
        boost::iterator_range<EdgeNodeIterator> outEdges() {
            EdgeNodeIterator begin(OUT_EDGES, &edgeLists_.next(OUT_EDGES));
            EdgeNodeIterator end(OUT_EDGES, &edgeLists_);
            return boost::iterator_range<EdgeNodeIterator>(begin, end);
        }
        VertexValue& value() { return value_; }
        const VertexValue& value() const { return value_; }
    };

    EdgeList edges_;                                    // all edges with integer ID numbers and O(1) insert/erase
    VertexList vertices_;                               // all vertices with integer ID numbers and O(1) insert/erase


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                                  Initialization
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:

    /** Default constructor.
     *
     *  Creates an empty graph. */
    Graph() {};

    /** Copy constructor.
     *
     *  Initializes this graph by copying all node and edge data from the @p other graph and initializing the same vertex
     *  connectivity.   Vertices and edges in this new graph are likely to have different ID numbers than in the @p other
     *  graph. */
    Graph(const Graph &other);

    /** Copy constructor.
     *
     *  Initializes this graph by copying all node and edge data from teh @p other graph and initializing the same vertex
     *  connectivity.  The vertices and edges of @p other must be convertible to the types of vertices and edges in this
     *  graph.  Vertices and edges in this new graph are likely to have different ID numbers than in the @p other graph. */
    template<class V2, class E2>
    Graph(const Graph<V2, E2> &other);

    /** Assignment.
     *
     *  Causes this graph to look like @p other in that this graph will have copies of all the @p other vertex and edge data
     *  and the same vertex connectivity as @p other.  However, the new copies of vertices and edges in this graph will likely
     *  have different ID numbers than in @p other. */
    Graph& operator=(const Graph &other);

    /** Assignment.
     *
     *  Causes this graph to look like @p other in that this graph will have copies of all the @p other vertex and edge data
     *  and the same vertex connectivity as @p other.  The vertices and edgse of @p other must be convertible to the types of
     *  vertices and edges in this graph. The new copies of vertices and edges in this graph will likely have different ID
     *  numbers than in @p other. */
    template<class V2, class E2>
    Graph& operator=(const Graph<V2, E2> &other);



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:

    /** Iterators for all vertices. */
    boost::iterator_range<VertexNodeIterator> vertices() {
        return boost::iterator_range<VertexNodeIterator>(VertexNodeIterator(vertices_.nodes().begin()),
                                                         VertexNodeIterator(vertices_.nodes().end()));
    }

    boost::iterator_range<VertexValueIterator> vertexValues() {
        return boost::iterator_range<VertexValueIterator>(VertexValueIterator(vertices_.nodes().begin()),
                                                          VertexValueIterator(vertices_.nodes().end()));
    }
    
    VertexNodeIterator findVertex(size_t id) {
        return VertexNodeIterator(vertices_.find(id));
    }

    /** Iterators for all edges. */
    boost::iterator_range<EdgeNodeIterator> edges() {
        return boost::iterator_range<EdgeNodeIterator>(EdgeNodeIterator(edges_.nodes().begin()),
                                                       EdgeNodeIterator(edges_.nodes().end()));
    }

    boost::iterator_range<EdgeValueIterator> edgeValues() {
        return boost::iterator_range<EdgeValueIterator>(EdgeValueIterator(edges_.nodes().begin()),
                                                        EdgeValueIterator(edges_.nodes().end()));
    }

    EdgeNodeIterator findEdge(size_t id) {
        return EdgeNodeIterator(edges_.find(id));
    }

    /** Total number of vertices. */
    size_t nVertices() const {
        return vertices_.size();
    }

    /** Total number of edges. */
    size_t nEdges() const {
        return edges_.size();
    }

    /** True if graph is empty. */
    bool isEmpty() const {
        ASSERT_require(edges_.isEmpty() || !vertices_.isEmpty()); // existence of edges implies existence of vertices
        return vertices_.isEmpty();
    }

    /** Insert a new vertex. */
    VertexNodeIterator insert(const VertexValue &value) {
        typename VertexList::NodeIterator inserted = vertices_.insert(vertices_.nodes().end(), Vertex(value));
        inserted->value().self_ = inserted;
        inserted->value().edgeLists_.reset(NULL);       // this is a sublist head, no edge node
        return VertexNodeIterator(inserted);
    }

    /** Insert a new edge */
    EdgeNodeIterator insert(const VertexNodeIterator &sourceVertex, const VertexNodeIterator &targetVertex,
                            const EdgeValue &value) {
        ASSERT_forbid(sourceVertex==vertices().end());
        ASSERT_forbid(targetVertex==vertices().end());
        typename EdgeList::NodeIterator inserted = edges_.insert(edges_.nodes().end(), Edge(value, sourceVertex, targetVertex));
        inserted->value().self_ = inserted;
        inserted->value().edgeLists_.reset(&inserted->value());
        EdgeNodeIterator newEdge(inserted);
        sourceVertex->edgeLists_.insert(OUT_EDGES, &newEdge->edgeLists_);
        targetVertex->edgeLists_.insert(IN_EDGES, &newEdge->edgeLists_);
        return newEdge;
    }

    /** Remove an edge. */
    EdgeNodeIterator erase(const EdgeNodeIterator &edge) {
        ASSERT_forbid(edge==edges().end());
        EdgeNodeIterator next = edge; ++next;           // advance before we delete edge
        edge->edgeLists_.remove(OUT_EDGES);
        edge->edgeLists_.remove(IN_EDGES);
        edges_.eraseAt(edge->self_);                    // edge is now deleted
        return next;
    }

    /** Remove a vertex and incident edges. */
    VertexNodeIterator erase(const VertexNodeIterator &vertex) {
        VertexNodeIterator next = vertex; ++next;       // advance before we delete vertex
        clearEdges(vertex);
        vertices_.eraseAt(vertex->self_);               // vertex is now deleted
        return next;
    }

    /** Remove all edges, but leave all vertices. */
    void clearEdges() {
        for (VertexNodeIterator vertex=vertices().begin(); vertex!=vertices().end(); ++vertex) {
            vertex->inEdges().reset();
            vertex->outEdges().reset();
        }
        edges_.clear();
    }

    /** Remove all edges connected to this vertex. */
    void clearEdges(const VertexNodeIterator &vertex) {
        clearOutEdges(vertex);
        clearInEdges(vertex);
    }

    /** Remove all outgoing edges from a vertex. */
    void clearOutEdges(const VertexNodeIterator &vertex) {
        ASSERT_forbid(vertex==vertices().end());
        for (EdgeNodeIterator edge=vertex->outEdges().begin(); edge!=vertex->outEdges().end(); /*void*/)
            edge = erase(edge);
    }

    void clearInEdges(const VertexNodeIterator &vertex) {
        ASSERT_forbid(vertex==vertices().end());
        for (EdgeNodeIterator edge=vertex->inEdges().begin(); edge!=vertex->inEdges().end(); /*void*/)
            edge = erase(edge);
    }

    /** Remove all vertices and edges. */
    void clear() {
        edges_.clear();
        vertices_.clear();
    }
};

} // namespace
} // namespace

#endif

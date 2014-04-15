// Boost Graph Library (BGL) interface around Sawyer::Container::Graph
#ifndef Sawyer_GraphBoost_H
#define Sawyer_GraphBoost_H

#include <sawyer/Graph.h>
#include <boost/graph/graph_traits.hpp>

namespace boost {

// BGL distinguishes between iterators and descriptors, but Sawyer treats them as the same thing (pointers to an object). In
// order to support existing BGL-style code like this:
//
//     VertexIterator v, v_end;
//     for (boost::tie(v, v_end)=boost::vertices(graph); v!=v_end; ++v) {
//         VertexDesc v_source = *v;
//         boost::add_edge(v_source, ....)
//     }
//
// we need to insert an extra level of indirection. We create a new iterator type for use by graph_traits<>::vertex_iterator
// that wraps a Sawyer::Container::Graph<>::VertexNodeIterator.  When dereferenced, this iterator returns a size_t ID number
// rather than the base iterator because BGL requires a null_vertex descriptor that's comparable across all graphs.
namespace SawyerBoost {
template<class V, class E>
class VertexOuterIterator: public std::iterator<std::bidirectional_iterator_tag, const size_t> {
private:
    typedef typename Sawyer::Container::Graph<V, E>::VertexNodeIterator BaseIter;
    BaseIter base_;
public:
    VertexOuterIterator() {}
    VertexOuterIterator(const VertexOuterIterator &other): base_(other.base_) {}
    explicit VertexOuterIterator(const BaseIter &base): base_(base) {}
    VertexOuterIterator& operator=(const VertexOuterIterator &other) { base_ = other.base_; return *this; }
    VertexOuterIterator& operator++() { ++base_; return *this; }
    VertexOuterIterator& operator--() { --base_; return *this; }
    VertexOuterIterator operator++(int) { VertexOuterIterator old = *this; ++base_; return old; }
    VertexOuterIterator operator--(int) { VertexOuterIterator old = *this; --base_; return old; }
    bool operator==(const VertexOuterIterator &other) const { return base_ == other.base_; }
    bool operator!=(const VertexOuterIterator &other) const { return base_ != other.base_; }
    const size_t& operator*() const { return base_->id(); }
    // size_t* operator->() const; //no methods defined on size_t, so not needed
};
} // namespace


// Similarly for edges
namespace SawyerBoost {
template<class V, class E>
class EdgeOuterIterator: public std::iterator<std::bidirectional_iterator_tag, const size_t> {
private:
    typedef typename Sawyer::Container::Graph<V, E>::EdgeNodeIterator BaseIter;
    BaseIter base_;
public:
    EdgeOuterIterator() {}
    EdgeOuterIterator(const EdgeOuterIterator &other): base_(other.base_) {}
    explicit EdgeOuterIterator(const BaseIter &base): base_(base) {}
    EdgeOuterIterator& operator=(const EdgeOuterIterator &other) { base_ = other.base_; return *this; }
    EdgeOuterIterator& operator++() { ++base_; return *this; }
    EdgeOuterIterator& operator--() { --base_; return *this; }
    EdgeOuterIterator operator++(int) { EdgeOuterIterator old = *this; ++base_; return old; }
    EdgeOuterIterator operator--(int) { EdgeOuterIterator old = *this; --base_; return old; }
    bool operator==(const EdgeOuterIterator &other) const { return base_ == other.base_; }
    bool operator!=(const EdgeOuterIterator &other) const { return base_ != other.base_; }
    const size_t& operator*() const { return base_->id(); }
    // size_t* operator->() const; //no methods defined on size_t, so not needed
};
} // namespace


template<class V, class E>
struct graph_traits<Sawyer::Container::Graph<V, E> > {
    typedef bidirectional_graph_tag traversal_category;

    // Graph concepts
    typedef size_t vertex_descriptor;
    typedef size_t edge_descriptor;
    typedef directed_tag directed_category;
    typedef allow_parallel_edge_tag edge_parallel_category;

    // VertexListGraph concepts
    typedef ::boost::SawyerBoost::VertexOuterIterator<V, E> vertex_iterator;
    typedef size_t vertices_size_type;

    // EdgeListGraph concepts
    typedef ::boost::SawyerBoost::EdgeOuterIterator<V, E> edge_iterator;
    typedef size_t edges_size_type;

    // IncidenceGraph concepts
    typedef ::boost::SawyerBoost::EdgeOuterIterator<V, E> out_edge_iterator;
    typedef size_t degree_size_type;

    // BidirectionalGraph concepts
    typedef ::boost::SawyerBoost::EdgeOuterIterator<V, E> in_edge_iterator;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Graph traits
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// BGL has a global entity that indicates no-vertex, but Sawyer doesn't--it has STL-like end() iterators.
template<class V, class E>
typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor
null_vertex() {
    return (size_t)(-1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      VertexListGraph concepts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class V, class E>
std::pair<typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_iterator,
          typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_iterator>
vertices(Sawyer::Container::Graph<V, E> &graph) {
    return std::make_pair(::boost::SawyerBoost::VertexOuterIterator<V, E>(graph.vertices().begin()),
                          ::boost::SawyerBoost::VertexOuterIterator<V, E>(graph.vertices().end()));
}

template<class V, class E>
typename graph_traits<Sawyer::Container::Graph<V, E> >::vertices_size_type
num_vertices(Sawyer::Container::Graph<V, E> &graph) {
    return graph.nVertices();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      EdgeListGraph concepts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class V, class E>
std::pair<typename graph_traits<Sawyer::Container::Graph<V, E> >::edge_iterator,
          typename graph_traits<Sawyer::Container::Graph<V, E> >::edge_iterator>
edges(Sawyer::Container::Graph<V, E> &graph) {
    return std::make_pair(::boost::SawyerBoost::EdgeOuterIterator<V, E>(graph.edges().begin()),
                          ::boost::SawyerBoost::EdgeOuterIterator<V, E>(graph.edges().end()));
}

template<class V, class E>
typename graph_traits<Sawyer::Container::Graph<V, E> >::edges_size_type
num_edges(Sawyer::Container::Graph<V, E> &graph) {
    return graph.nEdges();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      IncidenceGraph concepts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class V, class E>
typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor
source(typename graph_traits<Sawyer::Container::Graph<V, E> >::edge_descriptor edge,
       Sawyer::Container::Graph<V, E> &graph) {
    return graph.findEdge(edge)->source()->id();
}

template<class V, class E>
typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor
target(typename graph_traits<Sawyer::Container::Graph<V, E> >::edge_descriptor edge,
       Sawyer::Container::Graph<V, E> &graph) {
    return graph.findEdge(edge)->target()->id();
}

template<class V, class E>
std::pair<typename graph_traits<Sawyer::Container::Graph<V, E> >::out_edge_iterator,
          typename graph_traits<Sawyer::Container::Graph<V, E> >::out_edge_iterator>
out_edges(typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor &vertex,
          Sawyer::Container::Graph<V, E> &graph) {
    typename Sawyer::Container::Graph<V, E>::VertexNodeIterator v = graph.findVertex(vertex);
    return std::make_pair(::boost::SawyerBoost::EdgeOuterIterator<V, E>(v->outEdges().begin()),
                          ::boost::SawyerBoost::EdgeOuterIterator<V, E>(v->outEdges().end()));
}

template<class V, class E>
typename graph_traits<Sawyer::Container::Graph<V, E> >::degree_size_type
out_degree(typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor &vertex,
          Sawyer::Container::Graph<V, E> &graph) {
    return graph.findVertex(vertex)->nOutEdges();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      BidirectionalGraph concepts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class V, class E>
std::pair<typename graph_traits<Sawyer::Container::Graph<V, E> >::in_edge_iterator,
          typename graph_traits<Sawyer::Container::Graph<V, E> >::in_edge_iterator>
in_edges(typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor &vertex,
         Sawyer::Container::Graph<V, E> &graph) {
    typename Sawyer::Container::Graph<V, E>::VertexNodeIterator v = graph.findVertex(vertex);
    return std::make_pair(::boost::SawyerBoost::EdgeOuterIterator<V, E>(v->inEdges().begin()),
                          ::boost::SawyerBoost::EdgeOuterIterator<V, E>(v->inEdges().end()));
}

template<class V, class E>
typename graph_traits<Sawyer::Container::Graph<V, E> >::degree_size_type
in_degree(typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor &vertex,
          Sawyer::Container::Graph<V, E> &graph) {
    return graph.findVertex(vertex)->nInEdges();
}

template<class V, class E>
typename graph_traits<Sawyer::Container::Graph<V, E> >::degree_size_type
degree(typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor &vertex,
          Sawyer::Container::Graph<V, E> &graph) {
    return in_degree(vertex) + out_degree(vertex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      MutableGraph concepts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class V, class E>
std::pair<typename graph_traits<Sawyer::Container::Graph<V, E> >::edge_descriptor, bool>
add_edge(typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor source,
         typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor target,
         Sawyer::Container::Graph<V, E> &graph) {
    typename Sawyer::Container::Graph<V, E>::VertexNodeIterator src=graph.findVertex(source), tgt=graph.findVertex(target);
    typename Sawyer::Container::Graph<V, E>::EdgeNodeIterator newEdge = graph.insert(src, tgt, E());
    return std::make_pair(newEdge->id(), true);
}

template<class V, class E>
void
remove_edge(typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor source,
            typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor target,
            Sawyer::Container::Graph<V, E> &graph) {
    typename Sawyer::Container::Graph<V, E>::VertexNodeIterator src=graph.findVertex(source), tgt=graph.findVertex(target);
    typename Sawyer::Container::Graph<V, E>::EdgeNodeIterator edge = src->outEdges().begin();
    while (edge != src->outEdges().end()) {
        if (edge->target() == tgt) {
            edge = graph.erase(edge);
        } else {
            ++edge;
        }
    }
}

template<class V, class E>
void
remove_edge(typename graph_traits<Sawyer::Container::Graph<V, E> >::edge_descriptor edge,
            Sawyer::Container::Graph<V, E> &graph) {
    graph.erase(graph.findEdge(edge));
}

template<class Predicate, class V, class E>
void
remove_edge_if(Predicate predicate,
               Sawyer::Container::Graph<V, E> &graph) {
    typename Sawyer::Container::Graph<V, E>::EdgeNodeIterator edge = graph.edges().begin();
    while (edge != graph.edges().end()) {
        if (predicate(edge->id())) {
            edge = graph.erase(edge);
        } else {
            ++edge;
        }
    }
}

template<class Predicate, class V, class E>
void
remove_out_edge_if(typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor vertex,
                   Predicate predicate,
                   Sawyer::Container::Graph<V, E> &graph) {
    typename Sawyer::Container::Graph<V, E>::VertexNodeIterator v = graph.findVertex(vertex);
    typename Sawyer::Container::Graph<V, E>::EdgeNodeIterator edge = v->outEdges().begin();
    while (edge != v->outEdges().end()) {
        if (predicate(edge->id())) {
            edge = graph.erase(edge);
        } else {
            ++edge;
        }
    }
}

template<class Predicate, class V, class E>
void
remove_in_edge_if(typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor vertex,
                  Predicate predicate,
                  Sawyer::Container::Graph<V, E> &graph) {
    typename Sawyer::Container::Graph<V, E>::VertexNodeIterator v = graph.findVertex(vertex);
    typename Sawyer::Container::Graph<V, E>::EdgeNodeIterator edge = v->inEdges().begin();
    while (edge != v->inEdges().end()) {
        if (predicate(edge->id())) {
            edge = graph.erase(edge);
        } else {
            ++edge;
        }
    }
}

template<class V, class E>
typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor
add_vertex(Sawyer::Container::Graph<V, E> &graph) {
    return graph.insert(V())->id();
}

template<class V, class E>
void
clear_vertex(typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor vertex,
             Sawyer::Container::Graph<V, E> &graph) {
    graph.clearEdges(graph.findVertex(vertex));
}

template<class V, class E>
void
remove_vertex(typename graph_traits<Sawyer::Container::Graph<V, E> >::vertex_descriptor vertex,
              Sawyer::Container::Graph<V, E> &graph) {
    graph.erase(graph.findVertex(vertex));
}
    
} // namespace
#endif

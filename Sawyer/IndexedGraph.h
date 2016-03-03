#ifndef Sawyer_IndexedGraph_H
#define Sawyer_IndexedGraph_H

#include <Sawyer/BiMap.h>
#include <Sawyer/Exception.h>
#include <Sawyer/Graph.h>

namespace Sawyer {
namespace Container {

/** Vertex key type for graphs that do not index their vertices. */
template<class Vertex>
class GraphVertexNoKey {
public:
    typedef void Key;

    GraphVertexNoKey() {}
    GraphVertexNoKey(const GraphVertexNoKey&) /*default*/;
    GraphVertexNoKey(const Vertex&) {}
};

template<class Edge>
class GraphEdgeNoKey {};


template<class V = Nothing, class E = Nothing,
         class VKey = GraphVertexNoKey<V>,
         class EKey = GraphEdgeNoKey<E>,
         class A = DefaultAllocator>
class IndexedGraph: public Graph<V, E, A> {
public:
    typedef Graph<V, E, A> Super;
    typedef VKey VertexKey;
    typedef EKey EdgeKey;

private:
    typedef BiMap<VertexKey, typename Super::ConstVertexIterator> VertexIndex;
    VertexIndex vertexIndex_;

public:
    IndexedGraph() {};

    template<class V2, class E2, class A2>
    IndexedGraph(const Graph<V2, E2, A2> &graph, const typename Super::Allocator &allocator = typename Super::Allocator())
        : Super(graph) {
        initVertexIndex();
    }

    typename Super::VertexIterator
    insertVertex(const typename Super::VertexValue &value = typename Super::VertexValue()) /*override*/ {
        if (vertexIndex_.forward().exists(VertexKey(value)))
            throw Exception::AlreadyExists("cannot insert duplicate vertex when graph vertices are indexed");
        typename Super::VertexIterator retval = Super::insertVertex(value);
        vertexIndex_.insert(VertexKey(value), retval);
        return retval;
    }

    typename Super::VertexIterator
    insertVertexMaybe(const typename Super::VertexValue &value = typename Super::VertexValue()) {
        typename Super::VertexIterator found = findVertexValue(value);
        return this->isValidVertex(found) ? found : insertVertex(value);
    }

    typename Super::EdgeIterator
    insertEdgeAndMaybeVertices(const typename Super::VertexValue &sourceValue, const typename Super::VertexValue &targetValue,
                               const typename Super::EdgeValue &edgeValue = typename Super::EdgeValue()) {
        typename Super::VertexIterator source = insertVertexMaybe(sourceValue);
        typename Super::VertexIterator target = insertVertexMaybe(targetValue);
        return insertEdge(source, target, edgeValue);
    }
    
    typename Super::VertexIterator
    eraseVertex(const typename Super::VertexIterator &vertex) /*override*/ {
        vertexIndex_.eraseTarget(vertex);
        return Super::eraseVertex(vertex);
    }

    typename Super::VertexIterator
    eraseVertex(const typename Super::ConstVertexIterator &vertex) /*override*/ {
        ASSERT_require(isValidVertex(vertex));
        return eraseVertex(findVertex(vertex->id()));
    }

    typename Super::EdgeIterator
    eraseEdgeAndMabyeVertices(const typename Super::EdgeIterator &edge) {
        ASSERT_require(isValidEdge(edge));
        typename Super::VertexIterator source = edge->source();
        typename Super::VertexIterator target = edge->target();
        typename Super::EdgeIterator retval = Super::eraseEdge(edge);
        if (source == target) {
            if (source->degree() == 0)
                eraseVertex(source);
        } else {
            if (source->degree() == 0)
                eraseVertex(source);
            if (target->degree() == 0)
                eraseVertex(target);
        }
        return retval;
    }

    typename Super::EdgeIterator
    eraseEdgeAndMabyeVertices(const typename Super::ConstEdgeIterator &edge) {
        ASSERT_require(isValidEdge(edge));
        return eraseEdgeAndMabyeVertices(findEdge(edge->id()));
    }
    
    typename Super::VertexIterator
    findVertexValue(const typename Super::VertexValue &value) {
        typename Super::ConstVertexIterator vi = ((const IndexedGraph*)this)->findVertexValue(value);
        return this->isValidVertex(vi) ? findVertex(vi->id()) : this->vertices().end();
    }

    typename Super::ConstVertexIterator
    findVertexValue(const typename Super::VertexValue &value) const {
        return vertexIndex_.forward().getOrElse(VertexKey(value), this->vertices().end());
    }
    
    void clear() /*override*/ {
        Super::clear();
        vertexIndex_.clear();
    }
    
private:
    void initVertexIndex() {
        vertexIndex_.clear();
        for (typename Super::ConstVertexIterator vi = this->vertices().begin(); vi != this->vertices().end(); ++vi)
            vertexIndex_.insert(VKey(vi.value()), vi);
    }
};

} // namespace
} // namespace

#endif

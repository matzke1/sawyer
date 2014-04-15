#include <sawyer/Graph.h>
#include <sawyer/Assert.h>
#include <boost/foreach.hpp>
#include <string>
#include <vector>

template<class V, class E>
std::ostream& operator<<(std::ostream &o, Sawyer::Container::Graph<V, E> &graph) {
    typedef const typename Sawyer::Container::Graph<V, E> Graph;
    typedef typename Graph::VertexNodeIterator VertexNodeIterator;
    typedef typename Graph::EdgeNodeIterator EdgeNodeIterator;
    typedef typename Graph::Vertex Vertex;
    typedef typename Graph::Edge Edge;

    o <<"    vertices:\n";
    for (size_t id=0; id<graph.nVertices(); ++id) {
        VertexNodeIterator vertex = graph.findVertex(id);
        o <<"      [" <<vertex->id() <<"] = " <<vertex->value() <<"\n";
        BOOST_FOREACH (Edge &edge, vertex->outEdges())
            o <<"        out edge #" <<edge.id() <<" to   node #" <<edge.target()->id() <<" = " <<edge.value() <<"\n";
        BOOST_FOREACH (Edge &edge, vertex->inEdges())
            o <<"        in  edge #" <<edge.id() <<" from node #" <<edge.source()->id() <<" = " <<edge.value() <<"\n";
    }

    o <<"    edges:\n";
    for (size_t id=0; id<graph.nEdges(); ++id) {
        EdgeNodeIterator edge = graph.findEdge(id);
        o <<"      [" <<edge->id() <<"] = " <<edge->value() <<"\n";
        o <<"        from vertex [" <<edge->source()->id() <<"] = " <<edge->source()->value() <<"\n";
        o <<"        to   vertex [" <<edge->target()->id() <<"] = " <<edge->target()->value() <<"\n";
    }
    return o;
}

template<class Graph>
void default_ctor() {
    std::cout <<"default constructor:\n";
    Graph g1;
    std::cout <<g1;
}

template<class Graph>
void insert_vertex() {
    std::cout <<"vertex insertion\n";
    Graph graph;
    ASSERT_require(graph.nVertices()==0);

    typename Graph::VertexNodeIterator iter = graph.insert("banana");
    std::cout <<"  inserted [" <<iter->id() <<"] = " <<iter->value() <<"\n";
    ASSERT_require(graph.nVertices()==1);
    ASSERT_require(iter->id()==0);
    ASSERT_require(iter->value()=="banana");

    iter = graph.insert("orange");
    std::cout <<"  inserted [" <<iter->id() <<"] = " <<iter->value() <<"\n";
    ASSERT_require(graph.nVertices()==2);
    ASSERT_require(iter->id()==1);
    ASSERT_require(iter->value()=="orange");

    iter = graph.insert("pineapple");
    std::cout <<"  inserted [" <<iter->id() <<"] = " <<iter->value() <<"\n";
    ASSERT_require(graph.nVertices()==3);
    ASSERT_require(iter->id()==2);
    ASSERT_require(iter->value()=="pineapple");

    std::cout <<graph;
}

template<class Graph>
void erase_empty_vertex() {
    std::cout <<"emtpy vertex erasure:\n";
    typedef typename Graph::VertexNodeIterator Vertex;

    Graph graph;
    Vertex v0 = graph.insert("banana");
    Vertex v1 = graph.insert("orange");
    Vertex v2 = graph.insert("pineapple");
    std::cout <<"  initial graph:\n" <<graph;

    graph.erase(v1);
    ASSERT_require(graph.nVertices()==2);

    graph.erase(v0);
    ASSERT_require(graph.nVertices()==1);

    graph.erase(v2);
    ASSERT_require(graph.nVertices()==0);
    ASSERT_require(graph.isEmpty());
}

template<class Graph>
void iterate_vertices() {
    std::cout <<"vertex iteration:\n";

    Graph graph;
    std::vector<std::string> vertexValues;
    vertexValues.push_back("gold");
    vertexValues.push_back("glitter");
    vertexValues.push_back("goose");
    vertexValues.push_back("grinch");
    for (size_t i=0; i<vertexValues.size(); ++i)
        graph.insert(vertexValues[i]);

    std::cout <<"  using BOOST_FOREACH:";
    size_t idx = 0;
    BOOST_FOREACH (const typename Graph::Vertex &vertex, graph.vertices()) {
        std::cout <<" " <<vertex.value();
        ASSERT_require(vertex.value()== vertexValues[idx]);
        ++idx;
    }
    std::cout <<"\n";

    std::cout <<"  using begin/end:    ";
    idx = 0;
    for (typename Graph::VertexNodeIterator iter=graph.vertices().begin(); iter!=graph.vertices().end(); ++iter) {
        std::cout <<" " <<iter->value();
        ASSERT_require(iter->value() == vertexValues[idx]);
        ++idx;
    }
    std::cout <<"\n";
}

template<class Graph>
void find_vertex() {
    std::cout <<"find vertex by ID:\n";
    typedef typename Graph::VertexNodeIterator VertexDesc;
    
    Graph graph;
    std::vector<std::string> vertexValues;
    VertexDesc v0 = graph.insert("vine");
    VertexDesc v1 = graph.insert("vinegar");
    VertexDesc v2 = graph.insert("violin");
    VertexDesc v3 = graph.insert("visa");
    std::cout <<"  initial graph:\n" <<graph;

    ASSERT_require(v0 == graph.findVertex(v0->id()));
    ASSERT_require(v1 == graph.findVertex(v1->id()));
    ASSERT_require(v2 == graph.findVertex(v2->id()));
    ASSERT_require(v3 == graph.findVertex(v3->id()));
}

template<class Graph>
void insert_edge() {
    std::cout <<"edge insertion\n";
    typedef typename Graph::VertexNodeIterator VertexDescriptor;
    typedef typename Graph::EdgeNodeIterator EdgeDescriptor;

    Graph graph;
    std::vector<std::string> vertexValues;
    VertexDescriptor v0 = graph.insert("vine");
    VertexDescriptor v1 = graph.insert("vinegar");
    VertexDescriptor v2 = graph.insert("violin");
    VertexDescriptor v3 = graph.insert("visa");

    EdgeDescriptor e0 = graph.insert(v0, v1, "vine-vinegar");
    EdgeDescriptor e1 = graph.insert(v2, v1, "violin-vinegar");
    EdgeDescriptor e2 = graph.insert(v0, v3, "vine-visa");
    EdgeDescriptor e3 = graph.insert(v3, v0, "visa-vine");
    EdgeDescriptor e4 = graph.insert(v3, v3, "visa-visa");
    ASSERT_require(graph.nEdges() == 5);

    ASSERT_require(e0->value() == "vine-vinegar");
    ASSERT_require(e0->source() == v0);
    ASSERT_require(e0->target() == v1);

    ASSERT_require(e1->value() == "violin-vinegar");
    ASSERT_require(e1->source() == v2);
    ASSERT_require(e1->target() == v1);
    
    ASSERT_require(e2->value() == "vine-visa");
    ASSERT_require(e2->source() == v0);
    ASSERT_require(e2->target() == v3);
    
    ASSERT_require(e3->value() == "visa-vine");
    ASSERT_require(e3->source() == v3);
    ASSERT_require(e3->target() == v0);
    
    ASSERT_require(e4->value() == "visa-visa");
    ASSERT_require(e4->source() == v3);
    ASSERT_require(e4->target() == v3);
    
    std::cout <<graph;
}

template<class Graph>
void erase_edge() {
    std::cout <<"edge erasure:\n";
    typedef typename Graph::VertexNodeIterator Vertex;
    typedef typename Graph::EdgeNodeIterator Edge;
    
    Graph graph;
    std::vector<std::string> vertexValues;
    Vertex v0 = graph.insert("vine");
    Vertex v1 = graph.insert("vinegar");
    Vertex v2 = graph.insert("violin");
    Vertex v3 = graph.insert("visa");
    Edge e0 = graph.insert(v0, v1, "vine-vinegar");
    Edge e1 = graph.insert(v2, v1, "violin-vinegar");
    Edge e2 = graph.insert(v0, v3, "vine-visa");
    Edge e3 = graph.insert(v3, v0, "visa-vine");
    Edge e4 = graph.insert(v3, v3, "visa-visa");
    std::cout <<"  initial graph:\n" <<graph;

    graph.erase(e3);
    std::cout <<"  after erasing 'visa-vine' edge:\n" <<graph;
    ASSERT_require(graph.nEdges() == 4);

    graph.erase(e1);
    std::cout <<"  after erasing 'violin-vinegar' edge:\n" <<graph;
    ASSERT_require(graph.nEdges() == 3);

    graph.erase(e0);
    std::cout <<"  after erasing 'vine-vinegar' edge:\n" <<graph;
    ASSERT_require(graph.nEdges() == 2);

    graph.erase(e4);
    std::cout <<"  after erasing 'visa-visa' edge:\n" <<graph;
    ASSERT_require(graph.nEdges() == 1);

    graph.erase(e2);
    std::cout <<"  after erasing 'vine-visa' edge:\n" <<graph;
    ASSERT_require(graph.nEdges() == 0);

    ASSERT_forbid(graph.isEmpty());                     // still has vertices
}

template<class Graph>
void erase_vertex() {
    std::cout <<"erase vertices with edges:\n";
    typedef typename Graph::VertexNodeIterator Vertex;
    typedef typename Graph::EdgeNodeIterator Edge;
    
    Graph graph;
    std::vector<std::string> vertexValues;
    Vertex v0 = graph.insert("vine");
    Vertex v1 = graph.insert("vinegar");
    Vertex v2 = graph.insert("violin");
    Vertex v3 = graph.insert("visa");
    Edge e0 = graph.insert(v0, v1, "vine-vinegar");
    Edge e1 = graph.insert(v2, v1, "violin-vinegar");
    Edge e2 = graph.insert(v0, v3, "vine-visa");
    Edge e3 = graph.insert(v3, v0, "visa-vine");
    Edge e4 = graph.insert(v3, v3, "visa-visa");
    std::cout <<"  initial graph:\n" <<graph;

    graph.erase(v2);
    ASSERT_require(graph.nVertices() == 3);
    ASSERT_require(graph.nEdges() == 4);

    graph.erase(v0);
    ASSERT_require(graph.nVertices() == 2);
    ASSERT_require(graph.nEdges() == 1);

    graph.erase(v3);
    ASSERT_require(graph.nVertices() == 1);
    ASSERT_require(graph.nEdges() == 0);

    graph.erase(v1);
    ASSERT_require(graph.nVertices() == 0);
    ASSERT_require(graph.nEdges() == 0);
    ASSERT_require(graph.isEmpty());
}

template<class Graph>
void iterator_conversion() {
    std::cout <<"iterator implicit conversions:\n";
    typedef typename Graph::VertexNodeIterator Vertex;
    typedef typename Graph::EdgeNodeIterator Edge;
    
    Graph graph;
    std::vector<std::string> vertexValues;
    Vertex v0 = graph.insert("vine");
    Vertex v1 = graph.insert("vinegar");
    Edge e0 = graph.insert(v0, v1, "vine-vinegar");
    std::cout <<"  initial graph:\n" <<graph;

    typename Graph::VertexValueIterator vval = v0;
    ASSERT_require(*vval == "vine");

    typename Graph::EdgeValueIterator eval = e0;
    ASSERT_require(*eval == "vine-vinegar");

#if 0 // [Robb Matzke 2014-04-21]: going the other way is not indended to work (compile error)
    typename Graph::EdgeNodeIterator e0fail = eval;
#endif
    
}

int main() {
    typedef Sawyer::Container::Graph<std::string, std::string> G1;
    default_ctor<G1>();
    insert_vertex<G1>();
    iterate_vertices<G1>();
    find_vertex<G1>();
    erase_empty_vertex<G1>();
    insert_edge<G1>();
    erase_edge<G1>();
    erase_vertex<G1>();
    iterator_conversion<G1>();
}

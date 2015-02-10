#include <sawyer/Graph.h>
#include <sawyer/GraphTraversal.h>
#include <sawyer/Assert.h>
#include <boost/foreach.hpp>
#include <iostream>
#include <string>
#include <vector>

template<class V, class E>
std::ostream& operator<<(std::ostream &o, const Sawyer::Container::Graph<V, E> &graph) {
    typedef const typename Sawyer::Container::Graph<V, E> Graph;
    typedef typename Graph::ConstVertexNodeIterator VertexNodeIterator;
    typedef typename Graph::ConstEdgeNodeIterator EdgeNodeIterator;
    typedef typename Graph::VertexNode Vertex;
    typedef typename Graph::EdgeNode Edge;

    o <<"    vertices:\n";
    for (size_t id=0; id<graph.nVertices(); ++id) {
        VertexNodeIterator vertex = graph.findVertex(id);
        o <<"      [" <<vertex->id() <<"] = " <<vertex->value() <<"\n";
        BOOST_FOREACH (const Edge &edge, vertex->outEdges())
            o <<"        out edge #" <<edge.id() <<" to   node #" <<edge.target()->id() <<" = " <<edge.value() <<"\n";
        BOOST_FOREACH (const Edge &edge, vertex->inEdges())
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
    ASSERT_always_require(graph.nVertices()==0);

    typename Graph::VertexNodeIterator v0 = graph.insertVertex("banana");
    std::cout <<"  inserted [" <<v0->id() <<"] = " <<v0->value() <<"\n";
    ASSERT_always_require(graph.nVertices()==1);
    ASSERT_always_require(v0->id()==0);
    ASSERT_always_require(v0->value()=="banana");
    ASSERT_always_require(v0 == v0);
    ASSERT_always_forbid(v0 < v0);

    typename Graph::VertexNodeIterator v1 = graph.insertVertex("orange");
    std::cout <<"  inserted [" <<v1->id() <<"] = " <<v1->value() <<"\n";
    ASSERT_always_require(graph.nVertices()==2);
    ASSERT_always_require(v1->id()==1);
    ASSERT_always_require(v1->value()=="orange");
    ASSERT_always_require(v1 != v0);
    ASSERT_always_require(v1 < v0 || v0 < v1);

    typename Graph::VertexNodeIterator v2 = graph.insertVertex("pineapple");
    std::cout <<"  inserted [" <<v2->id() <<"] = " <<v2->value() <<"\n";
    ASSERT_always_require(graph.nVertices()==3);
    ASSERT_always_require(v2->id()==2);
    ASSERT_always_require(v2->value()=="pineapple");
    ASSERT_always_require(v2 != v1);
    ASSERT_always_require(v2 < v1 || v1 < v2);

    std::cout <<graph;
}

template<class Graph>
void erase_empty_vertex() {
    std::cout <<"emtpy vertex erasure:\n";
    typedef typename Graph::VertexNodeIterator Vertex;

    Graph graph;
    Vertex v0 = graph.insertVertex("banana");
    Vertex v1 = graph.insertVertex("orange");
    Vertex v2 = graph.insertVertex("pineapple");
    std::cout <<"  initial graph:\n" <<graph;

    graph.eraseVertex(v1);
    ASSERT_always_require(graph.nVertices()==2);

    graph.eraseVertex(v0);
    ASSERT_always_require(graph.nVertices()==1);

    graph.eraseVertex(v2);
    ASSERT_always_require(graph.nVertices()==0);
    ASSERT_always_require(graph.isEmpty());
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
        graph.insertVertex(vertexValues[i]);

    std::cout <<"  using BOOST_FOREACH:";
    size_t idx = 0;
    BOOST_FOREACH (const typename Graph::VertexNode &vertex, graph.vertices()) {
        std::cout <<" " <<vertex.value();
        ASSERT_always_require(vertex.value()== vertexValues[idx]);
        ++idx;
    }
    std::cout <<"\n";

    std::cout <<"  using begin/end:    ";
    idx = 0;
    for (typename Graph::VertexNodeIterator iter=graph.vertices().begin(); iter!=graph.vertices().end(); ++iter) {
        std::cout <<" " <<iter->value();
        ASSERT_always_require(iter->value() == vertexValues[idx]);
        ++idx;
    }
    std::cout <<"\n";
}

template<class Graph>
void find_vertex() {
    std::cout <<"find vertex by ID:\n";
    typedef typename Graph::VertexNodeIterator VertexDesc;
    
    Graph graph;
    VertexDesc v0 = graph.insertVertex("vine");
    VertexDesc v1 = graph.insertVertex("vinegar");
    VertexDesc v2 = graph.insertVertex("violin");
    VertexDesc v3 = graph.insertVertex("visa");
    std::cout <<"  initial graph:\n" <<graph;

    ASSERT_always_require(v0 == graph.findVertex(v0->id()));
    ASSERT_always_require(v1 == graph.findVertex(v1->id()));
    ASSERT_always_require(v2 == graph.findVertex(v2->id()));
    ASSERT_always_require(v3 == graph.findVertex(v3->id()));
}

template<class Graph>
void insert_edge() {
    std::cout <<"edge insertion\n";
    typedef typename Graph::VertexNodeIterator VertexDescriptor;
    typedef typename Graph::EdgeNodeIterator EdgeDescriptor;

    Graph graph;
    VertexDescriptor v0 = graph.insertVertex("vine");
    VertexDescriptor v1 = graph.insertVertex("vinegar");
    VertexDescriptor v2 = graph.insertVertex("violin");
    VertexDescriptor v3 = graph.insertVertex("visa");

    EdgeDescriptor e0 = graph.insertEdge(v0, v1, "vine-vinegar");
    EdgeDescriptor e1 = graph.insertEdge(v2, v1, "violin-vinegar");
    EdgeDescriptor e2 = graph.insertEdge(v0, v3, "vine-visa");
    EdgeDescriptor e3 = graph.insertEdge(v3, v0, "visa-vine");
    EdgeDescriptor e4 = graph.insertEdge(v3, v3, "visa-visa");
    ASSERT_always_require(graph.nEdges() == 5);

    ASSERT_always_require(e0->value() == "vine-vinegar");
    ASSERT_always_require(e0->source() == v0);
    ASSERT_always_require(e0->target() == v1);
    ASSERT_always_require(e0 == e0);
    ASSERT_always_forbid(e0 != e0);
    ASSERT_always_forbid(e0 < e0);

    ASSERT_always_require(e1->value() == "violin-vinegar");
    ASSERT_always_require(e1->source() == v2);
    ASSERT_always_require(e1->target() == v1);
    ASSERT_always_require(e1 != e0);
    ASSERT_always_require(e1 < e0 || e0 < e1);
    
    ASSERT_always_require(e2->value() == "vine-visa");
    ASSERT_always_require(e2->source() == v0);
    ASSERT_always_require(e2->target() == v3);
    ASSERT_always_require(e2 != e1);
    ASSERT_always_require(e2 < e1 || e1 < e2);
    
    ASSERT_always_require(e3->value() == "visa-vine");
    ASSERT_always_require(e3->source() == v3);
    ASSERT_always_require(e3->target() == v0);
    ASSERT_always_require(e3 != e2);
    ASSERT_always_require(e3 < e2 || e2 < e3);
    
    ASSERT_always_require(e4->value() == "visa-visa");
    ASSERT_always_require(e4->source() == v3);
    ASSERT_always_require(e4->target() == v3);
    ASSERT_always_require(e4 != e3);
    ASSERT_always_require(e4 < e3 || e3 < e4);

    ASSERT_always_require(v0->nInEdges() == 1);
    ASSERT_always_require(v0->nOutEdges() == 2);
    ASSERT_always_require(v1->nInEdges() == 2);
    ASSERT_always_require(v1->nOutEdges() == 0);
    ASSERT_always_require(v2->nInEdges() == 0);
    ASSERT_always_require(v2->nOutEdges() == 1);
    ASSERT_always_require(v3->nInEdges() == 2);
    ASSERT_always_require(v3->nOutEdges() == 2);
    
    std::cout <<graph;
}

template<class Graph>
void erase_edge() {
    std::cout <<"edge erasure:\n";
    typedef typename Graph::VertexNodeIterator Vertex;
    typedef typename Graph::EdgeNodeIterator Edge;
    
    Graph graph;
    Vertex v0 = graph.insertVertex("vine");
    Vertex v1 = graph.insertVertex("vinegar");
    Vertex v2 = graph.insertVertex("violin");
    Vertex v3 = graph.insertVertex("visa");
    Edge e0 = graph.insertEdge(v0, v1, "vine-vinegar");
    Edge e1 = graph.insertEdge(v2, v1, "violin-vinegar");
    Edge e2 = graph.insertEdge(v0, v3, "vine-visa");
    Edge e3 = graph.insertEdge(v3, v0, "visa-vine");
    Edge e4 = graph.insertEdge(v3, v3, "visa-visa");
    std::cout <<"  initial graph:\n" <<graph;

    graph.eraseEdge(e3);
    std::cout <<"  after erasing 'visa-vine' edge:\n" <<graph;
    ASSERT_always_require(graph.nEdges() == 4);
    ASSERT_always_require(v3->nOutEdges() == 1);
    ASSERT_always_require(v3->nInEdges() == 2);
    ASSERT_always_require(v0->nOutEdges() == 2);
    ASSERT_always_require(v0->nInEdges() == 0);

    graph.eraseEdge(e1);
    std::cout <<"  after erasing 'violin-vinegar' edge:\n" <<graph;
    ASSERT_always_require(graph.nEdges() == 3);
    ASSERT_always_require(v2->nOutEdges() == 0);
    ASSERT_always_require(v2->nInEdges() == 0);
    ASSERT_always_require(v1->nOutEdges() == 0);
    ASSERT_always_require(v1->nInEdges() == 1);

    graph.eraseEdge(e0);
    std::cout <<"  after erasing 'vine-vinegar' edge:\n" <<graph;
    ASSERT_always_require(graph.nEdges() == 2);
    ASSERT_always_require(v0->nOutEdges() == 1);
    ASSERT_always_require(v0->nInEdges() == 0);
    ASSERT_always_require(v1->nOutEdges() == 0);
    ASSERT_always_require(v1->nInEdges() == 0);

    graph.eraseEdge(e4);
    std::cout <<"  after erasing 'visa-visa' edge:\n" <<graph;
    ASSERT_always_require(graph.nEdges() == 1);
    ASSERT_always_require(v3->nOutEdges() == 0);
    ASSERT_always_require(v3->nInEdges() == 1);

    graph.eraseEdge(e2);
    std::cout <<"  after erasing 'vine-visa' edge:\n" <<graph;
    ASSERT_always_require(graph.nEdges() == 0);
    ASSERT_always_require(v0->nOutEdges() == 0);
    ASSERT_always_require(v0->nInEdges() == 0);
    ASSERT_always_require(v3->nOutEdges() == 0);
    ASSERT_always_require(v3->nInEdges() == 0);

    ASSERT_always_forbid(graph.isEmpty());                     // still has vertices
}

template<class Graph>
void erase_vertex() {
    std::cout <<"erase vertices with edges:\n";
    typedef typename Graph::VertexNodeIterator Vertex;
    typedef typename Graph::EdgeNodeIterator Edge;
    
    Graph graph;
    Vertex v0 = graph.insertVertex("vine");
    Vertex v1 = graph.insertVertex("vinegar");
    Vertex v2 = graph.insertVertex("violin");
    Vertex v3 = graph.insertVertex("visa");
    Edge e0 = graph.insertEdge(v0, v1, "vine-vinegar");
    Edge e1 = graph.insertEdge(v2, v1, "violin-vinegar");
    Edge e2 = graph.insertEdge(v0, v3, "vine-visa");
    Edge e3 = graph.insertEdge(v3, v0, "visa-vine");
    Edge e4 = graph.insertEdge(v3, v3, "visa-visa");
    std::cout <<"  initial graph:\n" <<graph;

    graph.eraseVertex(v2);
    ASSERT_always_require(graph.nVertices() == 3);
    ASSERT_always_require(graph.nEdges() == 4);

    graph.eraseVertex(v0);
    ASSERT_always_require(graph.nVertices() == 2);
    ASSERT_always_require(graph.nEdges() == 1);

    graph.eraseVertex(v3);
    ASSERT_always_require(graph.nVertices() == 1);
    ASSERT_always_require(graph.nEdges() == 0);

    graph.eraseVertex(v1);
    ASSERT_always_require(graph.nVertices() == 0);
    ASSERT_always_require(graph.nEdges() == 0);
    ASSERT_always_require(graph.isEmpty());
}

template<class Graph>
void iterator_conversion() {
    std::cout <<"iterator implicit conversions:\n";
    typedef typename Graph::VertexNodeIterator Vertex;
    typedef typename Graph::EdgeNodeIterator Edge;
    
    Graph graph;
    Vertex v0 = graph.insertVertex("vine");
    Vertex v1 = graph.insertVertex("vinegar");
    Edge e0 = graph.insertEdge(v0, v1, "vine-vinegar");
    std::cout <<"  initial graph:\n" <<graph;

    typename Graph::VertexValueIterator vval = v0;
    ASSERT_always_require(*vval == "vine");

    typename Graph::EdgeValueIterator eval = e0;
    ASSERT_always_require(*eval == "vine-vinegar");

#if 0 // [Robb Matzke 2014-04-21]: going the other way is not indended to work (compile error)
    typename Graph::EdgeNodeIterator e0fail = eval;
#endif
    
}

template<class Graph>
void copy_ctor() {
    std::cout <<"copy constructor:\n";
    typedef typename Graph::VertexNodeIterator Vertex;
    typedef typename Graph::EdgeNodeIterator Edge;
    
    Graph graph;
    Vertex v0 = graph.insertVertex("vine");
    Vertex v1 = graph.insertVertex("vinegar");
    Vertex v2 = graph.insertVertex("violin");
    Vertex v3 = graph.insertVertex("visa");
    Edge e0 = graph.insertEdge(v0, v1, "vine-vinegar");
    Edge e1 = graph.insertEdge(v2, v1, "violin-vinegar");
    Edge e2 = graph.insertEdge(v0, v3, "vine-visa");
    Edge e3 = graph.insertEdge(v3, v0, "visa-vine");
    Edge e4 = graph.insertEdge(v3, v3, "visa-visa");
    std::cout <<"  initial graph:\n" <<graph;

    Graph g2(graph);
    std::cout <<"  new copy:\n" <<g2;

    ASSERT_always_require(graph.nVertices() == g2.nVertices());
    for (size_t i=0; i<graph.nVertices(); ++i) {
        typename Graph::ConstVertexNodeIterator v1=graph.findVertex(i), v2=g2.findVertex(i);
        ASSERT_always_require(v1->value() == v2->value());
        ASSERT_always_require(v1->nOutEdges() == v2->nOutEdges());
        ASSERT_always_require(v1->nInEdges() == v2->nInEdges());
    }

    ASSERT_always_require(graph.nEdges() == g2.nEdges());
    for (size_t i=0; i<graph.nEdges(); ++i) {
        typename Graph::ConstEdgeNodeIterator e1=graph.findEdge(i), e2=g2.findEdge(i);
        ASSERT_always_require(e1->value() == e2->value());
        ASSERT_always_require(e1->source()->id() == e2->source()->id());
        ASSERT_always_require(e1->target()->id() == e2->target()->id());
    }
}

template<class Graph>
void assignment() {
    std::cout <<"assignment operator:\n";
    typedef typename Graph::VertexNodeIterator Vertex;
    typedef typename Graph::EdgeNodeIterator Edge;

    Graph g2;
    Vertex v4 = g2.insertVertex("vertex to be clobbered");
    g2.insertEdge(v4, v4, "edge to be clobbered");

    {
        Graph graph;
        Vertex v0 = graph.insertVertex("vine");
        Vertex v1 = graph.insertVertex("vinegar");
        Vertex v2 = graph.insertVertex("violin");
        Vertex v3 = graph.insertVertex("visa");
        Edge e0 = graph.insertEdge(v0, v1, "vine-vinegar");
        Edge e1 = graph.insertEdge(v2, v1, "violin-vinegar");
        Edge e2 = graph.insertEdge(v0, v3, "vine-visa");
        Edge e3 = graph.insertEdge(v3, v0, "visa-vine");
        Edge e4 = graph.insertEdge(v3, v3, "visa-visa");
        std::cout <<"  initial graph:\n" <<graph;
        g2 = graph;

        std::cout <<"  new graph:\n" <<g2;

        ASSERT_always_require(graph.nVertices() == g2.nVertices());
        for (size_t i=0; i<graph.nVertices(); ++i) {
            typename Graph::ConstVertexNodeIterator v1=graph.findVertex(i), v2=g2.findVertex(i);
            ASSERT_always_require(v1->value() == v2->value());
            ASSERT_always_require(v1->nOutEdges() == v2->nOutEdges());
            ASSERT_always_require(v1->nInEdges() == v2->nInEdges());
        }

        ASSERT_always_require(graph.nEdges() == g2.nEdges());
        for (size_t i=0; i<graph.nEdges(); ++i) {
            typename Graph::ConstEdgeNodeIterator e1=graph.findEdge(i), e2=g2.findEdge(i);
            ASSERT_always_require(e1->value() == e2->value());
            ASSERT_always_require(e1->source()->id() == e2->source()->id());
            ASSERT_always_require(e1->target()->id() == e2->target()->id());
        }
    }

    // graph is deleted now.
    for (typename Graph::VertexNodeIterator vi=g2.vertices().begin(); vi!=g2.vertices().end(); ++vi) {
        typename Graph::VertexNode &vertex = *vi;
#if 1 /*DEBUGGING [Robb Matzke 2014-06-02]*/
        typename Graph::EdgeNodeIterator xxx=vertex.outEdges().begin();
        ++xxx;
        typename Graph::EdgeNodeIterator yyy=vertex.outEdges().end();
#endif
        for (typename Graph::EdgeNodeIterator ei=vertex.outEdges().begin(); ei!=vertex.outEdges().end(); ++ei) {
            typename Graph::EdgeNode &edge = *ei;
            ASSERT_always_require(edge.source()->id() == vertex.id());
        }
        for (typename Graph::EdgeNodeIterator ei=vertex.inEdges().begin(); ei!=vertex.inEdges().end(); ++ei) {
            typename Graph::EdgeNode &edge = *ei;
            ASSERT_always_require(edge.target()->id() == vertex.id());
        }
    }
    BOOST_FOREACH (typename Graph::EdgeNode &edge, g2.edges())
        (void) edge.value();
}

class String {
    std::string string_;
public:
    String() {}
    String(const String &other): string_(other.string_) {}
    explicit String(const std::string &s): string_(s) {}
    const std::string& string() const { return string_; }
};

std::ostream& operator<<(std::ostream &output, const String &string) {
    output << string.string();
    return output;
}

template<class Graph>
void conversion() {
    std::cout <<"conversion constructor:\n";
    typedef typename Graph::VertexNodeIterator Vertex;
    typedef typename Graph::EdgeNodeIterator Edge;
    
    Graph graph;
    Vertex v0 = graph.insertVertex("vine");
    Vertex v1 = graph.insertVertex("vinegar");
    Vertex v2 = graph.insertVertex("violin");
    Vertex v3 = graph.insertVertex("visa");
    Edge e0 = graph.insertEdge(v0, v1, "vine-vinegar");
    Edge e1 = graph.insertEdge(v2, v1, "violin-vinegar");
    Edge e2 = graph.insertEdge(v0, v3, "vine-visa");
    Edge e3 = graph.insertEdge(v3, v0, "visa-vine");
    Edge e4 = graph.insertEdge(v3, v3, "visa-visa");
    std::cout <<"  initial graph:\n" <<graph;

    typedef Sawyer::Container::Graph<String, String> Graph2;
    Graph2 g2(graph);
    std::cout <<"  new graph:\n" <<g2;

    ASSERT_always_require(graph.nVertices() == g2.nVertices());
    for (size_t i=0; i<graph.nVertices(); ++i) {
        typename Graph::ConstVertexNodeIterator v1 = graph.findVertex(i);
        Graph2::ConstVertexNodeIterator v2 = g2.findVertex(i);
        ASSERT_always_require(v1->value() == v2->value().string());
        ASSERT_always_require(v1->nOutEdges() == v2->nOutEdges());
        ASSERT_always_require(v1->nInEdges() == v2->nInEdges());
    }

    ASSERT_always_require(graph.nEdges() == g2.nEdges());
    for (size_t i=0; i<graph.nEdges(); ++i) {
        typename Graph::ConstEdgeNodeIterator e1 = graph.findEdge(i);
        Graph2::ConstEdgeNodeIterator e2 = g2.findEdge(i);
        ASSERT_always_require(e1->value() == e2->value().string());
        ASSERT_always_require(e1->source()->id() == e2->source()->id());
        ASSERT_always_require(e1->target()->id() == e2->target()->id());
    }
}

template<class Graph>
void assignment_conversion() {
    std::cout <<"assignment operator conversion:\n";
    typedef typename Graph::VertexNodeIterator Vertex;
    typedef typename Graph::EdgeNodeIterator Edge;
    
    Graph graph;
    Vertex v0 = graph.insertVertex("vine");
    Vertex v1 = graph.insertVertex("vinegar");
    Vertex v2 = graph.insertVertex("violin");
    Vertex v3 = graph.insertVertex("visa");
    Edge e0 = graph.insertEdge(v0, v1, "vine-vinegar");
    Edge e1 = graph.insertEdge(v2, v1, "violin-vinegar");
    Edge e2 = graph.insertEdge(v0, v3, "vine-visa");
    Edge e3 = graph.insertEdge(v3, v0, "visa-vine");
    Edge e4 = graph.insertEdge(v3, v3, "visa-visa");
    std::cout <<"  initial graph:\n" <<graph;

    typedef Sawyer::Container::Graph<String, String> Graph2;
    typedef Graph2::VertexNodeIterator Vertex2;
    Graph2 g2;
    Vertex2 v4 = g2.insertVertex(String("vertex to be clobbered"));
    g2.insertEdge(v4, v4, String("edge to be clobbered"));
    g2 = graph;
    std::cout <<"  new graph:\n" <<g2;

    ASSERT_always_require(graph.nVertices() == g2.nVertices());
    for (size_t i=0; i<graph.nVertices(); ++i) {
        typename Graph::ConstVertexNodeIterator v1 = graph.findVertex(i);
        Graph2::ConstVertexNodeIterator v2 = g2.findVertex(i);
        ASSERT_always_require(v1->value() == v2->value().string());
        ASSERT_always_require(v1->nOutEdges() == v2->nOutEdges());
        ASSERT_always_require(v1->nInEdges() == v2->nInEdges());
    }

    ASSERT_always_require(graph.nEdges() == g2.nEdges());
    for (size_t i=0; i<graph.nEdges(); ++i) {
        typename Graph::ConstEdgeNodeIterator e1 = graph.findEdge(i);
        Graph2::ConstEdgeNodeIterator e2 = g2.findEdge(i);
        ASSERT_always_require(e1->value() == e2->value().string());
        ASSERT_always_require(e1->source()->id() == e2->source()->id());
        ASSERT_always_require(e1->target()->id() == e2->target()->id());
    }
}

struct DfsExpected {
    size_t sourceId, targetId;
    bool sourceSeen, targetSeen;
    std::string edgeName;
    DfsExpected() {}
    DfsExpected(size_t sourceId, bool sourceSeen, size_t targetId, bool targetSeen, const std::string &edgeName)
        : sourceId(sourceId), targetId(targetId), sourceSeen(sourceSeen), targetSeen(targetSeen), edgeName(edgeName) {}
};

template<class Graph>
struct DfsVisitor {
    std::vector<DfsExpected> stack;
    DfsVisitor(const std::vector<DfsExpected> &expected): stack(expected) {
        std::reverse(stack.begin(), stack.end());
    }

    void operator()(const typename Graph::ConstVertexNodeIterator &source, bool sourceSeen,
                    const typename Graph::ConstVertexNodeIterator &target, bool targetSeen,
                    const typename Graph::ConstEdgeNodeIterator &edge) {
        std::cout <<"    "
                  <<"edge " <<edge->value() <<" (v" <<source->id() <<" " <<(sourceSeen ? "  seen" : "unseen") <<" -> "
                  <<"    v" <<target->id() <<" " <<(targetSeen ? "  seen" : "unseen") <<")\n";
        ASSERT_always_require(edge->source() == source);
        ASSERT_always_require(edge->target() == target);
        ASSERT_always_forbid2(stack.empty(), "too many edges visited");
        ASSERT_always_require(source->id() == stack.back().sourceId);
        ASSERT_always_require(sourceSeen == stack.back().sourceSeen);
        ASSERT_always_require(target->id() == stack.back().targetId);
        ASSERT_always_require(targetSeen == stack.back().targetSeen);
        ASSERT_always_require(stack.back().edgeName == edge->value());
        stack.pop_back();
    }
};

static void dfltGraph() {
    // Default is to store nothing (an instance of Sawyer::Nothing) at each vertex and edge.
    typedef Sawyer::Container::Graph<> Graph;
    Graph graph;

    Graph::VertexNodeIterator v1 = graph.insertVertex();
    Graph::VertexNodeIterator v2 = graph.insertVertex();
    Graph::EdgeNodeIterator e1 = graph.insertEdge(v1, v2);

    ASSERT_always_require(v1->value() == Sawyer::Nothing());
    ASSERT_always_require(e1->value() == Sawyer::Nothing());
    
    BOOST_FOREACH (const Sawyer::Nothing &v, graph.vertexValues()) {
        ASSERT_always_require(v == Sawyer::Nothing());
    }
}

static void compileTraversals() {
    using namespace Sawyer::Container::Algorithm;
    typedef Sawyer::Container::Graph<> Graph;
    typedef Graph::VertexNodeIterator Vertex;
    typedef Graph::EdgeNodeIterator Edge;
    Graph g;
    Vertex v = g.insertVertex();
    Edge e = g.insertEdge(v, v);

    DepthFirstForwardGraphTraversal<Graph> dffg1(g, v);
    DepthFirstForwardGraphTraversal<Graph> dffg2(g, e);
    DepthFirstReverseGraphTraversal<Graph> dfrg1(g, v);
    DepthFirstReverseGraphTraversal<Graph> dfrg2(g, e);

    BreadthFirstForwardGraphTraversal<Graph> bffg1(g, v);
    BreadthFirstForwardGraphTraversal<Graph> bffg2(g, e);
    BreadthFirstReverseGraphTraversal<Graph> bfrg1(g, v);
    BreadthFirstReverseGraphTraversal<Graph> bfrg2(g, e);

    DepthFirstForwardVertexTraversal<Graph> dffv1(g, v);
    DepthFirstForwardVertexTraversal<Graph> dffv2(g, e);
    DepthFirstReverseVertexTraversal<Graph> dfrv1(g, v);
    DepthFirstReverseVertexTraversal<Graph> dfrv2(g, e);

    DepthFirstForwardEdgeTraversal<Graph> dffe1(g, v);
    DepthFirstForwardEdgeTraversal<Graph> dffe2(g, e);
    DepthFirstReverseEdgeTraversal<Graph> dfre1(g, v);
    DepthFirstReverseEdgeTraversal<Graph> dfre2(g, e);

    BreadthFirstForwardVertexTraversal<Graph> bffv1(g, v);
    BreadthFirstForwardVertexTraversal<Graph> bffv2(g, e);
    BreadthFirstReverseVertexTraversal<Graph> bfrv1(g, v);
    BreadthFirstReverseVertexTraversal<Graph> bfrv2(g, e);

    BreadthFirstForwardEdgeTraversal<Graph> bffe1(g, v);
    BreadthFirstForwardEdgeTraversal<Graph> bffe2(g, e);
    BreadthFirstReverseEdgeTraversal<Graph> bfre1(g, v);
    BreadthFirstReverseEdgeTraversal<Graph> bfre2(g, e);
}

template<class Graph>
class TraversalAnswer {
    struct Ans {
        Sawyer::Container::Algorithm::TraversalEvent event;
        typename Sawyer::Container::GraphTraits<Graph>::VertexNodeIterator vertex;
        typename Sawyer::Container::GraphTraits<Graph>::EdgeNodeIterator edge;
        Ans(Sawyer::Container::Algorithm::TraversalEvent event,
            typename Sawyer::Container::GraphTraits<Graph>::VertexNodeIterator vertex,
            typename Sawyer::Container::GraphTraits<Graph>::EdgeNodeIterator edge)
            : event(event), vertex(vertex), edge(edge) {}
        bool operator==(const Ans &other) const {
            return event==other.event && vertex==other.vertex && edge==other.edge;
        }
        bool operator!=(const Ans &other) const {
            return !(*this==other);
        }
    };
    std::vector<Ans> ans_;
    size_t current_;
    Graph &graph_;
    bool isGood_;
public:
    TraversalAnswer(Graph &graph): current_(0), graph_(graph), isGood_(true) {}

    void clear() {
        ans_.clear();
        current_ = 0;
        isGood_ = true;
    }
    
    void operator()(Sawyer::Container::Algorithm::TraversalEvent event,
                    typename Sawyer::Container::GraphTraits<Graph>::VertexNodeIterator vertex,
                    typename Sawyer::Container::GraphTraits<Graph>::EdgeNodeIterator edge) {
        ans_.push_back(Ans(event, vertex, edge));
    }

    std::string toString(const Ans &ans) {
        std::ostringstream ss;
        ss <<std::setw(20) <<Sawyer::Container::Algorithm::traversalEventName(ans.event);
        if (ans.vertex==graph_.vertices().end()) {
            ss <<"\tvertex=end";
        } else {
            ss <<"\tvertex=" <<ans.vertex->value();
        }
        if (ans.edge==graph_.edges().end()) {
            ss <<"\tedge=end";
        } else {
            ss <<"\tedge=" <<ans.edge->value();
        }
        return ss.str();
    }
    
    template<class Traversal>
    void check(Traversal &t) {
        Ans got(t.event(), t.vertex(), t.edge());
        if (!isGood_ || current_>=ans_.size() || got!=ans_[current_]) {
            if (isGood_) {
                for (size_t i=0; i<current_; ++i) {
                    std::cout <<"    correct: " <<std::setw(3) <<i <<toString(ans_[i]) <<"\n";
                }
            }
            if (current_ >= ans_.size()) {
                std::cout <<"    FAILED: past end of answer\n";
                std::cout <<"       got:     " <<toString(got) <<"\n";
            } else if (got!=ans_[current_]) {
                std::cout <<"    FAILED:  " <<std::setw(3) <<current_ <<toString(ans_[current_]) <<"\n";
                std::cout <<"       got:     " <<toString(got) <<"\n";
            } else {
                std::cout <<"    correct: " <<std::setw(3) <<current_ <<toString(ans_[current_]) <<"\n";
            }
            isGood_ = false;
        }
        ++current_;
    }

    bool isGood() const {
        return isGood_ && current_==ans_.size();
    }
};

static void traversals() {
    using namespace Sawyer::Container::Algorithm;
    typedef Sawyer::Container::Graph<std::string, std::string> Graph;
    typedef Graph::VertexNodeIterator Vertex;
    typedef Graph::EdgeNodeIterator Edge;

    Graph g;                                            //      A  <--.     //
    Vertex va = g.insertVertex("A");                    //    /   \    \    //
    Vertex vb = g.insertVertex("B");                    //  0/    1\    :   //
    Vertex vc = g.insertVertex("C");                    //  v       v   |   //
    Vertex vd = g.insertVertex("D");                    //  B       C   |   //
    Edge e0 = g.insertEdge(va, vb, "E0");               //   \     /    |   //
    Edge e1 = g.insertEdge(va, vc, "E1");               //   2\  3/     :   //
    Edge e2 = g.insertEdge(vb, vd, "E2");               //     v v     /    //
    Edge e3 = g.insertEdge(vc, vd, "E3");               //      D    4/     //
    Edge e4 = g.insertEdge(vd, va, "E4");               //       \___/      //
    Edge no_edge = g.edges().end();
    Vertex no_vert = g.vertices().end();
    TraversalAnswer<Graph> ans(g);

    std::cout <<"Depth-first forward traversal starting at vertex A\n";
    ans.clear();
    ans(DISCOVER_VERTEX,        va,     no_edge);
    ans(ENTER_VERTEX,           va,     no_edge);
    ans(ENTER_EDGE,             va,     e0);
    ans(DISCOVER_VERTEX,        vb,     e0);
    ans(ENTER_VERTEX,           vb,     e0);
    ans(ENTER_EDGE,             vb,     e2);
    ans(DISCOVER_VERTEX,        vd,     e2);
    ans(ENTER_VERTEX,           vd,     e2);
    ans(ENTER_EDGE,             vd,     e4);
    ans(LEAVE_EDGE,             vd,     e4);
    ans(LEAVE_VERTEX,           vd,     e2);
    ans(LEAVE_EDGE,             vb,     e2);
    ans(LEAVE_VERTEX,           vb,     e0);
    ans(LEAVE_EDGE,             va,     e0);
    ans(ENTER_EDGE,             va,     e1);
    ans(DISCOVER_VERTEX,        vc,     e1);
    ans(ENTER_VERTEX,           vc,     e1);
    ans(ENTER_EDGE,             vc,     e3);
    ans(LEAVE_EDGE,             vc,     e3);
    ans(LEAVE_VERTEX,           vc,     e1);
    ans(LEAVE_EDGE,             va,     e1);
    ans(LEAVE_VERTEX,           va,     no_edge);
    for (DepthFirstForwardGraphTraversal<Graph> t(g, va); t; ++t)
        ans.check(t);
    ASSERT_always_require(ans.isGood());

    std::cout <<"Depth-first forward traversal starting at edge E0\n";
    ans.clear();
    ans(ENTER_EDGE,             no_vert,e0);
    ans(DISCOVER_VERTEX,        vb,     e0);
    ans(ENTER_VERTEX,           vb,     e0);
    ans(ENTER_EDGE,             vb,     e2);
    ans(DISCOVER_VERTEX,        vd,     e2);
    ans(ENTER_VERTEX,           vd,     e2);
    ans(ENTER_EDGE,             vd,     e4);
    ans(DISCOVER_VERTEX,        va,     e4);
    ans(ENTER_VERTEX,           va,     e4);
    ans(ENTER_EDGE,             va,     e1);
    ans(DISCOVER_VERTEX,        vc,     e1);
    ans(ENTER_VERTEX,           vc,     e1);
    ans(ENTER_EDGE,             vc,     e3);
    ans(LEAVE_EDGE,             vc,     e3);
    ans(LEAVE_VERTEX,           vc,     e1);
    ans(LEAVE_EDGE,             va,     e1);
    ans(LEAVE_VERTEX,           va,     e4);
    ans(LEAVE_EDGE,             vd,     e4);
    ans(LEAVE_VERTEX,           vd,     e2);
    ans(LEAVE_EDGE,             vb,     e2);
    ans(LEAVE_VERTEX,           vb,     e0);
    ans(LEAVE_EDGE,             no_vert,e0);
    for (DepthFirstForwardGraphTraversal<Graph> t(g, e0); t; ++t)
        ans.check(t);
    ASSERT_always_require(ans.isGood());

    std::cout <<"Depth-first reverse traversal starting at vertex A\n";
    ans.clear();
    ans(DISCOVER_VERTEX,        va,     no_edge);
    ans(ENTER_VERTEX,           va,     no_edge);
    ans(ENTER_EDGE,             va,     e4);
    ans(DISCOVER_VERTEX,        vd,     e4);
    ans(ENTER_VERTEX,           vd,     e4);
    ans(ENTER_EDGE,             vd,     e2);
    ans(DISCOVER_VERTEX,        vb,     e2);
    ans(ENTER_VERTEX,           vb,     e2);
    ans(ENTER_EDGE,             vb,     e0);
    ans(LEAVE_EDGE,             vb,     e0);
    ans(LEAVE_VERTEX,           vb,     e2);
    ans(LEAVE_EDGE,             vd,     e2);
    ans(ENTER_EDGE,             vd,     e3);
    ans(DISCOVER_VERTEX,        vc,     e3);
    ans(ENTER_VERTEX,           vc,     e3);
    ans(ENTER_EDGE,             vc,     e1);
    ans(LEAVE_EDGE,             vc,     e1);
    ans(LEAVE_VERTEX,           vc,     e3);
    ans(LEAVE_EDGE,             vd,     e3);
    ans(LEAVE_VERTEX,           vd,     e4);
    ans(LEAVE_EDGE,             va,     e4);
    ans(LEAVE_VERTEX,           va,     no_edge);
    for (DepthFirstReverseGraphTraversal<Graph> t(g, va); t; ++t)
        ans.check(t);
    ASSERT_always_require(ans.isGood());

    std::cout <<"Depth-first reverse traversal starting at edge E0\n";
    ans.clear();
    ans(ENTER_EDGE,             no_vert,e0);
    ans(DISCOVER_VERTEX,        va,     e0);
    ans(ENTER_VERTEX,           va,     e0);
    ans(ENTER_EDGE,             va,     e4);
    ans(DISCOVER_VERTEX,        vd,     e4);
    ans(ENTER_VERTEX,           vd,     e4);
    ans(ENTER_EDGE,             vd,     e2);
    ans(DISCOVER_VERTEX,        vb,     e2);
    ans(ENTER_VERTEX,           vb,     e2);
    ans(LEAVE_VERTEX,           vb,     e2);
    ans(LEAVE_EDGE,             vd,     e2);
    ans(ENTER_EDGE,             vd,     e3);
    ans(DISCOVER_VERTEX,        vc,     e3);
    ans(ENTER_VERTEX,           vc,     e3);
    ans(ENTER_EDGE,             vc,     e1);
    ans(LEAVE_EDGE,             vc,     e1);
    ans(LEAVE_VERTEX,           vc,     e3);
    ans(LEAVE_EDGE,             vd,     e3);
    ans(LEAVE_VERTEX,           vd,     e4);
    ans(LEAVE_EDGE,             va,     e4);
    ans(LEAVE_VERTEX,           va,     e0);
    ans(LEAVE_EDGE,             no_vert,e0);
    for (DepthFirstReverseGraphTraversal<Graph> t(g, e0); t; ++t)
        ans.check(t);
    ASSERT_always_require(ans.isGood());

    std::cout <<"Breadth-first forward traversal starting at vertex A\n";
    ans.clear();
    ans(DISCOVER_VERTEX,        va,     no_edge);
    ans(ENTER_VERTEX,           va,     no_edge);
    ans(ENTER_EDGE,             va,     e0);
    ans(DISCOVER_VERTEX,        vb,     e0);
    ans(LEAVE_EDGE,             va,     e0);
    ans(ENTER_EDGE,             va,     e1);
    ans(DISCOVER_VERTEX,        vc,     e1);
    ans(LEAVE_EDGE,             va,     e1);
    ans(LEAVE_VERTEX,           va,     no_edge);
    ans(ENTER_VERTEX,           vb,     e0);
    ans(ENTER_EDGE,             vb,     e2);
    ans(DISCOVER_VERTEX,        vd,     e2);
    ans(LEAVE_EDGE,             vb,     e2);
    ans(LEAVE_VERTEX,           vb,     e0);
    ans(ENTER_VERTEX,           vc,     e1);
    ans(ENTER_EDGE,             vc,     e3);
    ans(LEAVE_EDGE,             vc,     e3);
    ans(LEAVE_VERTEX,           vc,     e1);
    ans(ENTER_VERTEX,           vd,     e2);
    ans(ENTER_EDGE,             vd,     e4);
    ans(LEAVE_EDGE,             vd,     e4);
    ans(LEAVE_VERTEX,           vd,     e2);
    for (BreadthFirstForwardGraphTraversal<Graph> t(g, va); t; ++t)
        ans.check(t);
    ASSERT_always_require(ans.isGood());

    std::cout <<"Breadth-first forward traversal starting at edge E0\n";
    ans.clear();
    ans(ENTER_EDGE,             no_vert,e0);
    ans(DISCOVER_VERTEX,        vb,     e0);
    ans(LEAVE_EDGE,             no_vert,e0);
    ans(ENTER_VERTEX,           vb,     e0);
    ans(ENTER_EDGE,             vb,     e2);
    ans(DISCOVER_VERTEX,        vd,     e2);
    ans(LEAVE_EDGE,             vb,     e2);
    ans(LEAVE_VERTEX,           vb,     e0);
    ans(ENTER_VERTEX,           vd,     e2);
    ans(ENTER_EDGE,             vd,     e4);
    ans(DISCOVER_VERTEX,        va,     e4);
    ans(LEAVE_EDGE,             vd,     e4);
    ans(LEAVE_VERTEX,           vd,     e2);
    ans(ENTER_VERTEX,           va,     e4);
    ans(ENTER_EDGE,             va,     e1);
    ans(DISCOVER_VERTEX,        vc,     e1);
    ans(LEAVE_EDGE,             va,     e1);
    ans(LEAVE_VERTEX,           va,     e4);
    ans(ENTER_VERTEX,           vc,     e1);
    ans(ENTER_EDGE,             vc,     e3);
    ans(LEAVE_EDGE,             vc,     e3);
    ans(LEAVE_VERTEX,           vc,     e1);
    for (BreadthFirstForwardGraphTraversal<Graph> t(g, e0); t; ++t)
        ans.check(t);
    ASSERT_always_require(ans.isGood());

    std::cout <<"Breadth-first reverse traversal starting at vertex A\n";
    ans.clear();
    ans(DISCOVER_VERTEX,        va,     no_edge);
    ans(ENTER_VERTEX,           va,     no_edge);
    ans(ENTER_EDGE,             va,     e4);
    ans(DISCOVER_VERTEX,        vd,     e4);
    ans(LEAVE_EDGE,             va,     e4);
    ans(LEAVE_VERTEX,           va,     no_edge);
    ans(ENTER_VERTEX,           vd,     e4);
    ans(ENTER_EDGE,             vd,     e2);
    ans(DISCOVER_VERTEX,        vb,     e2);
    ans(LEAVE_EDGE,             vd,     e2);
    ans(ENTER_EDGE,             vd,     e3);
    ans(DISCOVER_VERTEX,        vc,     e3);
    ans(LEAVE_EDGE,             vd,     e3);
    ans(LEAVE_VERTEX,           vd,     e4);
    ans(ENTER_VERTEX,           vb,     e2);
    ans(ENTER_EDGE,             vb,     e0);
    ans(LEAVE_EDGE,             vb,     e0);
    ans(LEAVE_VERTEX,           vb,     e2);
    ans(ENTER_VERTEX,           vc,     e3);
    ans(ENTER_EDGE,             vc,     e1);
    ans(LEAVE_EDGE,             vc,     e1);
    ans(LEAVE_VERTEX,           vc,     e3);
    for (BreadthFirstReverseGraphTraversal<Graph> t(g, va); t; ++t)
        ans.check(t);
    ASSERT_always_require(ans.isGood());
    
    std::cout <<"Breadth-first reverse traversal starting at edge E0\n";
    ans.clear();
    ans(ENTER_EDGE,             no_vert,e0);
    ans(DISCOVER_VERTEX,        va,     e0);
    ans(LEAVE_EDGE,             no_vert,e0);
    ans(ENTER_VERTEX,           va,     e0);
    ans(ENTER_EDGE,             va,     e4);
    ans(DISCOVER_VERTEX,        vd,     e4);
    ans(LEAVE_EDGE,             va,     e4);
    ans(LEAVE_VERTEX,           va,     e0);
    ans(ENTER_VERTEX,           vd,     e4);
    ans(ENTER_EDGE,             vd,     e2);
    ans(DISCOVER_VERTEX,        vb,     e2);
    ans(LEAVE_EDGE,             vd,     e2);
    ans(ENTER_EDGE,             vd,     e3);
    ans(DISCOVER_VERTEX,        vc,     e3);
    ans(LEAVE_EDGE,             vd,     e3);
    ans(LEAVE_VERTEX,           vd,     e4);
    ans(ENTER_VERTEX,           vb,     e2);
    ans(LEAVE_VERTEX,           vb,     e2);
    ans(ENTER_VERTEX,           vc,     e3);
    ans(ENTER_EDGE,             vc,     e1);
    ans(LEAVE_EDGE,             vc,     e1);
    ans(LEAVE_VERTEX,           vc,     e3);
    for (BreadthFirstReverseGraphTraversal<Graph> t(g, e0); t; ++t)
        ans.check(t);
    ASSERT_always_require(ans.isGood());

    std::cout <<"Depth-first forward traversal starting at vertex A and revisiting vertices\n";
    ans.clear();
    ans(DISCOVER_VERTEX,        va,     no_edge);
    ans(ENTER_VERTEX,           va,     no_edge);
    ans(ENTER_EDGE,             va,     e0);
    ans(DISCOVER_VERTEX,        vb,     e0);
    ans(ENTER_VERTEX,           vb,     e0);
    ans(ENTER_EDGE,             vb,     e2);
    ans(DISCOVER_VERTEX,        vd,     e2);
    ans(ENTER_VERTEX,           vd,     e2);
    ans(ENTER_EDGE,             vd,     e4);
    ans(LEAVE_EDGE,             vd,     e4);
    ans(LEAVE_VERTEX,           vd,     e2);
    ans(LEAVE_EDGE,             vb,     e2);
    ans(LEAVE_VERTEX,           vb,     e0);
    ans(LEAVE_EDGE,             va,     e0);
    ans(ENTER_EDGE,             va,     e1);
    ans(DISCOVER_VERTEX,        vc,     e1);
    ans(ENTER_VERTEX,           vc,     e1);
    ans(ENTER_EDGE,             vc,     e3);
    ans(DISCOVER_VERTEX,        vd,     e3);
    ans(ENTER_VERTEX,           vd,     e3);
    ans(ENTER_EDGE,             vd,     e4);
    ans(LEAVE_EDGE,             vd,     e4);
    ans(LEAVE_VERTEX,           vd,     e3);
    ans(LEAVE_EDGE,             vc,     e3);
    ans(LEAVE_VERTEX,           vc,     e1);
    ans(LEAVE_EDGE,             va,     e1);
    ans(LEAVE_VERTEX,           va,     no_edge);
    for (DepthFirstForwardGraphTraversal<Graph> t(g, va); t; ++t) {
        ans.check(t);
        if (t.event() == LEAVE_VERTEX)
            t.allowRediscovery(t.vertex());
    }
    ASSERT_always_require(ans.isGood());
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
    copy_ctor<G1>();
    assignment<G1>();
    conversion<G1>();
    assignment_conversion<G1>();
    dfltGraph();
    compileTraversals();
    traversals();
}

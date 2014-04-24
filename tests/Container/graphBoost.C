#include <sawyer/GraphBoost.h>                          // BGL interface for Sawyer::Container::Graph
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/foreach.hpp>
#include <string>

// User-defined data per vertex and stored inside the graph.
struct VertexData {
    double x, y;
    std::string name;

    VertexData(): x(0.0), y(0.0) {}
    VertexData(const VertexData &other): x(other.x), y(other.y), name(other.name) {}
    VertexData(double x, double y, const std::string &name): x(x), y(y), name(name) {}
};

// Additional user-defined data per vertex, but stored outside the graph in an std::vector
struct VertexExternalData {
    double r, g, b;
    std::string name;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Boost Graph Library Example
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// BGL uses a property mechanism for associating user-defined data with vertices and/or edges, in contrast to Sawyer's Graph
// container which is designed more like STL containers, where the user data types are provided as template arguments. 
struct vertex_data_t {                                  // only needed for BGL
    typedef boost::vertex_property_tag kind;
};

struct vertex_external_data_t {                         // only needed for BGL
    typedef boost::vertex_property_tag kind;
};

struct vertex_id_t {                                    // only needed for BGL
    typedef boost::vertex_property_tag kind;
};


void example_adjacency_list_graph() {
    
    // Types.  Aside from these types main types, which are so pervasive and long-winded that they're often declared in as high
    // a scope as possible, this function treats each sub-example is if it could be in its own function, and declares things as
    // locally as possible.  Although it's somewhat artificial here, in practice this is actually what happens, and therefore
    // the amount of verbosity here will be close to what occurs in practice.
    typedef boost::property<vertex_data_t, VertexData, boost::property<vertex_id_t, size_t> > VertexProperties;
    typedef boost::property<boost::edge_name_t, std::string> EdgeProperties;
    typedef boost::adjacency_list<boost::setS, boost::setS, boost::bidirectionalS, VertexProperties, EdgeProperties> Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor VertexDesc;
    typedef boost::graph_traits<Graph>::edge_descriptor EdgeDesc;
    typedef boost::graph_traits<Graph>::edge_iterator EdgeIter;

    // Construct an empty graph
    Graph graph;

    // Note about name qualification.  We intentially do not import the "boost" namespace. Although imports can make toy
    // examples more readable, they make real-world code less readable.  This effect is because in an example, the reader
    // already has in mind that any unqualified names with no previously visible (to the reader) declaration come from the
    // namespace that the example is expounding (e.g., examples found in boost documentation are using the "boost" name
    // space). On the other hand, in the real world, imports only require the reader to go track-down every unqualified name by
    // hand to figure out what the function does.
    //
    // Although Koenig lookup would find most of the BGL functions we're calling even without qualification, we can't depend on
    // it in the real world because the underlying graph type might not be in the "boost" name space.  After all, one of BGL's
    // selling points is that you can simply plug in your own graph type and algorithms "just work".

    // Add some vertices. The BGL API requires three steps:
    //   1. Obtain the storage for the vertex user-defined data
    //   2. Add a vertex with default-constructed user-defined data
    //   3. Modify the user-defined data associated with the new vertex
    // Also, we'll be creating a lookup table below, and we want to have O(1) lookup times. That means we need small, unique
    // ID numbers for each vertex.  There isn't a good way (i.e., fast and simple) to do this with the BGL interface when the
    // graph needs to support edge and/or vertex erasure.
    boost::property_map<Graph, vertex_data_t>::type vertexData = get(vertex_data_t(), graph);
    boost::property_map<Graph, vertex_id_t>::type vertexIds = get(vertex_id_t(), graph);
    VertexDesc a = boost::add_vertex(graph);
    boost::put(vertexData, a, VertexData(1.0, 1.0, "vampire"));
    boost::put(vertexIds, a, boost::num_vertices(graph)-1);
    VertexDesc b = boost::add_vertex(graph);
    boost::put(vertexData, b, VertexData(2.0, 1.0, "venison"));
    boost::put(vertexIds, b, boost::num_vertices(graph)-1);
    VertexDesc c = boost::add_vertex(graph);
    boost::put(vertexData, c, VertexData(3.1, 1.1, "vermouth"));
    boost::put(vertexIds, c, boost::num_vertices(graph)-1);
    VertexDesc d = boost::add_vertex(graph);
    boost::put(vertexData, d, VertexData(6.6, 1.4, "vogue"));
    boost::put(vertexIds, d, boost::num_vertices(graph)-1);
    VertexDesc e = add_vertex(graph);
    boost::put(vertexData, e, VertexData(8.9, 1.5, "vigil"));
    boost::put(vertexIds, e, boost::num_vertices(graph)-1);

    // Add some edges. BGL returns a pair where second is always true for graphs that allow parallel edges. As with adding
    // vertices, adding edges requires three steps, although we are able to hoist the first step to the top in these examples
    // (not always convenient in real life).
    boost::property_map<Graph, boost::edge_name_t>::type edgeData = boost::get(boost::edge_name_t(), graph);
    EdgeDesc ab = boost::add_edge(a, b, graph).first;
    boost::put(edgeData, ab, "elephant");
    EdgeDesc ad = boost::add_edge(a, d, graph).first;
    boost::put(edgeData, ad, "echidna");
    EdgeDesc ca = boost::add_edge(c, a, graph).first;
    boost::put(edgeData, ca, "emu");
    EdgeDesc dc = boost::add_edge(d, c, graph).first;
    boost::put(edgeData, dc, "eagle");
    EdgeDesc ce = boost::add_edge(c, e, graph).first;
    boost::put(edgeData, ce, "eel");
    EdgeDesc bd = boost::add_edge(b, d, graph).first;
    boost::put(edgeData, bd, "earwig");
    EdgeDesc de = boost::add_edge(d, e, graph).first;
    boost::put(edgeData, de, "egret");

    // Iterate over the edges of a graph manually and print their associated std::string values.  This is a two-step
    // process: first get the storage for the edge properties, then look up the particular data based on the edge.
    // We'd like to be able to print an edge ID number, but alas, it takes much more work to do that (see how we did ID numbers
    // for vertices).
    std::cout <<"edges:\n";
    EdgeIter e_iter, e_end;
    for (boost::tie(e_iter, e_end)=boost::edges(graph); e_iter!=e_end; ++e_iter) {
        typedef boost::property_map<Graph, boost::edge_name_t>::type EdgeDataMap;
        EdgeDataMap edgeDataMap = boost::get(boost::edge_name_t(), graph);
        std::cout <<"  [?] = " <<boost::get(edgeDataMap, *e_iter) <<"\n";
    }

    // Iterate over the vertices of a graph with foreach and make each name upper case with a "!" at the end.
    BOOST_FOREACH (const VertexDesc &vertex, boost::vertices(graph)) {
        typedef boost::property_map<Graph, vertex_data_t>::type VertexDataMap;
        VertexDataMap vertexDataMap = boost::get(vertex_data_t(), graph);
        VertexData &vertexData = boost::get(vertexDataMap, vertex);
        vertexData.name = boost::to_upper_copy(vertexData.name) + "!";
    }

    // Iterate over the vertices and print the data associated with each.
    std::cout <<"vertices:\n";
    BOOST_FOREACH (const VertexDesc &vertex, boost::vertices(graph)) {
        typedef boost::property_map<Graph, vertex_data_t>::type VertexDataMap;
        VertexDataMap vertexDataMap = boost::get(vertex_data_t(), graph);
        VertexData &vertexData = boost::get(vertexDataMap, vertex);
        std::cout <<"  " <<vertexData.x <<"\t" <<vertexData.y <<"\t" <<vertexData.name <<"\n";
    }

    // Create a lookup table that contains additional information per vertex that we will not add to the graph.  BGL calls
    // these external properties.  We want O(1) lookup time for this table, which means we need to store vertex ID numbers in a
    // property of the graph.  We could have used "vecS" for our vertex container, but that trades off other performance (and
    // we'd like the potential of having O(1) erasure).
    std::vector<VertexExternalData> table(boost::num_vertices(graph), VertexExternalData());
    BOOST_FOREACH (const VertexDesc &vertex, boost::vertices(graph)) {
        typedef boost::property_map<Graph, vertex_id_t>::type VertexIdMap;
        VertexIdMap vertexIdMap = boost::get(vertex_id_t(), graph);
        size_t id = boost::get(vertexIdMap, vertex);
        assert(id < table.size());
        typedef boost::property_map<Graph, vertex_data_t>::type VertexDataMap;
        VertexDataMap vertexDataMap = boost::get(vertex_data_t(), graph);
        const VertexData &vertexData = boost::get(vertexDataMap, vertex);
        table[id].r = sin(vertexData.x);
        table[id].g = cos(vertexData.y);
        table[id].b = 0.5;
        table[id].name = boost::to_lower_copy(vertexData.name);
    }
    std::cout <<"table:\n";
    for (size_t i=0; i<table.size(); ++i)
        std::cout <<"  " <<table[i].r <<"\t" <<table[i].g <<"\t" <<table[i].b <<"\t" <<table[i].name <<"\n";

    // Erase a vertex from the graph. Removing and deleting a node from a container in STL is called "erase", which is a
    // different concept than "remove" in STL.  Boost uses "remove", STL and Sawyer use "erase".  The other problem we have is
    // that in order for our external rgb table to be useful in the long run for a highly dynamic graph, we need to renumber
    // the vertexId property, and move entries in the rgb table.  The approach we take here is crude, simple, and slow, but
    // already more complex than the corresponding Sawyer example below.
    VertexDesc toRemove = c;
    {
        // Get ID number that we're removing
        typedef boost::property_map<Graph, vertex_id_t>::type VertexIdMap;
        VertexIdMap vertexIdMap = boost::get(vertex_id_t(), graph);
        size_t removedId = boost::get(vertexIdMap, toRemove);

        // Renumber vertices with higher IDs, linear time.  We could have rather swapped with the highest ID number, but that
        // means we need to keep track of it, making vertex insertion more complicated than it already is.
        BOOST_FOREACH (const VertexDesc &vertex, boost::vertices(graph)) {
            size_t id = boost::get(vertexIdMap, vertex);
            if (id > removedId)
                boost::put(vertexIdMap, vertex, id-1);
        }

        // Move members of the table, again linear time.
        table.erase(table.begin()+removedId);

        // Erase the vertex.  BGL requires us to first erase the incident edges.  At least this part is linear in the number of
        // incident edges, and was our motivation for using setS as the edge and vertex containers.
        boost::clear_vertex(toRemove, graph);
        boost::remove_vertex(toRemove, graph);
    }

    // Show the table again.
    std::cout <<"table after vertex erasure:\n";
    for (size_t i=0; i<table.size(); ++i)
        std::cout <<"  " <<table[i].r <<"\t" <<table[i].g <<"\t" <<table[i].b <<"\t" <<table[i].name <<"\n";
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Sawyer::Container::Graph Example
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This example does the exact same thing as the BGL example above.
void example_sawyer_graph() {
    // Types.  Sawyer uses iterators for both iteration and pointing, which is consistent with how pointers are used to iterate
    // over arrays, and therefore some users omit "Iterator" from the local type names (like we do here).  Note that the
    // internally-stored data for vertices and edges is specified with the two template arguments.
    typedef Sawyer::Container::Graph<VertexData, std::string> Graph;
    typedef Graph::VertexNodeIterator Vertex;           // a "pointer" to all information about a vertex
    typedef Graph::EdgeNodeIterator Edge;               // a "pointer" to all information about an edge

    // Construct an empty graph
    Graph graph;

    // Note about name lookup.  Since Sawyer uses methods rather than functions, name lookup is not an issue and the only
    // names that need qualification are the type names.  The Sawyer API looks like an object-oriented API rather than a bunch
    // of arbitrarily related functions.

    // Add some vertices. Like STL containers, Sawyer vertices are immediately associated with user-defined values which are
    // copied into the graph.
    Vertex a = graph.insert(VertexData(1.0, 1.0, "vampire"));
    Vertex b = graph.insert(VertexData(2.0, 1.0, "venison"));
    Vertex c = graph.insert(VertexData(3.1, 1.1, "vermouth"));
    Vertex d = graph.insert(VertexData(6.6, 1.4, "vogue"));
    Vertex e = graph.insert(VertexData(8.9, 1.5, "vigil"));

    // Add some edges. Arguments are source and destination vertices, and user-defined value.
    Edge ab = graph.insert(a, b, "elephant");
    Edge ad = graph.insert(a, d, "echidna");
    Edge ca = graph.insert(c, a, "emu");
    Edge dc = graph.insert(d, c, "eagle");
    Edge ce = graph.insert(c, e, "eel");
    Edge bd = graph.insert(b, d, "earwig");
    Edge de = graph.insert(d, e, "egret");

    // Print the values stored for edges.  None of the Sawyer containers have direct begin() and end() methods, but rather an
    // iterator range proxy. This has two advantages: range-returning functions can be chained together as with the boost
    // string algorithms, not possible with STL containers; and we can easily support a richer collection of iterators.  For
    // instance, Graph has edges(), vertices(), edgeValues(), and vertexValues() methods that return eight different types of
    // iterators (const and non-const versions for each). So-called "value" iterators that return only the user-defined value
    // when they're dereferenced, and "node" iterators that return an internal data structure that wraps the user-defined value
    // and provides additional information (ID numbers and connectivity info in the case of Graph).  We'll use the "node"
    // iterators in this example so we can get to the ID numbers.
    //
    // Speaking of ID numbers, the Graph automatically calculates small, sequential ID numbers for every vertex and edge (one
    // set for vertices and another for edges).  These numbers are stable across insertion, but if an vertex (or edge) is
    // erased from the graph then the highest vertex (or edge) ID number is changed to fill in the hole that's left.  This,
    // combined with other internal data structures, makes erasure a constant-time operation.
    std::cout <<"edges:\n";
    for (Edge edge=graph.edges().begin(); edge!=graph.edges().end(); ++edge)
        std::cout <<"  [" <<edge->id() <<"] = " <<edge->value() <<"\n";

    // Make each vertex name upper case.  This time we use a "value" iterator since we don't need ID or connectivity info.
    BOOST_FOREACH (VertexData &vertexData, graph.vertexValues())
        vertexData.name = boost::to_upper_copy(vertexData.name) + "!";

    // Print the vertex information.
    std::cout <<"vertices:\n";
    BOOST_FOREACH (const VertexData &vertexData, graph.vertexValues())
        std::cout <<"  " <<vertexData.x <<"\t" <<vertexData.y <<"\t" <<vertexData.name <<"\n";

    // Create a lookup table that contains additional information per vertex that we will not add to the graph.  We want O(1)
    // lookup time for this table, which means we need to use vertex ID numbers. These ID numbers are maintained by the graph
    // object itself and are guaranteed to be sequential beginning at zero.  This time we use "node" iterators in order to have
    // access to vertex ID numbers.
    std::vector<VertexExternalData> table(graph.nVertices(), VertexExternalData());
    BOOST_FOREACH (const Graph::VertexNode &vertex, graph.vertices()) {
        table[vertex.id()].r = sin(vertex.value().x);
        table[vertex.id()].g = cos(vertex.value().y);
        table[vertex.id()].b = 0.5;
        table[vertex.id()].name = boost::to_lower_copy(vertex.value().name);
    }
    std::cout <<"table:\n";
    for (size_t i=0; i<table.size(); ++i)
        std::cout <<"  " <<table[i].r <<"\t" <<table[i].g <<"\t" <<table[i].b <<"\t" <<table[i].name <<"\n";

    // Remove a vertex and update the lookup table.  Sawyer automatically removes edges that were connected to the removed
    // vertex, and recomputes edge and vertex IDs.  The total time is linear in the number of edges incident to the removed
    // vertex.  We have to adjust the lookup table entries ourselves (FIXME[Robb Matzke 2014-04-22]: this might be something we
    // could improve, especially when there are incident edges whose tables need to be also adjusted).
    Vertex toRemove = c;
    size_t removedId = toRemove->id();
    graph.erase(toRemove);                              // "erase" is for either vertices or edges depending on argument.
    std::swap(table[removedId], table.back());          // fix up our table
    table.pop_back();                                   // erase the data for the vertex we just removed

    // Show the table again.  The elements won't be in the same order as the BGL example because we used an O(1) fixup
    // rather than the O(n) fixup used in the BGL example.
    std::cout <<"table after vertex erasure:\n";
    for (size_t i=0; i<table.size(); ++i)
        std::cout <<"  " <<table[i].r <<"\t" <<table[i].g <<"\t" <<table[i].b <<"\t" <<table[i].name <<"\n";
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Hybrid Example
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This example uses the BGL wrapper around a Sawyer::Container::Graph.  The idea is that we can use either API interchangeably
// and use the algorithms provided by BGL.
void hybrid_example() {
    // Use Sawyer as the graph.
    typedef Sawyer::Container::Graph<VertexData, std::string> Graph;

    // Types.  Get types from boost rather than Sawyer.
    typedef boost::graph_traits<Graph>::vertex_descriptor VertexDesc;
    typedef boost::graph_traits<Graph>::edge_descriptor EdgeDesc;
    typedef boost::graph_traits<Graph>::edge_iterator EdgeIter;

    // Construct an empty graph
    Graph graph;

    // Add some vertices.  These functions need qualified names because graph's type is not in boost namespace, so Koenig
    // lookup fails.  We're mixing the BGL and Sawyer APIs here because we don't yet support BGL graph-internal properties on
    // Sawyer graphs.  Anyway, we wouldn't need the vertex ID property that we used for the pure BGL example since Sawyer
    // maintains its own ID numbers.
    VertexDesc a = boost::add_vertex(graph);
    graph.findVertex(a)->value() = VertexData(1.0, 1.0, "vampire"); // mixing BGL and Sawyer. This is O(1) time.
    VertexDesc b = boost::add_vertex(graph);
    graph.findVertex(b)->value() = VertexData(2.0, 1.0, "venison");
    VertexDesc c = boost::add_vertex(graph);
    graph.findVertex(c)->value() = VertexData(3.1, 1.1, "vermouth");
    VertexDesc d = boost::add_vertex(graph);
    graph.findVertex(d)->value() = VertexData(6.6, 1.4, "vogue");
    VertexDesc e = boost::add_vertex(graph);
    graph.findVertex(e)->value() = VertexData(8.9, 1.5, "vigil");

    // Add some edges. Mixing BGL and Sawyer APIs for the internal properties like we did for vertices.
    EdgeDesc ab = boost::add_edge(a, b, graph).first;
    graph.findEdge(ab)->value() = "elephant";           // This is O(1) time
    EdgeDesc ad = boost::add_edge(a, d, graph).first;
    graph.findEdge(ad)->value() = "echidna";
    EdgeDesc ca = boost::add_edge(c, a, graph).first;
    graph.findEdge(ca)->value() = "emu";
    EdgeDesc dc = boost::add_edge(d, c, graph).first;
    graph.findEdge(dc)->value() = "eagle";
    EdgeDesc ce = boost::add_edge(c, e, graph).first;
    graph.findEdge(ce)->value() = "eel";
    EdgeDesc bd = boost::add_edge(b, d, graph).first;
    graph.findEdge(bd)->value() = "earwig";
    EdgeDesc de = boost::add_edge(d, e, graph).first;
    graph.findEdge(de)->value() = "egret";

    // Iterate over the edges of a graph manually and print their associated std::string values.  Again, we're mixing the BGL
    // and Sawyer APIs in order to get to the internal edge properties.
    std::cout <<"edges:\n";
    EdgeIter e_iter, e_end;
    for (boost::tie(e_iter, e_end)=boost::edges(graph); e_iter!=e_end; ++e_iter)
        std::cout <<"  [" <<*e_iter <<"] = " <<graph.findEdge(*e_iter)->value() <<"\n";

    // Iterate over the vertices of a graph with foreach and make each name upper case with a "!" at the end. Again mixing BGL
    // and Sawyer APIs to access internal vertex properties.
    BOOST_FOREACH (VertexDesc vertex, boost::vertices(graph)) {
        VertexData &vertexData = graph.findVertex(vertex)->value();
        vertexData.name = boost::to_upper_copy(vertexData.name) + "!";
    }

    // Iterate over the vertices and print the data associated with each. Again mixing BGL and Sawyer APIs to access internal
    // vertex properties.
    std::cout <<"vertices:\n";
    BOOST_FOREACH (VertexDesc vertex, boost::vertices(graph)) {
        VertexData &vertexData = graph.findVertex(vertex)->value();
        std::cout <<"  " <<vertexData.x <<"\t" <<vertexData.y <<"\t" <<vertexData.name <<"\n";
    }

    // Create a lookup table that contains additional information per vertex that we will not add to the graph.  BGL calls
    // these external properties, and we can use the BGL interface (or the Sawyer interface if we like).
    std::vector<VertexExternalData> table(boost::num_vertices(graph), VertexExternalData());
    BOOST_FOREACH (const VertexDesc &vertex, boost::vertices(graph)) {
        assert(vertex < table.size());                  // VertexDesc is exactly an ID
        const VertexData &vertexData = graph.findVertex(vertex)->value();
        table[vertex].r = sin(vertexData.x);
        table[vertex].g = cos(vertexData.y);
        table[vertex].b = 0.5;
        table[vertex].name = boost::to_lower_copy(vertexData.name);
    }
    std::cout <<"table:\n";
    for (size_t i=0; i<table.size(); ++i)
        std::cout <<"  " <<table[i].r <<"\t" <<table[i].g <<"\t" <<table[i].b <<"\t" <<table[i].name <<"\n";

    // Erase a vertex from the graph. Fortunately Sawyer keeps track of ID numbers for us, so we don't need to jump through all
    // the hoops we had to in the pure BGL example.
    VertexDesc toRemove = c;
    {
        // Get ID number that we're removing (vertex_descriptor and IDs are the same thing in the hybrid approach)
        size_t removedId = toRemove;

        // Erase the vertex.  BGL requires us to first erase the incident edges.
        boost::clear_vertex(toRemove, graph);
        boost::remove_vertex(toRemove, graph);

        // Move members of the table, this time by swapping since Sawyer's renumbering method is part of its public behavior.
        std::swap(table[removedId], table.back());
        table.pop_back();
    }

    // Show the table again.
    std::cout <<"table after vertex erasure:\n";
    for (size_t i=0; i<table.size(); ++i)
        std::cout <<"  " <<table[i].r <<"\t" <<table[i].g <<"\t" <<table[i].b <<"\t" <<table[i].name <<"\n";
}

int main() {
    std::cout <<"======== BOOST adjacency_list_graph ========\n";
    example_adjacency_list_graph();
    std::cout <<"========= Sawyer::Container::Graph =========\n";
    example_sawyer_graph();
    std::cout <<"============= Hybrid BGL-Sawyer ============\n";
    hybrid_example();
}

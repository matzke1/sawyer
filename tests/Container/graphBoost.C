// This file shows three ways to manipulate graphs:
//    1. The pure Boost Graph Library (BGL) approach.
//    2. The pure Sawyer approach
//    3. The BGL interface on top of a Sawyer graph.

#include <Sawyer/GraphBoost.h>                          // BGL interface for Sawyer::Container::Graph
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/foreach.hpp>
#include <iostream>
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



void example_adjacency_list_graph() {
    
    // Types.  Aside from these types main types, which are so pervasive and long-winded that they're often declared in as high
    // a scope as possible, this function treats each sub-example is if it could be in its own function, and declares things as
    // locally as possible.  Although it's somewhat artificial here, in practice this is actually what happens, and therefore
    // the amount of verbosity here will be close to what occurs in practice.
    typedef boost::property<vertex_data_t, VertexData, boost::property<boost::vertex_index_t, size_t> > VertexProperties;
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

    // We'll be creating a lookup table below, and we want to have O(1) lookup times. That means we need small, unique
    // ID numbers for each vertex.  There isn't a good way (i.e., fast and simple) to do this with the BGL interface when the
    // graph needs to support fast erasure of edges and/or vertices.
    boost::property_map<Graph, vertex_data_t>::type vertexData = boost::get(vertex_data_t(), graph);
    boost::property_map<Graph, boost::vertex_index_t>::type vertexIds = boost::get(boost::vertex_index_t(), graph);
    VertexDesc a = boost::add_vertex(VertexData(1.0, 1.0, "vampire"), graph);
    boost::put(vertexIds, a, boost::num_vertices(graph)-1);
    VertexDesc b = boost::add_vertex(VertexData(2.0, 1.0, "venison"), graph);
    boost::put(vertexIds, b, boost::num_vertices(graph)-1);
    VertexDesc c = boost::add_vertex(VertexData(3.1, 1.1, "vermouth"), graph);
    boost::put(vertexIds, c, boost::num_vertices(graph)-1);
    VertexDesc d = boost::add_vertex(VertexData(6.6, 1.4, "vogue"), graph);
    boost::put(vertexIds, d, boost::num_vertices(graph)-1);
    VertexDesc e = add_vertex(VertexData(8.9, 1.5, "vigil"), graph);
    boost::put(vertexIds, e, boost::num_vertices(graph)-1);

    // Add some edges. BGL returns a pair where second is always true for graphs that allow parallel edges. As with adding
    // vertices, adding edges requires three steps, although we are able to hoist the first step to the top in these examples
    // (not always convenient in real life).  The std::string c'tors are because there is no implicit conversion happening in
    // the add_edge function like what Sawyer allows.
    boost::property_map<Graph, boost::edge_name_t>::type edgeData = boost::get(boost::edge_name_t(), graph);
    EdgeDesc ab = boost::add_edge(a, b, std::string("elephant"), graph).first;
    EdgeDesc ad = boost::add_edge(a, d, std::string("echidna"), graph).first;
    EdgeDesc ca = boost::add_edge(c, a, std::string("emu"), graph).first;
    EdgeDesc dc = boost::add_edge(d, c, std::string("eagle"), graph).first;
    EdgeDesc ce = boost::add_edge(c, e, std::string("eel"), graph).first;
    EdgeDesc bd = boost::add_edge(b, d, std::string("earwig"), graph).first;
    EdgeDesc de = boost::add_edge(d, e, std::string("egret"), graph).first;

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
        typedef boost::property_map<Graph, boost::vertex_index_t>::type VertexIdMap;
        VertexIdMap vertexIdMap = boost::get(boost::vertex_index_t(), graph);
        boost::property_traits<VertexIdMap>::value_type id = boost::get(vertexIdMap, vertex);
        typedef boost::property_map<Graph, vertex_data_t>::type VertexDataMap;
        VertexDataMap vertexDataMap = boost::get(vertex_data_t(), graph);
        VertexData &vertexData = boost::get(vertexDataMap, vertex);
        std::cout <<"  [" <<id <<"]\t" <<vertexData.x <<"\t" <<vertexData.y <<"\t" <<vertexData.name <<"\n";
    }

    // Create a lookup table that contains additional information per vertex that we will not add to the graph.  BGL calls
    // these external properties.  We want O(1) lookup time for this table, which means we need to store vertex ID numbers in a
    // property of the graph.  We could have used "vecS" for our vertex container, but that trades off other performance (and
    // we'd like the potential of having O(1) erasure).
    std::vector<VertexExternalData> table(boost::num_vertices(graph), VertexExternalData());
    BOOST_FOREACH (const VertexDesc &vertex, boost::vertices(graph)) {
        typedef boost::property_map<Graph, boost::vertex_index_t>::type VertexIdMap;
        VertexIdMap vertexIdMap = boost::get(boost::vertex_index_t(), graph);
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
        typedef boost::property_map<Graph, boost::vertex_index_t>::type VertexIdMap;
        VertexIdMap vertexIdMap = boost::get(boost::vertex_index_t(), graph);
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
    typedef Graph::VertexIterator Vertex;               // a "pointer" to all information about a vertex
    typedef Graph::EdgeIterator Edge;                   // a "pointer" to all information about an edge

    // Construct an empty graph
    Graph graph;

    // Note about name lookup.  Since Sawyer uses methods rather than functions, name lookup is not an issue and the only
    // names that need qualification are the type names.  The Sawyer API looks like an object-oriented API rather than a bunch
    // of arbitrarily related functions.

    // Add some vertices. Like STL containers, Sawyer vertices are immediately associated with user-defined values which are
    // copied into the graph.
    Vertex a = graph.insertVertex(VertexData(1.0, 1.0, "vampire"));
    Vertex b = graph.insertVertex(VertexData(2.0, 1.0, "venison"));
    Vertex c = graph.insertVertex(VertexData(3.1, 1.1, "vermouth"));
    Vertex d = graph.insertVertex(VertexData(6.6, 1.4, "vogue"));
    Vertex e = graph.insertVertex(VertexData(8.9, 1.5, "vigil"));

    // Add some edges. Arguments are source and destination vertices, and user-defined value. Implicit conversion from char
    // const* to std::string is occurring here.  These return type Edge, but we don't need them.
    graph.insertEdge(a, b, "elephant");
    graph.insertEdge(a, d, "echidna");
    graph.insertEdge(c, a, "emu");
    graph.insertEdge(d, c, "eagle");
    graph.insertEdge(c, e, "eel");
    graph.insertEdge(b, d, "earwig");
    graph.insertEdge(d, e, "egret");

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

    // Print the vertex information.  If we were only interested in the user data (and not the ID) then we could have iterated
    // over graph.vertexValues(), which returns references to our data of type VertexData defined above.
    std::cout <<"vertices:\n";
    BOOST_FOREACH (const Graph::Vertex &vertexNode, graph.vertices()) {
        const VertexData &data = vertexNode.value();
        std::cout <<"  [" <<vertexNode.id() <<"]\t" <<data.x <<"\t" <<data.y <<"\t" <<data.name <<"\n";
    }

    // Create a lookup table that contains additional information per vertex that we will not add to the graph.  We want O(1)
    // lookup time for this table, which means we need to use vertex ID numbers. These ID numbers are maintained by the graph
    // object itself and are guaranteed to be sequential beginning at zero.  This time we use "node" iterators in order to have
    // access to vertex ID numbers.
    std::vector<VertexExternalData> table(graph.nVertices(), VertexExternalData());
    BOOST_FOREACH (const Graph::Vertex &vertex, graph.vertices()) {
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
    graph.eraseVertex(toRemove);                        // "erase" is for either vertices or edges depending on argument.
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

// This mess is due to not being able to partially specialize boost::property_traits for arbitrary ConstVertexPropertyMap types.
// See comments in GraphBoost.h for the disabled boost::property_traits specializations. It also precludes hybrid_example()
// from being implemented more generally as a fuction template.
typedef Sawyer::Container::Graph<VertexData, std::string> HybridExampleGraph;
typedef Sawyer::Boost::ConstVertexIdPropertyMap<const HybridExampleGraph> HybridExampleGraphVertexIdPropertyMap;

namespace boost {
    template<>
    struct property_traits<HybridExampleGraphVertexIdPropertyMap> {
        typedef size_t value_type;
        typedef const size_t &reference;
        typedef size_t key_type;                            // vertex ID number
        typedef boost::readable_property_map_tag category;
    };
} // namespace


// This example uses the BGL wrapper around a Sawyer::Container::Graph.  The idea is that we can use either API interchangeably
// and use the algorithms provided by BGL.
void hybrid_example() {
    // Use Sawyer as the graph.
    typedef HybridExampleGraph Graph;

    // Types.  Get types from boost rather than Sawyer.
    typedef boost::graph_traits<Graph>::vertex_descriptor VertexDesc;
    typedef boost::graph_traits<Graph>::edge_descriptor EdgeDesc;
    typedef boost::graph_traits<Graph>::edge_iterator EdgeIter;

    // Construct an empty graph
    Graph graph;

    // Obtain the property map for the vertex values.  This example demonstrates accessing an internal aggregate property whose
    // tag is Sawyer::Boost::vertex_value_t, a new tag introduced by <Sawyer/GraphBoost.h> for accessing the user-defined value
    // associated internally with each vertex.  The Sawyer::Boost::edge_value_t is the tag for accessing the user-defined value
    // associated with each edge.
    typedef boost::property_map<Graph, Sawyer::Boost::vertex_value_t>::type VertexValueMap;
    VertexValueMap vertexData = boost::get(Sawyer::Boost::vertex_value_t(), graph);

    // Add some vertices.  These functions need qualified names because graph's type is not in boost namespace, so Koenig
    // lookup fails.  We're mixing the BGL and Sawyer APIs here because we don't yet support BGL graph-internal properties on
    // Sawyer graphs.  Anyway, we wouldn't need the vertex ID property that we used for the pure BGL example since Sawyer
    // maintains its own ID numbers.
    VertexDesc a = boost::add_vertex(VertexData(1.0, 1.0, "vampire"), graph);
    VertexDesc b = boost::add_vertex(VertexData(2.0, 1.0, "venison"), graph);
    VertexDesc c = boost::add_vertex(VertexData(3.1, 1.1, "vermouth"), graph);
    VertexDesc d = boost::add_vertex(VertexData(6.6, 1.4, "vogue"), graph);
    VertexDesc e = boost::add_vertex(VertexData(8.9, 1.5, "vigil"), graph);

    // Add some edges. Mixing BGL and Sawyer APIs for the internal properties like we did for vertices.  We use an internal
    // property map to access the std::string stored on each edge. Unlike the pure BGL example, implicit conversion from char
    // const* to std::string works here.
    boost::property_map<Graph, Sawyer::Boost::edge_value_t>::type edgeData = boost::get(Sawyer::Boost::edge_value_t(), graph);
    boost::add_edge(a, b, "elephant", graph).first;
    boost::add_edge(a, d, "echidna", graph).first;
    boost::add_edge(c, a, "emu", graph).first;
    boost::add_edge(d, c, "eagle", graph).first;
    boost::add_edge(c, e, "eel", graph).first;
    boost::add_edge(b, d, "earwig", graph).first;
    boost::add_edge(d, e, "egret", graph).first;

    // Iterate over the edges of a graph manually and print their associated std::string values. Using the Sawyer API would
    // have been quite a bit more readable.  At least this time we know that BGL edge descriptors are also Sawyer edge IDs.
    std::cout <<"edges:\n";
    EdgeIter e_iter, e_end;
    for (boost::tie(e_iter, e_end)=boost::edges(graph); e_iter!=e_end; ++e_iter) {
        typedef boost::property_map<Graph, Sawyer::Boost::edge_value_t>::type EdgeDataMap;
        EdgeDataMap edgeDataMap = boost::get(Sawyer::Boost::edge_value_t(), graph);
        std::cout <<"  [" <<*e_iter <<"] = " <<boost::get(edgeDataMap, *e_iter) <<"\n";
    }

    // Iterate over the vertices of a graph with foreach and make each name upper case with a "!" at the end.
    BOOST_FOREACH (const VertexDesc &vertex, boost::vertices(graph)) {
        typedef boost::property_map<Graph, Sawyer::Boost::vertex_value_t>::type VertexDataMap;
        VertexDataMap vertexDataMap = boost::get(Sawyer::Boost::vertex_value_t(), graph);
        VertexData &vertexData = boost::get(vertexDataMap, vertex);
        vertexData.name = boost::to_upper_copy(vertexData.name) + "!";
    }

    // Iterate over the vertices and print the data associated with each.
    std::cout <<"vertices:\n";
    BOOST_FOREACH (const VertexDesc &vertex, boost::vertices(graph)) {
        typedef boost::property_map<Graph, Sawyer::Boost::vertex_id_t>::type VertexIdMap;
        VertexIdMap vertexIdMap = boost::get(Sawyer::Boost::vertex_id_t(), graph);
        boost::property_traits<VertexIdMap>::value_type id = boost::get(vertexIdMap, vertex);
        typedef boost::property_map<Graph, Sawyer::Boost::vertex_value_t>::type VertexDataMap;
        VertexDataMap vertexDataMap = boost::get(Sawyer::Boost::vertex_value_t(), graph);
        VertexData &vertexData = boost::get(vertexDataMap, vertex);
        std::cout <<"  [" <<id <<"]\t" <<vertexData.x <<"\t" <<vertexData.y <<"\t" <<vertexData.name <<"\n";
    }

    // Create a lookup table that contains additional information per vertex that we will not add to the graph.  BGL calls
    // these external properties.
    std::vector<VertexExternalData> table(boost::num_vertices(graph), VertexExternalData());
    BOOST_FOREACH (const VertexDesc &vertex, boost::vertices(graph)) {
        typedef boost::property_map<Graph, Sawyer::Boost::vertex_id_t>::type VertexIdMap;
        VertexIdMap vertexIdMap = boost::get(Sawyer::Boost::vertex_id_t(), graph);
        typedef boost::property_traits<VertexIdMap>::value_type VertexId;
        VertexId id = boost::get(vertexIdMap, vertex);
        assert(id < table.size());
        typedef boost::property_map<Graph, Sawyer::Boost::vertex_value_t>::type VertexDataMap;
        VertexDataMap vertexDataMap = boost::get(Sawyer::Boost::vertex_value_t(), graph);
        const VertexData &vertexData = boost::get(vertexDataMap, vertex);
        table[id].r = sin(vertexData.x);
        table[id].g = cos(vertexData.y);
        table[id].b = 0.5;
        table[id].name = boost::to_lower_copy(vertexData.name);
    }
    std::cout <<"table:\n";
    for (size_t i=0; i<table.size(); ++i)
        std::cout <<"  " <<table[i].r <<"\t" <<table[i].g <<"\t" <<table[i].b <<"\t" <<table[i].name <<"\n";

    // Erase a vertex from the graph. Fortunately Sawyer keeps track of ID numbers for us, so we don't need to jump through all
    // the hoops we had to in the pure BGL example.
    VertexDesc toRemove = c;
    {
        // Get ID number that we're removing (BGL vertex_descriptor is a Sawyer ID for this graph type)
        size_t removedId = toRemove;

        // Erase the vertex.  BGL requires us to first erase the incident edges.
        boost::clear_vertex(toRemove, graph);           // optional since underlying graph is a Sawyer graph
        boost::remove_vertex(toRemove, graph);

        // Move members of the table. This time we can use an O(1) swap corresponding to Sawyer's renumbering policy.
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

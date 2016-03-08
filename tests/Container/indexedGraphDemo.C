// Demonstrate some ways to use a Sawyer::Container::Graph with vertex and edge indexes

#include <boost/lexical_cast.hpp>
#include <Sawyer/Graph.h>
#include <string>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Demo 1: A graph whose vertices are unique std::string city names and whose edges are the time in hours it takes to travel
// from the source to the target city by train, including layovers.
//
// We want the vertices (cities) to be indexed so we don't have to keep track of them ourselves. I.e., we want to be able to
// find the vertex that represents a city when all we know is the city name.  We also want to get an error if we try to insert
// the same city twice.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// First, we need data types for what we'll be storing at each vertex and each edge. Sawyer's graph implementation divides
// concerns between the Sawyer library and the user in a manner very similar to STL containers. Take std::list for example: the
// STL is reponsible for managing the notions of vertices and the linear connectivity between vertices, while the user is
// responsible for his data stored at each node. Similarly, Sawyer's Graph type is responsible for managing the vertices and
// the connectivity between vertices (i.e., edges), while the user is responsible only for his data stored at each vertex
// and/or edge.
typedef std::string TrainCityName;                      // User's data stored at each vertex
typedef double TrainTravelTime;                         // User's data stored at each edge

// An indexed Graph needs to know what part of the user's node and/or edge data to use as the lookup keys. In this example, the
// lookup keys for the vertices are exactly the strings we stored there, and we don't need any index for edges.  Subsequent
// examples will expand on this.
typedef TrainCityName TrainVertexKey;

// Create the indexed graph. This is done by providing the 3rd and/or 4th template arguments: the types for the vertex and edge
// lookup keys. The defaults for these arguments are to not have an index; having an index might substantially slow down vertex
// and edge inserting and erasing.
typedef Sawyer::Container::GraphEdgeNoKey<TrainTravelTime> EdgeKey;
typedef Sawyer::Container::Graph<TrainCityName, TrainTravelTime, TrainVertexKey, EdgeKey> TrainGraph;

static void
demo1() {
    TrainGraph g;

    // Insert a few cities. Sawyer uses iterators also for pointers, just like an "int*" can point into a C array of integers
    // and also be used to iterate over that array. Incrementing these particular three iterators probably isn't too useful,
    // just as incrementing an "int*" to an array representation of a lattice isn't too useful.
    TrainGraph::VertexIterator albuquerque = g.insertVertex("Albuquerque");
    TrainGraph::VertexIterator boston = g.insertVertex("Boston");
    TrainGraph::VertexIterator chicago = g.insertVertex("Chicago");
    ASSERT_always_require(g.nVertices() == 3);

    // Since the vertices are indexed they must be unique. Trying to add another vertex with the same label will throw an
    // exception.
    try {
        g.insertVertex("Boston");
        ASSERT_not_reachable("insertion should have failed");
    } catch (Sawyer::Exception::AlreadyExists &e) {
    }

    // Due to the index, we can find a vertex given its value.  This is O(log|V|).
    TrainGraph::VertexIterator c2 = g.findVertexValue("Chicago");
    ASSERT_always_require(c2 == chicago);

    // We can also insert a vertex conditionally since we have an index.
    TrainGraph::VertexIterator denver = g.insertVertexMaybe("Denver"); // inserted since it doesn't exist yet
    TrainGraph::VertexIterator d2 = g.insertVertexMaybe("Denver");     // returns an existing vertex
    ASSERT_always_require(d2 == denver);

    // Each vertex stores its label which you can get in constant time.
    ASSERT_always_require(chicago->value() == "Chicago");

    // Lets add some edges
    TrainGraph::EdgeIterator ab = g.insertEdge(albuquerque, boston, 58); // 58 hours, etc.
    TrainGraph::EdgeIterator ba = g.insertEdge(boston, albuquerque, 54);
    TrainGraph::EdgeIterator ac = g.insertEdge(albuquerque, chicago, 27);
    TrainGraph::EdgeIterator ca = g.insertEdge(chicago, albuquerque, 24);
    TrainGraph::EdgeIterator bc = g.insertEdge(boston, chicago, 23);
    TrainGraph::EdgeIterator cb = g.insertEdge(chicago, boston, 25);
    ASSERT_always_require(g.nEdges() == 6);

    // We can add vertices at the same time if they don't exist.
    TrainGraph::EdgeIterator ce = g.insertEdgeWithVertices("Chicago", "Effingham", 6 /*hours*/);
    ASSERT_always_require(g.nVertices() == 5);
    ASSERT_always_require(g.nEdges() == 7);
    TrainGraph::VertexIterator effingham = ce->target();
    ASSERT_always_require(effingham->value() == "Effingham");

    // Similarly, we can have the graph automatically erase a vertex when we erase that vertex's last edge. Unfortunately this
    // leaves the effingham pointer dangling, which might result in a segfault if we try to dereference it.
    g.eraseEdgeWithVertices(ce);
    ASSERT_always_require(g.nVertices() == 4);
    ASSERT_always_require(g.nEdges() == 6);
    ASSERT_always_forbid(g.isValidVertex(g.findVertexValue("Effingham")));

    // Since edges are not indexed in our graph, adding the same edge twice results in two parallel edges with the same value.
    TrainGraph::EdgeIterator ab2 = g.insertEdge(albuquerque, boston, 58);
    ASSERT_always_require(g.nEdges() == 7);
    ASSERT_always_require(ab != ab2);
    g.eraseEdge(ab2);                                   // let's erase it
    ab2 = g.edges().end();                              // good programming practice to avoid dangling pointers
    ASSERT_always_require(g.nEdges() == 6);

    // We can erase a vertex, which also erases the incident edges.
    g.eraseVertex(chicago);
    effingham = g.vertices().end();
    ASSERT_always_require(g.nVertices() == 3);          // Albuquerque, Boston, Denver
    ASSERT_always_require(g.nEdges() == 2);             // Albuquerque->Boston and Boston->Albuquerque
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Demo 2: This demo builds on the previous demo by defining a graph that stores multiple things at each vertex, two of which
// are used as the key. The edges represent layovers at airports.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Each vertex stores a Flight
struct Flight {
    std::string airline;
    int number;
    double cost;
    // pretend there's lots more...

    Flight(const std::string airline, int number, double cost = 0.0)
        : airline(airline), number(number), cost(cost) {}
};

// The FlightKey is used to index the vertices. It needs the following functionality:
//    1. A default constructor
//    2. A copy constructor
//    3. Construction from a vertex value (Flight)
//    4. Comparison (operator <)
//
// In the previous demo, where the user's vertex data and key were both std::string, it happens that std::string satisfies all
// these requirements. In this Flight demo, we could have done something similar by making Flight both the vertex type and the
// key type and adding an operator<. The reason that's not a good idea is that it would make the key much heavier than it needs
// to be -- we'd end up storing two copies of each vertex's data: one in the vertex and another in the vertex index, when all
// we really need to store in the index is a short string. This also demonstrates that the key can be constructed on the fly
// from the vertex's value (as long as the same key is always generated for the same value).
class FlightKey {
    std::string key_;
public:
    FlightKey() {}

    // Extract the key from the flight
    explicit FlightKey(const Flight &flight) {
        key_ = flight.airline + " " + boost::lexical_cast<std::string>(flight.number);
    }

    bool operator<(const FlightKey &other) const {
        return key_ < other.key_;
    }
};

// The graph edges store the time in minutes for a layover between two flights. We don't index the edges.
struct Layover {
    std::string airportCode;
    unsigned minutes;

    Layover(const std::string code, unsigned minutes)
        : airportCode(code), minutes(minutes) {}
};

// Create the graph with vertex indexing.
typedef Sawyer::Container::Graph<Flight, Layover, FlightKey> FlightGraph;

static void
demo2() {
    FlightGraph g;

    // Insert a couple flights as vertices
    FlightGraph::VertexIterator ua9934 = g.insertVertex(Flight("United", 9934, 187.00));
    FlightGraph::VertexIterator d1540 = g.insertVertex(Flight("Delta", 1540, 218.00));
    ASSERT_always_require(g.nVertices() == 2);

    // We can look up a vertex with given flight information. Since the cost isn't part of the lookup key, we can provide
    // anything we want.
    FlightGraph::VertexIterator v1 = g.findVertexValue(Flight("United", 9934));
    ASSERT_always_require(v1 == ua9934);

    // We can add a layover between two flights
    FlightGraph::EdgeIterator lay1 = g.insertEdge(ua9934, d1540, Layover("ABQ", 120));
    ASSERT_always_require(g.nEdges() == 1);

    // We can add a layover while populating the flights at the same time.
    FlightGraph::EdgeIterator lay2 = g.insertEdgeWithVertices(Flight("Alaska", 123, 99.00),
                                                              Flight("Alaska", 456, 99.00),
                                                              Layover("DIA", 60));
    ASSERT_always_require(g.nVertices() == 4);
    ASSERT_always_require(g.nEdges() == 2);

    // We're prevented from adding the same flight twice even if it has a different cost.
    try {
        g.insertVertex(Flight("United", 9934, 340.52));
        ASSERT_not_reachable("duplicate vertex insertion should have failed");
    } catch (const Sawyer::Exception::AlreadyExists&) {
        ASSERT_always_require(g.nVertices() == 4);
    }

    // We can remove a layover along with removing flights that no longer have edges.
    g.eraseEdgeWithVertices(lay2);
    ASSERT_always_require(g.nVertices() == 2);
    ASSERT_always_require(g.nEdges() == 1);

    // We can remove an edge without removing vertices
    g.eraseEdge(lay1);
    ASSERT_always_require(g.nVertices() == 2);
    ASSERT_always_require(g.nEdges() == 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Demo 3: Using a different index type.  By default, a graph uses a map based on a balanced binary tree and lookup times are
// guaranteed to be O(log N).  We can declare our own index type if we want to use something like a hash-based lookup.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Declare our vertex values and keys. In this case we'll keep things trivial by storing only strings and using the string
// itself as the vertex lookup key.
typedef std::string Demo3VertexValue;
typedef std::string Demo3VertexKey;

// Declare the index type and define its required operations.  E.g., you could use a pair of std::map's or a pair of hash-based
// maps such as boost::unordered_map or C++11's std::unordered_map.
template<class ConstVertexIterator>
class Demo3VertexIndex {
    typedef Sawyer::Container::BiMap<Demo3VertexKey, ConstVertexIterator> Map;
    Map map_;

public:
    void clear() {
        map_.clear();
    }

    void insert(const Demo3VertexKey &key, const ConstVertexIterator &iter) {
        map_.insert(key, iter);
    }

    void eraseTarget(const ConstVertexIterator &iter) {
        map_.eraseTarget(iter);
    }

    Sawyer::Optional<ConstVertexIterator> forwardLookup(const Demo3VertexKey &key) const {
        return map_.forward().getOptional(key);
    }

    Sawyer::Optional<Demo3VertexKey> reverseLookup(const ConstVertexIterator &iter) {
        return map_.reverse().getOptional(iter);
    }
};

// Use this convenience macro to partially specialize Sawyer::Container::GraphIndexTraits. This has to be at global scope and
// the index is expected to take one template argument. If this doesn't meet your needs then take a look at the macro
// definition (it's very small and simple).
SAWYER_GRAPH_INDEXING_SCHEME(Demo3VertexKey, Demo3VertexIndex);

// Now do some things with this graph
static void
demo3() {
    typedef Sawyer::Container::Graph<Demo3VertexValue, Sawyer::Nothing, Demo3VertexKey> G;
    G g;

    g.insertVertex("vertex 1");
    try {
        g.insertVertex("vertex 1");
        ASSERT_not_reachable("inserting the same vertex label twice should have failed");
    } catch (const Sawyer::Exception::AlreadyExists&) {
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int
main() {
    demo1();
    demo2();
    demo3();
}


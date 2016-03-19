// Algorithms for Sawyer::Container::Graph

#ifndef Sawyer_GraphAlgorithm_H
#define Sawyer_GraphAlgorithm_H

#include <Sawyer/Sawyer.h>
#include <Sawyer/DenseIntegerSet.h>
#include <Sawyer/GraphTraversal.h>
#include <Sawyer/Message.h>
#include <Sawyer/Set.h>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <set>
#include <vector>

namespace Sawyer {
namespace Container {
namespace Algorithm {

/** Determines if the any edges of a graph form a cycle.
 *
 *  Returns true if any cycle is found, false if the graph contains no cycles. */
template<class Graph>
bool
graphContainsCycle(const Graph &g) {
    std::vector<bool> visited(g.nVertices(), false);    // have we seen this vertex already?
    std::vector<bool> onPath(g.nVertices(), false);     // is a vertex on the current path of edges?
    for (size_t rootId = 0; rootId < g.nVertices(); ++rootId) {
        if (visited[rootId])
            continue;
        visited[rootId] = true;
        ASSERT_require(!onPath[rootId]);
        onPath[rootId] = true;
        for (DepthFirstForwardGraphTraversal<const Graph> t(g, g.findVertex(rootId), ENTER_EDGE|LEAVE_EDGE); t; ++t) {
            size_t targetId = t.edge()->target()->id();
            if (t.event() == ENTER_EDGE) {
                if (onPath[targetId])
                    return true;                        // must be a back edge forming a cycle
                onPath[targetId] = true;
                if (visited[targetId]) {
                    t.skipChildren();
                } else {
                    visited[targetId] = true;
                }
            } else {
                ASSERT_require(t.event() == LEAVE_EDGE);
                ASSERT_require(onPath[targetId]);
                onPath[targetId] = false;
            }
        }
        ASSERT_require(onPath[rootId]);
        onPath[rootId] = false;
    }
    return false;
}

/** Break cycles of a graph arbitrarily.
 *
 *  Modifies the argument in place to remove edges that cause cycles.  Edges are not removed in any particular order.  Returns
 *  the number of edges that were removed. */
template<class Graph>
size_t
graphBreakCycles(Graph &g) {
    std::vector<bool> visited(g.nVertices(), false);    // have we seen this vertex already?
    std::vector<unsigned char> onPath(g.nVertices(), false);  // is a vertex on the current path of edges? 0, 1, or 2
    std::set<typename Graph::ConstEdgeIterator> edgesToErase;

    for (size_t rootId = 0; rootId < g.nVertices(); ++rootId) {
        if (visited[rootId])
            continue;
        visited[rootId] = true;
        ASSERT_require(!onPath[rootId]);
        onPath[rootId] = true;
        for (DepthFirstForwardGraphTraversal<const Graph> t(g, g.findVertex(rootId), ENTER_EDGE|LEAVE_EDGE); t; ++t) {
            size_t targetId = t.edge()->target()->id();
            if (t.event() == ENTER_EDGE) {
                if (onPath[targetId]) {
                    edgesToErase.insert(t.edge());
                    t.skipChildren();
                }
                ++onPath[targetId];
                if (visited[targetId]) {
                    t.skipChildren();
                } else {
                    visited[targetId] = true;
                }
            } else {
                ASSERT_require(t.event() == LEAVE_EDGE);
                ASSERT_require(onPath[targetId]);
                --onPath[targetId];
            }
        }
        ASSERT_require(onPath[rootId]==1);
        onPath[rootId] = 0;
    }

    BOOST_FOREACH (const typename Graph::ConstEdgeIterator &edge, edgesToErase)
        g.eraseEdge(edge);
    return edgesToErase.size();
}

/** Test whether a graph is connected.
 *
 *  Returns true if a graph is connected and false if not. This is a special case of findConnectedComponents but is faster for
 *  graphs that are not connected since this algorithm only needs to find one connected component instead of all connected
 *  components.
 *
 *  Time complexity is O(|V|+|E|).
 *
 *  @sa graphFindConnectedComponents. */
template<class Graph>
bool
graphIsConnected(const Graph &g) {
    if (g.isEmpty())
        return true;
    std::vector<bool> seen(g.nVertices(), false);
    size_t nSeen = 0;
    DenseIntegerSet<size_t> worklist(g.nVertices());
    worklist.insert(0);
    while (!worklist.isEmpty()) {
        size_t id = *worklist.values().begin();
        worklist.erase(id);

        if (seen[id])
            continue;
        seen[id] = true;
        ++nSeen;

        typename Graph::ConstVertexIterator v = g.findVertex(id);
        BOOST_FOREACH (const typename Graph::Edge &e, v->outEdges()) {
            if (!seen[e.target()->id()])                // not necessary, but saves some work
                worklist.insert(e.target()->id());
        }
        BOOST_FOREACH (const typename Graph::Edge &e, v->inEdges()) {
            if (!seen[e.source()->id()])                // not necessary, but saves some work
                worklist.insert(e.source()->id());
        }
    }
    return nSeen == g.nVertices();
}

/** Find all connected components of a graph.
 *
 *  Finds all connected components of a graph and numbers them starting at zero. The provided vector is initialized to hold the
 *  results with the vector serving as a map from vertex ID number to connected component number.  Returns the number of
 *  conencted components.
 *
 *  Time complexity is O(|V|+|E|).
 *
 *  @sa @ref graphIsConnected. */
template<class Graph>
size_t
graphFindConnectedComponents(const Graph &g, std::vector<size_t> &components /*out*/) {
    static const size_t NOT_SEEN(-1);
    size_t nComponents = 0;
    components.clear();
    components.resize(g.nVertices(), NOT_SEEN);
    DenseIntegerSet<size_t> worklist(g.nVertices());
    for (size_t rootId = 0; rootId < g.nVertices(); ++rootId) {
        if (components[rootId] != NOT_SEEN)
            continue;
        ASSERT_require(worklist.isEmpty());
        worklist.insert(rootId);
        while (!worklist.isEmpty()) {
            size_t id = *worklist.values().begin();
            worklist.erase(id);

            ASSERT_require(components[id]==NOT_SEEN || components[id]==nComponents);
            if (components[id] != NOT_SEEN)
                continue;
            components[id] = nComponents;

            typename Graph::ConstVertexIterator v = g.findVertex(id);
            BOOST_FOREACH (const typename Graph::Edge &e, v->outEdges()) {
                if (components[e.target()->id()] == NOT_SEEN) // not necessary, but saves some work
                    worklist.insert(e.target()->id());
            }
            BOOST_FOREACH (const typename Graph::Edge &e, v->inEdges()) {
                if (components[e.source()->id()] == NOT_SEEN) // not necesssary, but saves some work
                    worklist.insert(e.source()->id());
            }
        }
        ++nComponents;
    }
    return nComponents;
}

/** Create a subgraph.
 *
 *  Creates a new graph by copying an existing graph, but copying only those vertices whose ID numbers are specified.  All
 *  edges between the specified vertices are copied.  The @p vertexIdVector should have vertex IDs that are part of graph @p g
 *  and no ID number should occur more than once in that vector.
 *
 *  The ID numbers of the vertices in the returned subgraph are equal to the corresponding index into the @p vertexIdVector for
 *  the super-graph.
 *
 * @{ */
template<class Graph>
Graph
graphCopySubgraph(const Graph &g, const std::vector<size_t> &vertexIdVector) {
    Graph retval;

    // Insert vertices
    typedef typename Graph::ConstVertexIterator VIter;
    typedef Map<size_t, VIter> Id2Vertex;
    Id2Vertex resultVertices;
    for (size_t i=0; i<vertexIdVector.size(); ++i) {
        ASSERT_forbid2(resultVertices.exists(vertexIdVector[i]), "duplicate vertices not allowed");
        VIter rv = retval.insertVertex(g.findVertex(vertexIdVector[i])->value());
        ASSERT_require(rv->id() == i);                  // some analyses depend on this
        resultVertices.insert(vertexIdVector[i], rv);
    }

    // Insert edges
    for (size_t i=0; i<vertexIdVector.size(); ++i) {
        typename Graph::ConstVertexIterator gSource = g.findVertex(vertexIdVector[i]);
        typename Graph::ConstVertexIterator rSource = resultVertices[vertexIdVector[i]];
        BOOST_FOREACH (const typename Graph::Edge &e, gSource->outEdges()) {
            typename Graph::ConstVertexIterator rTarget = retval.vertices().end();
            if (resultVertices.getOptional(e.target()->id()).assignTo(rTarget))
                retval.insertEdge(rSource, rTarget, e.value());
        }
    }
    return retval;
}
/** @} */


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common subgraph isomorphism (CSI)
// Loosely based on the algorithm presented by Evgeny B. Krissinel and Kim Henrick
// "Common subgraph isomorphism detection by backtracking search"
// European Bioinformatics Institute, Genome Campus, Hinxton, Cambridge CB10 1SD, UK
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Graph>
class CsiDefaultVertexEquivalence {
public:
    bool operator()(const Graph &g1, const typename Graph::ConstVertexIterator &v1,
                    const Graph &g2, const typename Graph::ConstVertexIterator &v2) const {
        return true;
    }
};

template<class Graph>
class CsiDefaultEdgeEquivalence {
public:
    bool operator()(const Graph &g1, typename Graph::ConstVertexIterator i1, typename Graph::ConstVertexIterator i2,
                    const std::vector<typename Graph::ConstEdgeIterator> &edges1,
                    const Graph &g2, typename Graph::ConstVertexIterator j1, typename Graph::ConstVertexIterator j2,
                    const std::vector<typename Graph::ConstEdgeIterator> &edges2) const {
        return true;
    }
};

template<class Graph>
class CsiShowSolution {
    size_t n;
public:
    CsiShowSolution(): n(0) {}

    void operator()(const Graph &g1, const std::vector<size_t> &g1VertIds,
                    const Graph &g2, const std::vector<size_t> &g2VertIds) {
        ASSERT_require(g1VertIds.size() == g2VertIds.size());
        std::cerr <<"Common subgraph isomorphism solution #" <<n <<" found:\n"
                  <<"  x = [";
        BOOST_FOREACH (size_t i, g1VertIds)
            std::cerr <<" " <<i;
        std::cerr <<" ]\n"
                  <<"  y = [";
        BOOST_FOREACH (size_t j, g2VertIds)
            std::cerr <<" " <<j;
        std::cerr <<" ]\n";
        ++n;
    }
};

template<class Graph,
         class SolutionProcessor = CsiShowSolution<Graph>,
         class VertexEquivalenceP = CsiDefaultVertexEquivalence<Graph>,
         class EdgeEquivalenceP = CsiDefaultEdgeEquivalence<Graph> >
class CommonSubgraphIsomorphism {
    const Graph &g1, &g2;                               // the two whole graphs being compared
    DenseIntegerSet<size_t> v, w;                       // available vertices of g1 and g2, respectively
    std::vector<size_t> x, y;                           // selected vertices of g1 and g2, which defines vertex mapping
    mutable Message::Stream debug;                      // debugging output
    DenseIntegerSet<size_t> vNotX;                      // X erased from V

    SolutionProcessor solutionProcessor_;               // functor to call for each solution
    VertexEquivalenceP vertexEquivalenceP_;             // predicate to determine if two vertices can be equivalent
    EdgeEquivalenceP edgeEquivalenceP_;                 // predicate determines vertex equivalence given sets of edges
    size_t minAllowedSolution_;                         // size of smallest permitted solutions

    class Vam {                                         // Vertex Availability Map
        typedef std::vector<size_t> TargetVertices;     // target vertices in no particular order
        typedef std::vector<TargetVertices> Map;        // map from source vertex to available target vertices
        Map map_;
    public:
        // Set to initial emtpy state
        void clear() {
            map_.clear();
        }

        // Predicate to determine whether vam is empty. Since we never remove items from the VAM and we only add rows if we're
        // adding a column, this is equivalent to checking whether the map has any rows.
        bool isEmpty() const {
            return map_.empty();
        }

        // Insert the pair (i,j) into the mapping. Assumes this pair isn't already present.
        void insert(size_t i, size_t j) {
            if (i >= map_.size())
                map_.resize(i+1);
            map_[i].push_back(j);
        }

        // Given a vertex i in G1, return the number of vertices j in G2 where i and j can be equivalent.
        size_t size(size_t i) const {
            return i < map_.size() ? map_[i].size() : size_t(0);
        }

        // Given a vertex i in G1, return those vertices j in G2 where i and j can be equivalent.
        const std::vector<size_t>& get(size_t i) const {
            static const std::vector<size_t> empty;
            return i < map_.size() ? map_[i] : empty;
        }

        // Print for debugging
        void print(std::ostream &out, const std::string &prefix = "vam") const {
            if (isEmpty()) {
                out <<prefix <<" is empty\n";
            } else {
                for (size_t i=0; i<map_.size(); ++i) {
                    if (!map_[i].empty()) {
                        out <<prefix <<"[" <<i <<"] -> {";
                        BOOST_FOREACH (size_t j, map_[i])
                            out <<" " <<j;
                        out <<" }\n";
                    }
                }
            }
        }
    };

public:
    CommonSubgraphIsomorphism(const Graph &g1, const Graph &g2,
                              Message::Stream debug = Message::mlog[Message::DEBUG],
                              SolutionProcessor solutionProcessor = SolutionProcessor(), 
                              VertexEquivalenceP vertexEquivalenceP = VertexEquivalenceP(),
                              EdgeEquivalenceP edgeEquivalenceP = EdgeEquivalenceP())
        : g1(g1), g2(g2), v(g1.nVertices()), w(g2.nVertices()), debug(debug), vNotX(g1.nVertices()),
          solutionProcessor_(solutionProcessor), vertexEquivalenceP_(vertexEquivalenceP),
          edgeEquivalenceP_(edgeEquivalenceP), minAllowedSolution_(1) {}

private:
    CommonSubgraphIsomorphism(const CommonSubgraphIsomorphism&) {
        ASSERT_not_reachable("copy constructor makes no sense");
    }

    CommonSubgraphIsomorphism& operator=(const CommonSubgraphIsomorphism&) {
        ASSERT_not_reachable("assignment operator makes no sense");
    }

public:
    void run() {
        v.insertAll();
        w.insertAll();
        x.clear();
        y.clear();
        vNotX.insertAll();
        Vam vam;                                        // this is the only per-recursion local state
        initializeVam(vam);
        recurse(vam);
    }

    size_t minAllowedSolutionSize() const { return minAllowedSolution_; }
    void minAllowedSolutionSize(size_t n) { minAllowedSolution_ = n; }

    SolutionProcessor& solutionProcessor() { return solutionProcessor_; }
    const SolutionProcessor& solutionProcessor() const { return solutionProcessor_; }

private:
    // Print contents of a container
    template<class ForwardIterator>
    void printContainer(std::ostream &out, const std::string &prefix, ForwardIterator begin, const ForwardIterator &end,
                        const char *ends = "[]", bool doSort = false) const {
        ASSERT_require(ends && 2 == strlen(ends));
        if (doSort) {
            std::vector<size_t> sorted(begin, end);
            std::sort(sorted.begin(), sorted.end());
            printContainer(out, prefix, sorted.begin(), sorted.end(), ends, false);
        } else {
            out <<prefix <<ends[0];
            while (begin != end) {
                out <<" " <<*begin;
                ++begin;
            }
            out <<" " <<ends[1] <<"\n";
        }
    }

    template<class ForwardIterator>
    void printContainer(std::ostream &out, const std::string &prefix, const boost::iterator_range<ForwardIterator> &range,
                        const char *ends = "[]", bool doSort = false) const {
        printContainer(out, prefix, range.begin(), range.end(), ends, doSort);
    }

    // Initialize VAM so to indicate which source vertices (v of g1) map to which target vertices (w of g2) based only on the
    // vertex comparator.  We also handle self edges here.
    void initializeVam(Vam &vam) const {
        vam.clear();
        BOOST_FOREACH (size_t i, v.values()) {
            typename Graph::ConstVertexIterator v1 = g1.findVertex(i);
            BOOST_FOREACH (size_t j, w.values()) {
                typename Graph::ConstVertexIterator w1 = g2.findVertex(j);
                std::vector<typename Graph::ConstEdgeIterator> selfEdges1, selfEdges2;
                findEdges(g1, i, i, selfEdges1 /*out*/);
                findEdges(g2, j, j, selfEdges2 /*out*/);
                if (selfEdges1.size() == selfEdges2.size() &&
                    vertexEquivalenceP_(g1, g1.findVertex(i), g2, g2.findVertex(j)) &&
                    edgeEquivalenceP_(g1, v1, v1, selfEdges1, g2, w1, w1, selfEdges2))
                    vam.insert(i, j);
            }
        }
    }

    // Can the solution (stored in X and Y) be extended by adding another (i,j) pair of vertices where i is an element of the
    // set of available vertices of graph1 (vNotX) and j is a vertex of graph2 that is equivalent to i according to the
    // user-provided vertex equivalence predicate. Furthermore, is it even possible to find a solution along this branch of
    // discover which is large enough for the user? By eliminating entire branch of the search space we can drastically
    // decrease the time it takes to search, and the larger the required solution the more branches we can eliminate.
    bool isSolutionPossible(const Vam &vam) const {
        size_t largestPossibleSolution = x.size();
        BOOST_FOREACH (size_t i, vNotX.values()) {
            if (vam.size(i) > 0) {
                ++largestPossibleSolution;
                if (largestPossibleSolution >= minAllowedSolution_)
                    return true;
            }
        }
        return false;
    }

    // Choose some vertex i from G1 which is still available (i.e., i is a member of vNotX) and for which there is a vertex
    // equivalence of i with some available j from G2 (i.e., the pair (i,j) is present in the VAM).  The recursion terminates
    // quickest if we return the row of the VAM that has the fewest vertices in G2.
    size_t pickVertex(const Vam &vam) const {
        // FIXME[Robb Matzke 2016-03-19]: Perhaps this can be made even faster. The step that initializes the VAM
        // (initializeVam or refine) might be able to compute and store the shortest row so we can retrieve it here in constant
        // time.
        size_t shortestRowLength(-1), retval(-1);
        BOOST_FOREACH (size_t i, vNotX.values()) {
            size_t vs = vam.size(i);
            if (vs > 0 && vs < shortestRowLength) {
                shortestRowLength = vs;
                retval = i;
            }
        }
        ASSERT_require2(retval != size_t(-1), "cannot be reached if isSolutionPossible returned true");
        return retval;
    }

    // Extend the current solution by adding vertex i from G1 and vertex j from G2. The VAM should be adjusted separately.
    void extendSolution(size_t i, size_t j) {
        SAWYER_MESG(debug) <<"  extending solution with (i,j) = (" <<i <<"," <<j <<")\n";
        ASSERT_require(x.size() == y.size());
        ASSERT_require(std::find(x.begin(), x.end(), i) == x.end());
        ASSERT_require(std::find(y.begin(), y.end(), j) == y.end());
        ASSERT_require(vNotX.exists(i));
        x.push_back(i);
        y.push_back(j);
        vNotX.erase(i);
    }

    // Remove the last vertex mapping from a solution. The VAM should be adjusted separately.
    void retractSolution() {
        ASSERT_require(x.size() == y.size());
        ASSERT_require(!x.empty());
        size_t i = x.back();
        ASSERT_forbid(vNotX.exists(i));
        x.pop_back();
        y.pop_back();
        vNotX.insert(i);
    }
    
    // Find all edges that have the specified source and target vertices.  This is usually zero or one edge, but can be more if
    // the graph contains parallel edges.
    void
    findEdges(const Graph &g, size_t sourceVertex, size_t targetVertex,
              std::vector<typename Graph::ConstEdgeIterator> &result /*in,out*/) const {
        BOOST_FOREACH (const typename Graph::Edge &candidate, g.findVertex(sourceVertex)->outEdges()) {
            if (candidate.target()->id() == targetVertex)
                result.push_back(g.findEdge(candidate.id()));
        }
    }

    // Given that we just extended a solution by adding the vertex pair (i, j), decide whether there's a
    // possible equivalence between vertex iUnused of G1 and vertex jUnused of G2 based on the edge(s) between i and iUnused
    // and between j and jUnused.
    bool edgesAreSuitable(size_t i, size_t iUnused, size_t j, size_t jUnused) const {
        ASSERT_require(i != iUnused);
        ASSERT_require(j != jUnused);

        std::vector<typename Graph::ConstEdgeIterator> edges1, edges2;
        findEdges(g1, i, iUnused, edges1 /*out*/);
        findEdges(g2, j, jUnused, edges2 /*out*/);
        if (edges1.size() != edges2.size())
            return false;
        findEdges(g1, iUnused, i, edges1 /*out*/);
        findEdges(g2, jUnused, j, edges2 /*out*/);
        if (edges1.size() != edges2.size())
            return false;

        if (edges1.empty() && edges2.empty())
            return true;

        typename Graph::ConstVertexIterator v1 = g1.findVertex(i), v2 = g1.findVertex(iUnused);
        typename Graph::ConstVertexIterator w1 = g2.findVertex(j), w2 = g2.findVertex(jUnused);
        return edgeEquivalenceP_(g1, v1, v2, edges1, g2, w1, w2, edges2);
    }

    // Create a new VAM from an existing one. The (i,j) pairs of the new VAM will form a subset of the specified VAM.
    Vam refine(const Vam &vam) const {
        Vam refined;
        BOOST_FOREACH (size_t i, vNotX.values()) {
            BOOST_FOREACH (size_t j, vam.get(i)) {
                if (j != y.back()) {
                    SAWYER_MESG(debug) <<"  refining with edges " <<x.back() <<" <--> " <<i <<" in G1"
                                       <<" and " <<y.back() <<" <--> " <<j <<" in G2";
                    if (edgesAreSuitable(x.back(), i, y.back(), j)) {
                        SAWYER_MESG(debug) <<": inserting (" <<i <<", " <<j <<")\n";
                        refined.insert(i, j);
                    } else {
                        SAWYER_MESG(debug) <<": non-equivalent edges\n";
                    }
                }
            }
        }
        refined.print(debug, "  refined");
        return refined;
    }

    // Show some debugging inforamtion
    void showState(const Vam &vam, const std::string &what, size_t level) const {
        debug <<what <<" " <<level <<":\n";
        printContainer(debug, "  v - x = ", vNotX.values(), "{}", true);
        printContainer(debug, "  x = ", x.begin(), x.end());
        printContainer(debug, "  y = ", y.begin(), y.end());
        vam.print(debug, "  vam");
    }
            
    void recurse(const Vam &vam, size_t level = 0) {
        if (debug)
            showState(vam, "entering state", level);        // debugging
        if (isSolutionPossible(vam)) {
            size_t i = pickVertex(vam);
            SAWYER_MESG(debug) <<"  picked i = " <<i <<"\n";
            std::vector<size_t> jCandidates = vam.get(i);
            BOOST_FOREACH (size_t j, jCandidates) {
                extendSolution(i, j);
                Vam refined = refine(vam);
                recurse(refined, level+1);
                retractSolution();
                if (debug)
                    showState(vam, "back to state", level);
            }

            // Try again after removing vertex i from consideration
            SAWYER_MESG(debug) <<"  removing i=" <<i <<" from consideration\n";
            v.erase(i);
            ASSERT_require(vNotX.exists(i));
            vNotX.erase(i);
            recurse(vam, level+1);
            v.insert(i);
            vNotX.insert(i);
            if (debug)
                showState(vam, "restored i=" + boost::lexical_cast<std::string>(i) + " back to state", level);
        } else if (x.size() >= minAllowedSolution_) {
            ASSERT_require(x.size() == y.size());
            if (debug) {
                printContainer(debug, "  found soln x = ", x.begin(), x.end());
                printContainer(debug, "  found soln y = ", y.begin(), y.end());
            }
            solutionProcessor_(g1, x, g2, y);
        }
    }
};

                
            
#if 0 // [Robb Matzke 2016-03-17]
template<class Graph>
class CommonSubgraphIsomorphism {
    typedef std::vector<std::vector<size_t> > Vmm;      // vertex matching matrix

    const Graph &g1, &g2;
    Message::Stream debug;                              // optional debugging stream
    DenseIntegerSet<size_t> v, w;                       // vertex IDs from g1 and g2, respectively
    DenseIntegerSet<size_t> vAvoid;                     // vertices of V to avoid (only for debugging)
    std::vector<size_t> x, y;                           // vertex equivalence maps
    size_t n0;                                          // minimum size of desired matches
    size_t nmax;                                        // maximum size of matched subsets (num vertices)
public:
    CommonSubgraphIsomorphism(const Graph &g1, const Graph &g2, Message::Stream debug = Message::mlog[Message::DEBUG])
        : g1(g1), g2(g2), debug(debug), v(g1.nVertices()), w(g2.nVertices()), vAvoid(g1.nVertices()), n0(1), nmax(0) {
        v.insertAll();
        w.insertAll();
    }

    // This is usually user-defined
    bool vertexComparator(const typename Graph::ConstVertexIterator &v1, const typename Graph::ConstVertexIterator &v2) {
        return true;
    }

    // This is usually user-defined
    bool edgeComparator(const std::vector<typename Graph::ConstEdgeIterator> &edges1,
                        const std::vector<typename Graph::ConstEdgeIterator> &edges2) {
        return true;
    }

    // This is usually user-defined
    void output(const std::vector<size_t> &x, const std::vector<size_t> &y) {
        std::cout <<"match found:\n"
                  <<"  x= {";
        BOOST_FOREACH (size_t i, x)
            std::cout <<" " <<i;
        std::cout <<" }\n"
                  <<"  y= {";
        BOOST_FOREACH (size_t j, y)
            std::cout <<" " <<j;
        std::cout <<" }\n";
    }

    void vmmPush(Vmm &d /*in,out*/, size_t i, size_t value) {
        if (i >= d.size())
            d.resize(i+1);
        d[i].push_back(value);
    }

    size_t vmmSize(const Vmm &d, size_t i) {
        return i < d.size() ? d[i].size() : 0;
    }

    const std::vector<size_t>& vmmRow(const Vmm &d, size_t i) {
        static const std::vector<size_t> empty;
        return i < d.size() ? d[i] : empty;
    }

    void vmmPrint(std::ostream &out, const std::string &indent, const std::string &title, const Vmm &d) {
        for (size_t i=0; i<d.size(); ++i) {
            if (!d[i].empty()) {
                out <<indent <<title <<"[" <<i <<"] = {";
                BOOST_FOREACH (size_t j, d[i])
                    out <<" " <<j;
                out <<" }\n";
            }
        }
    }

    void printVector(std::ostream &out, const std::string &indent, const std::string &title, const std::vector<size_t> &v) {
        out <<indent <<title <<"[";
        BOOST_FOREACH (size_t i, v)
            out <<" " <<i;
        out <<" ]\n";
    }

    void printSet(std::ostream &out, const std::string &indent, const std::string &title, const DenseIntegerSet<size_t> &s) {
        out <<indent <<title <<"{";
        BOOST_FOREACH (size_t i, s.values())
            out <<" " <<i;
        out <<" }\n";
    }
    
    void vmmInit(Vmm &d /*out*/) {
        d.clear();
        BOOST_FOREACH (size_t i, v.values()) {
            BOOST_FOREACH (size_t j, w.values()) {
                if (vertexComparator(g1.findVertex(i), g2.findVertex(j)))
                    vmmPush(d, i, j);
            }
        }
    }

    size_t pickVertex(const Vmm &d) {
        DenseIntegerSet<size_t> lv = v;
        for (size_t k=0; k<x.size(); ++k)
            lv.erase(x[k]);
        ASSERT_require(!lv.isEmpty());
#if 0
        // Debugging -- chose the least value
        size_t least = *lv.values().begin();
        BOOST_FOREACH (size_t i, lv.values())
            least = std::min(least, i);
        return least;
#elif 1
        // Choose any value
        return *lv.values().begin();
#else
        BOOST_FOREACH (size_t i, lv.values()) {
            if (vmmSize(d, i) == 0)
                continue;
            bool satisfied = true;
            BOOST_FOREACH (size_t j, lv.values()) {
                if (vmmSize(d, j) < vmmSize(d, i)) {
                    satisfied = false;
                    break;
                }
            }
            if (satisfied)
                return i;
        }
#endif
        ASSERT_not_reachable("Not sure what to do here");
    }

    bool isExtendable(const Vmm &d) {
        size_t q = x.size();
        size_t s = q;
        DenseIntegerSet<size_t> lv = v;

        // We might not need this step. Perhaps the only d_i rows that are not empty are those where i is not in x
        for (size_t i=0; i<q; ++i)
            lv.erase(x[i]);

        BOOST_FOREACH (size_t i, lv.values()) {
            if (vmmSize(d, i) > 0)
                ++s;
        }
        return s >= std::max(n0, nmax) && s > q;
    }

    const std::vector<size_t>& getMappableVertices(size_t i, const Vmm &d) {
        return vmmRow(d, i);
    }

    std::vector<typename Graph::ConstEdgeIterator> findEdges(const Graph &g, size_t source, size_t target) {
        std::vector<typename Graph::ConstEdgeIterator> retval;
        BOOST_FOREACH (const typename Graph::Edge &candidate, g.findVertex(source)->outEdges()) {
            if (candidate.target()->id() == target)
                retval.push_back(g.findEdge(candidate.id()));
        }
        return retval;
    }

    // We just added source1 and source2 to the subgraphs. Target1 and target2 are unmapped vertices that we could consider
    // next.
    bool suitableEdges(size_t source1, size_t target1, size_t source2, size_t target2) {
        ASSERT_require(source1 != target1);
        if (source2 == target2)
            return false;

        std::vector<typename Graph::ConstEdgeIterator> edges1 = findEdges(g1, source1, target1);
        std::vector<typename Graph::ConstEdgeIterator> edges2 = findEdges(g2, source2, target2);

        // Both subgraphs must have the same number of edges.
        if (edges2.size() != edges1.size())
            return false;

        return edgeComparator(edges1, edges2);
    }

    Vmm refine(const Vmm &d) {
        ASSERT_require(x.size() == y.size());
        DenseIntegerSet<size_t> lv = v;
        for (size_t i=0; i<x.size(); ++i)
            lv.erase(x[i]);

        Vmm d1;
        BOOST_FOREACH (size_t i, lv.values()) {
            BOOST_FOREACH (size_t j, d[i]) {
                if (suitableEdges(x.back(), i, y.back(), j))
                    vmmPush(d1, i, j);
            }
        }
        return d1;
    }

    void backtrack(const Vmm &d, size_t level=0) {
        std::string indent;
        static const size_t maxLevel = 9, indentAmount = 2;
        if (debug) {
            if (level > maxLevel) {
                indent = std::string(indentAmount*(maxLevel+1), ' ') + "L" + boost::lexical_cast<std::string>(level) + ": ";
            } else {
                indent = std::string(indentAmount*level, ' ');
            }
            debug <<indent <<"at level " <<level <<":\n";
            printVector(debug, indent, "  x = ", x);
            printVector(debug, indent, "  y = ", y);
            vmmPrint(debug, indent, "  d", d);
            printSet(debug, indent, "  avoiding v = ", vAvoid);
        }

        if (isExtendable(d)) {
            int i = pickVertex(d);
            std::vector<size_t> z = getMappableVertices(i, d);
            if (debug) {
                debug <<indent <<"  pick v[" <<i <<"] mappable to any of w";
                printVector(debug, "", "", z);
            }

            BOOST_FOREACH (size_t j, z) {
                SAWYER_MESG(debug) <<indent <<"  trying v[" <<i <<"] mapped to w[" <<j <<"]\n";
                x.push_back(i);
                y.push_back(j);
                Vmm d1 = refine(d);
                backtrack(d1, level+1);
                x.pop_back();
                y.pop_back();
            }

            SAWYER_MESG(debug) <<indent <<"  backtracking by removing v[" <<i <<"] from consideration\n";
            v.erase(i);
            vAvoid.insert(i);                           // debugging
            backtrack(d, level+1);
            v.insert(i);
            vAvoid.erase(i);                            // debuggng
        } else {
            nmax = std::max(nmax, x.size());
            if (debug) {
                printVector(debug, indent, "  solution x = ", x);
                printVector(debug, indent, "  solution y = ", y);
                debug <<indent <<"  n0 = " <<n0 <<", nmax = " <<nmax <<"\n";
            }
            output(x, y);
        }
    }

    void run() {
        Vmm d;
        vmmInit(d);
        backtrack(d);
    }
};
#endif

} // namespace
} // namespace
} // namespace

#endif

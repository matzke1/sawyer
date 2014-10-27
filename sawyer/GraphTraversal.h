#ifndef Sawyer_GraphTraversal_H
#define Sawyer_GraphTraversal_H

#include <sawyer/Graph.h>
#include <list>
#include <vector>

namespace Sawyer {
namespace Container {

/** Algorithms that operate on container classes. */
namespace Algorithm {

/** Abstract base class for graph traversals.
 *
 *  Graph traversals are similar to single-pass, forward iterators in that a traversal object points to a particular node
 *  (vertex or edge) in a graph and supports dereference and increment operations.  The dereference operator returns a
 *  reference to the node and the increment operator advances the traversal object so it points to the next node.
 *
 *  A graph traversal visits one kind of node, either vertices or edges, but not both.  The type of node that's visited is
 *  called the "primary" type and the other is called the "secondary" type.  When a traversal is dereferenced it returns
 *  instances of its primary type. When a traversal is incremented it uses the secondary type to provide connectivity
 *  information.  For instance, in a vertex traversal the vertices are the primaries and vertices are connected to one another
 *  through edges, the secondaries.  Vice versa for edge traversals.
 *
 *  A graph traversal can traverse over a constant graph or a mutable graph. When traversing over constant graphs,
 *  dereferencing the traversal object returns constant nodes (vertices or edges depending on the traversal kind). When
 *  traversing over a mutable graph the dereferences produce mutable nodes which are permitted to be modified during the
 *  traversal. The connectivity of a graph should not be modified while a traversal is in progress; i.e., traversals are
 *  unstable over insertion and erasure and the traversal will have undefined behavior.
 *
 *  A graph traversal can be depth-first or breadth-first.  A depth-first traversal's increment operator will follow edges
 *  immediately, quickly moving away from an initial vertex before eventually returning to follow another edge from the initial
 *  vertext.  A breadth-first traversal will follow each of the edges from a single vertex before moving to another vertex.
 *
 *  A graph traversal can be in a forward or reverse direction.  Forward traversals flow along edges in their natural direction
 *  from source to target.  Reverse traversals are identical in every respect except they flow backward across edges.
 *
 *  Regardless of the type of traversal, a traversal will normally visit each reachable primary exactly one time. However, the
 *  user may adjust the visited state for each node during the traversal, may change the current position of the traversal, and
 *  may cause the traversal to skip over parts of a graph.
 *
 *  The following traversals are implemented:
 *
 *  @li DepthFirstForwardEdgeTraversal: depth-first traversal that visits edges in their natural direction.
 *  @li DepthFirstReverseEdgeTraversal: depth-first traversal that visits edges backward from target to source.
 *  @li BreadthFirstForwardEdgeTraversal: breadth-first traversal that visits edges in their natural direction.
 *  @li BreadthFirstReverseEdgeTraversal: breadth-first traversal that visits edges backward from target to source.
 *  @li DepthFirstForwardVertexTraversal: depth-first traversal that visits vertices by naturally following edges.
 *  @li DepthFirstReverseVertexTraversal: depth-first traversal that visits vertices by flowing backward along edges.
 *  @li BreadthFirstForwardVertexTraversal: breadth-first traversal that visits vertices by naturally following edges.
 *  @li BreadthFirstReverseVertexTraversal: breadth-first traversal that visits vertices by flowing backward along edges.
 *
 *  Although graph traversals look similar to iterators, there are some important differences to keep in mind:
 *
 *  @li Iterators are lightweight objects typically a few bytes that can be passed around efficiently by value, whereas a
 *      traversal's size is linear with respect to the size of the graph over which it traverses.  The reason for this is that
 *      a traversal must keep track of which nodes have been visited.
 *  @li Iterator increment takes constant time but traversal increment can take longer. The worst case time for traversals
 *      occurs when the graph has a single vertex and only self edges.
 *  @li Iterators can be incremented and decremented to move the pointer forward and backward along a list of nodes, but
 *      traversals can only be incremented in a forward direction.
 *  @li Iterating over nodes requires two iterators--the current position and an end position; traversing nodes requires only
 *      one traversal object which keeps track of its own state.
 *  @li Iterators are copyable and comparable, traversals are not.
 *  @li %Graph iterators are insert and erase stable, traversals are not.
 *  @li Iterators do not follow connectivity, but that's the main point for traversals.
 *  @li Iterators are a core feature of a graph. Traversals are implemented in terms of iterators and don't provide any
 *      functionality that isn't already available to the user through the Graph API.
 *
 * @section example Examples
 *
 *  The examples in this section assume the following context:
 *
 * @code
 *  #include <sawyer/GraphTraversal.h>
 *
 *  using namespace Sawyer::Container;
 *  using namespace Sawyer::Container::Algorithm;
 *
 *  typedef ... MyVertexType;   // User data for vertices; any default-constructable, copyable type
 *  typedef ... MyEdgeType;     // Ditto for edges. These can be Sawyer::Nothing if desired.
 *
 *  typedef Graph<MyVertexType, MyEdgeType> MyGraph;
 *  MyGraph graph;
 * @endcode
 *
 *  This example shows how to build a reachability vector.  The vector is indexed by vertex ID number and is true when the
 *  vertex is reachable from the starting vertex by following edges in their natural direction.  The starting point is a
 *  vertex, but it could just as well have been an edge that points to the first vertex.  One could also create edge
 *  reachability vectors by using an edge traversal instead.  The @ref next method returns the current vertex and advances the
 *  traversal to the next vertex.
 *
 * @code
 *  MyGraph::VertexNodeIterator startingVertex = ....;
 *  std::vector<bool> reachable(graph.nVertices(), false);
 *
 *  DepthFirstForwardVertexTraversal<MyGraph> traversal(graph, startingVertex);
 *  while (traversal.hasNext())
 *      reachable[traversal.next().id()] = true;
 * @endcode
 *
 *  A more typical C++ iterator paradigm can be used instead. However, traversals are not comparable and there is no "end"
 *  traversal like there are for iterators.  Evaluating a traversal in Boolean context returns true unless the traversal is at
 *  the end.  Here's the loop again:
 *
 * @code
 *  typedef DepthFirstForwardVertexTraversal<MyGraph> Traversal;
 *  for (Traversal traversal(graph, start); traversal; ++traversal)
 *      reachable[traversal->id()] = true;
 * @endcode
 *
 *  All traversals support skipping over parts of the graph by using the @ref childAvoidance Boolean property. If the property
 *  is set when the traversal is advancing then the traversal avoids following the current node and then the property is
 *  automatically cleared.  For instance, the following code computes an edge reachability vector for a control flow graph but
 *  does not follow edges that are marked as function calls.  The @c value method returns a reference to a @c MyEdgeType (see
 *  typedef example above) which we assume has a @c isFunctionCall method.
 *
 * @code
 *  MyGraph::VertexNodeIterator startingVertex = ...;
 *  std::vector<bool> edgeReachable(graph.nEdges(), false);
 *  typedef DepthFirstForwardEdgeTraversal<MyGraph> Traversal;
 *
 *  for (Traversal t(graph, startingVertex); t; ++t) {
 *      edgeReachable[t->id()] = true;
 *      t.childAvoidance(t->value().isFunctionCall());
 *  }
 * @endcode
 *
 *  The fact that traversals iterate over only vertices or only edges is not a handicap. For instance, the following code is
 *  like the example above but it computes @e vertex reachability without following edges that are function calls. Note that a
 *  vertex can be "visited" more than once by this procedure since it may have multiple incoming edges. But this is actually
 *  what we want because if one of the incoming edges is a function call and the other isn't then the result should be that the
 *  vertex is reachable.
 *
 * @code
 *  MyGraph::VertexNodeIterator startingVertex = ...;
 *  std::vector<bool> vertexReachable(graph.nVertices(), false);
 *  typedef DepthFirstForwardEdgeTraversal<MyGraph> Traversal;
 *
 *  for (Traversal t(graph, startingVertex); t; ++t) {
 *      if (t->value().isFunctionCall()) {
 *          t.childAvoidance(true);
 *      } else {
 *          MyGraph::VertexNodeIterator vertex = t->target(); // target vertex for edge
 *          vertexReachable[vertex->id()] = true;
 *      }
 *  }
 * @endcode
 *
 *  Traversals are suitable for using in generic functions as well.  For instance, the next example is a generic function that
 *  computes a vertex reachability vector for any type of %Sawyer graph.  It operates on const or non-const graphs for
 *  demonstration purposes; in real life one might advertise that the function doesn't modify the graph by having an
 *  explicit const qualifier. The GraphTraits is used to obtain the appropriate const/non-const vertex iterator type.
 *
 * @code
 *  template<class Graph>
 *  std::vector<bool> reachable(Graph &graph, typename GraphTraits<Graph>::VertexNodeIterator startingVertex) {
 *      std::vector<bool> retval(graph.nVertices(), false);
 *      DepthFirstForwardVertexTraversal<Graph> traversal(graph, startingVertex);
 *      while (traversal.hasNext())
 *          retval[traversal.next().id()] = true;
 *      return retval;
 *  }
 * @endcode */
template<class Graph, class PrimaryIterator, class SecondaryIterator>
class GraphTraversal {
    // PrimaryIterator is the thing that next() dereferences (EdgeNodeIterator for edge traversals, VertexNodeIterator for
    // vertex traversals).  SecondaryIterator is the other kind of iterator (VertexNodeIterator for edge traversals,
    // EdgeNodeIterator for vertex traversals).
protected:
    Graph &graph_;
    typedef std::list<PrimaryIterator> WorkList;        // vertex or edge node iterators
    WorkList workList_;
private:
    std::vector<uint8_t> visited_;                      // vertex or edge has been visited?
    bool childAvoidance_;                               // avoid children on next increment?

protected:
    explicit GraphTraversal(Graph &graph, size_t nPrimaries)
        : graph_(graph), visited_((nPrimaries+7)/8, 0), childAvoidance_(false) {};

public:
    /** Returns true when traversal reaches the end. */
    bool atEnd() const {
        return workList_.empty();
    }

    /** Returns false when traversal has more to visit. */
    bool hasNext() {
        popVisited();
        return !atEnd();
    }

    /** Consumes and returns one node from the traversal.
     *
     *  The current node is returned, the traversal advances to the next unvisited node, and the @ref childAvoidance property
     *  is cleared. This method should not be called if @ref hasNext returns false (or @ref atEnd returns true). */
    typename PrimaryIterator::Reference next() {
        if (!hasNext())
            throw std::runtime_error("end iterator has no next value");
        typename PrimaryIterator::Reference retval = *iterator();
        increment();
        return retval;
    }

    /** Advance to the next node.
     *
     *  Advances the traversal to the next unvisited node and clears the @ref childAvoidance property.  This method should not
     *  be called if @ref hasNext returns false (or @ref atEnd returns true).  This method is called by @ref next and the
     *  increment operator. */
    void increment() {
        if (atEnd())
            throw std::runtime_error("advanced iterator past end");
        PrimaryIterator primary = workList_.front();
        workList_.pop_front();
        this->markVisited(primary->id());
        if (!childAvoidance_) {
            boost::iterator_range<SecondaryIterator> secondaries = nextSecondaries(primary);
            for (SecondaryIterator si=secondaries.begin(); si!=secondaries.end(); ++si)
                pushPrimaries(si);
        } else {
            childAvoidance_ = false;
        }
        popVisited();
    }

    /** Dereference the traversal to obtain the current node.
     *
     *  Returns a reference to the current node (vertex or edge) without advancing the traversal.  This should only be called
     *  when @ref atEnd returns false (or @ref hasNext returns true).
     *
     * @{ */
    typename PrimaryIterator::Reference operator*() const {
        return *iterator();
    }
    typename PrimaryIterator::Pointer operator->() const {
        return &*iterator();
    }
    /** @} */

    /** Returns an iterator for the current node.
     *
     *  This is similar to a dereference operation, but it returns an iterator instead of a node reference.  The iterator is
     *  either a vertex or edge iterator, either const or non-const variety. Alternatively, the caller can convert nodes to
     *  iteraters by using @ref Graph::findVertex or @ref Graph::findEdge given the graph and node ID. */
    PrimaryIterator iterator() const {
        if (atEnd())
            throw std::runtime_error("dereferenced end iterator");
        return workList_.front();
    }
    
    /** Returns true if node has been visited.
     *
     *  Returns true if the specified primary node (vertex or edge) has been visited.  A node is marked as visited only when
     *  the traversal has incremented over the node and advanced past it. */
    bool isVisited(size_t nodeId) const {
        return 0 != (visited_[nodeId>>3] & (uint8_t(1) << (nodeId & 7)));
    }

    /** Mark a primary node as having been visited or not.
     *
     *  If a node is marked as visited then it will not be traversed.  Nodes are automatically marked as having been visited as
     *  the traversal is advanced over the node (i.e., the property is cleared for node @e x when an advancing operation like
     *  @ref next or "++" is called when @e x is the current node). If @p b is false then the visited state of the node is
     *  cleared instead.
     *
     *  Clearing the visited state of a node may or may not cause the node to be visited again depending on the implementation
     *  in subclasses.  The subclass is allowed to prune already-visited nodes from its work list as the worklist is
     *  created. Clearing a pruned node's visited state will not cause it to be magically added back to the worklist -- in what
     *  order would it be inserted?  However, see also @ref visit, that can be used to force a visit to a node regardless of
     *  its visited state. */
    void markVisited(size_t nodeId, bool b=true) {
        uint8_t mask = uint8_t(1) << (nodeId & 7);
        if (b) {
            visited_[nodeId>>3] |= mask;
        } else {
            visited_[nodeId>>3] &= ~mask;
        }
    }

    /** Visit a particular node.
     *
     *  The traversal can be adjusted so it is visiting a particular node regardless of whether that node has already been
     *  visited. The node that was previously being visited is not removed from the traversal and will eventually be visited
     *  again.  If the caller does not want that behavior then it should either increment the traversal before setting the new
     *  position, or explicitly mark the node as having been visited already by calling @ref markVisited.  The @p node to visit
     *  must not be an end iterator or undefined behavior will result.  It is permissible to set the current position even when
     *  the traversal has reached its end. */
    void visit(PrimaryIterator node) {
        workList_.push_front(node);
    }

    /** Property: child avoidance
     *
     *  If this property is set then the next incrementing operation will not traverse the current node.  For instance, if the
     *  traversal is visiting vertices and this property is set then an increment operation will not follow the edges of this
     *  vertex to reach neighboring vertices. Likewise, for an edge traversal, the increment will not follow the edge to a
     *  vertex to reach neighboring edges.  The edges that are not reached are not marked as having been visited, therefore
     *  they might be visited by other paths.  This property is cleared by the next incrementing operation.
     *
     * @{ */
    bool childAvoidance() const { return childAvoidance_; }
    void childAvoidance(bool b) { childAvoidance_ = b; }
    /** @} */

    // The following trickery is to allow things like "if (x)" to work but without having an implicit
    // conversion to bool which would cause no end of other problems. This is fixed in C++11.
private:
    typedef void(GraphTraversal::*unspecified_bool)() const;
    void this_type_does_not_support_comparisons() const {}
public:
    /** Type for Boolean context.
     *
     *  Implicit conversion to a type that can be used in a boolean context such as an <code>if</code> or <code>while</code>
     *  statement.  For instance:
     *
     *  @code
     *   typedef DepthFirstForwardVertexTraversal<MyGraph> Traversal;
     *   for (Traversal t(graph, startVertex); t; ++t)
     *       reachable[t->id()] = true;
     *  @endcode */
    operator unspecified_bool() const {
        return atEnd() ? 0 : &GraphTraversal::this_type_does_not_support_comparisons;
    }

protected:
    // Given a secondary iterator, push primary iterators.  For instance, in an edge traversal the edges are primary and the
    // vertices are secondary; vice versa in a vertex traversal.  Forward traversals that flow from edge source to target while
    // reverse traversals flow from edge target to source.  Depth-first traversals push onto the front of the work list while
    // breadth-first traversals push onto the back of the work list.  The current item is the front of the list.
    virtual void pushPrimaries(SecondaryIterator) = 0;

    // Given a primary iterator return its secondary neighbors.  For instance, in a forward vertex traversal it would return
    // the vertex's outgoing edges.
    virtual boost::iterator_range<SecondaryIterator> nextSecondaries(PrimaryIterator) const = 0;

    // Pop already-visited primaries from the front of the work list.
    void popVisited() {
        while (!workList_.empty() && isVisited(workList_.front()->id()))
            workList_.pop_front();
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Abstract base class for graph vertex traversals.
 *
 *  @sa GraphTraversal. */
template<class Graph>
class GraphVertexTraversal: public GraphTraversal<Graph,
                                                  typename GraphTraits<Graph>::VertexNodeIterator, // primary iterator
                                                  typename GraphTraits<Graph>::EdgeNodeIterator    // secondary iterator
                                                  > {
public:
    typedef typename GraphTraits<Graph>::VertexNodeIterator VertexNodeIterator;
    typedef typename GraphTraits<Graph>::EdgeNodeIterator EdgeNodeIterator;

protected:
    explicit GraphVertexTraversal(Graph &graph)
        : GraphTraversal<Graph, VertexNodeIterator, EdgeNodeIterator>(graph, graph.nVertices()) {}

    GraphVertexTraversal(Graph &graph, VertexNodeIterator startVertex)
        : GraphTraversal<Graph, VertexNodeIterator, EdgeNodeIterator>(graph, graph.nVertices()) {
        if (startVertex != graph.vertices().end())
            this->workList_.push_front(startVertex);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Depth-first traversal to visit vertices in their natural order.
 *
 *  The advancing operators like @ref next and increment visit unvisited neighboring vertices by following outgoing edges
 *  before they visit unvisited sibling vertices.
 *
 * @sa GraphTraversal */
template<class Graph>
class DepthFirstForwardVertexTraversal: public GraphVertexTraversal<Graph> {
public:
    /** Const or non-const vertex node iterator. */
    typedef typename GraphTraits<Graph>::VertexNodeIterator VertexNodeIterator;

    /** Const or non-const edge node iterator. */
    typedef typename GraphTraits<Graph>::EdgeNodeIterator EdgeNodeIterator;

    /** Start traversal at specified vertex. */
    DepthFirstForwardVertexTraversal(Graph &graph, VertexNodeIterator startVertex)
        : GraphVertexTraversal<Graph>(graph, startVertex) {}

    /** Start traversal at specified edge.
     *
     *  The @p startEdge target vertex is used as the initial vertex for this traversal. */
    DepthFirstForwardVertexTraversal(Graph &graph, EdgeNodeIterator startEdge): GraphVertexTraversal<Graph>(graph) {
        if (startEdge != graph.edges().end())
            pushPrimaries(startEdge);
    }

    /** Advance to next node.
     *
     *  This is the same as calling @ref increment except it also returns a reference to this traversal.
     *
     *  @sa GraphTraversal */
    DepthFirstForwardVertexTraversal& operator++() {
        this->increment();
        return *this;
    }

private:
    virtual void pushPrimaries(EdgeNodeIterator edge) /*override*/ {
        this->workList_.push_front(edge->target());
    }

    virtual boost::iterator_range<VertexNodeIterator> nextSecondaries(VertexNodeIterator vertex) const /*override*/ {
        return vertex->outEdges();
    }
};

/** Depth-first traversal to visit vertices by following edges in reverse.
 *
 *  The advancing operators like @ref next and increment visit unvisited neighboring vertices by following incoming edges
 *  in reverse before they visit unvisited sibling vertices.
 *
 * @sa GraphTraversal */
template<class Graph>
class DepthFirstReverseVertexTraversal: public GraphVertexTraversal<Graph> {
public:
    /** Const or non-const vertex node iterator. */
    typedef typename GraphTraits<Graph>::VertexNodeIterator VertexNodeIterator;

    /** Const or non-const edge node iterator. */
    typedef typename GraphTraits<Graph>::EdgeNodeIterator EdgeNodeIterator;

    /** Start traversal at specified vertex. */
    DepthFirstReverseVertexTraversal(Graph &graph, VertexNodeIterator startVertex)
        : GraphVertexTraversal<Graph>(graph, startVertex) {}

    /** Start traversal at specified edge.
     *
     *  The @p startEdge source vertex is used as the initial vertex for this traversal. */
    DepthFirstReverseVertexTraversal(Graph &graph, EdgeNodeIterator startEdge): GraphVertexTraversal<Graph>(graph) {
        if (startEdge != graph.edges().end())
            pushPrimaries(startEdge);
    }

    /** Advance to next node.
     *
     *  This is the same as calling @ref increment except it also returns a reference to this traversal.
     *
     *  @sa GraphTraversal */
    DepthFirstReverseVertexTraversal& operator++() {
        this->increment();
        return *this;
    }

private:
    virtual void pushPrimaries(EdgeNodeIterator edge) /*override*/ {
        this->workList_.push_front(edge->source());
    }

    virtual boost::iterator_range<VertexNodeIterator> nextSecondaries(VertexNodeIterator vertex) const /*override*/ {
        return vertex->inEdges();
    }
};

/** Breadth-first traversal to visit vertices in their natural order.
 *
 *  The advancing operators like @ref next and increment visit unvisited sibling vertices before visiting neighboring vertices
 *  by following outgoing edges.
 *
 * @sa GraphTraversal */
template<class Graph>
class BreadthFirstForwardVertexTraversal: public GraphVertexTraversal<Graph> {
public:
    /** Const or non-const vertex node iterator. */
    typedef typename GraphTraits<Graph>::VertexNodeIterator VertexNodeIterator;

    /** Const or non-const edge node iterator. */
    typedef typename GraphTraits<Graph>::EdgeNodeIterator EdgeNodeIterator;

    /** Start traversal at specified vertex. */
    BreadthFirstForwardVertexTraversal(Graph &graph, VertexNodeIterator startVertex)
        : GraphVertexTraversal<Graph>(graph, startVertex) {}

    /** Start traversal at specified edge.
     *
     *  The @p startEdge target vertex is used as the initial vertex for this traversal. */
    BreadthFirstForwardVertexTraversal(Graph &graph, EdgeNodeIterator startEdge): GraphVertexTraversal<Graph>(graph) {
        if (startEdge != graph.edges().end())
            pushPrimaries(startEdge);
    }

    /** Advance to next node.
     *
     *  This is the same as calling @ref increment except it also returns a reference to this traversal.
     *
     *  @sa GraphTraversal */
    BreadthFirstForwardVertexTraversal& operator++() {
        this->increment();
        return *this;
    }

private:
    virtual void pushPrimaries(EdgeNodeIterator edge) /*override*/ {
        this->workList_.push_back(edge->target());
    }

    virtual boost::iterator_range<VertexNodeIterator> nextSecondaries(VertexNodeIterator vertex) const /*override*/ {
        return vertex->outEdges();
    }
};

/** Breadth-first traversal to visit vertices by following edges in reverse.
 *
 *  The advancing operators like @ref next and increment visit unvisited sibling vertices before visiting neighboring vertices
 *  by following incoming edges in reverse.
 *
 * @sa GraphTraversal */
template<class Graph>
class BreadthFirstReverseVertexTraversal: public GraphVertexTraversal<Graph> {
public:
    /** Const or non-const vertex node iterator. */
    typedef typename GraphTraits<Graph>::VertexNodeIterator VertexNodeIterator;

    /** Const or non-const edge node iterator. */
    typedef typename GraphTraits<Graph>::EdgeNodeIterator EdgeNodeIterator;

    /** Start traversal at specified vertex. */
    BreadthFirstReverseVertexTraversal(Graph &graph, VertexNodeIterator startVertex)
        : GraphVertexTraversal<Graph>(graph, startVertex) {}

    /** Start traversal at specified edge.
     *
     *  The @p startEdge source vertex is used as the initial vertex for this traversal. */
    BreadthFirstReverseVertexTraversal(Graph &graph, EdgeNodeIterator startEdge): GraphVertexTraversal<Graph>(graph) {
        if (startEdge != graph.edges().end())
            pushPrimaries(startEdge);
    }

    /** Advance to next node.
     *
     *  This is the same as calling @ref increment except it also returns a reference to this traversal.
     *
     *  @sa GraphTraversal */
    BreadthFirstReverseVertexTraversal& operator++() {
        this->increment();
        return *this;
    }

private:
    virtual void pushPrimaries(EdgeNodeIterator edge) /*override*/ {
        this->workList_.push_back(edge->source());
    }

    virtual boost::iterator_range<VertexNodeIterator> nextSecondaries(VertexNodeIterator vertex) const /*override*/ {
        return vertex->inEdges();
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Abstract base class for graph edge traversals.
 *
 *  @sa GraphTraversal */
template<class Graph>
class GraphEdgeTraversal: public GraphTraversal<Graph,
                                                typename GraphTraits<Graph>::EdgeNodeIterator,   // primary iterator
                                                typename GraphTraits<Graph>::VertexNodeIterator  // secondary iterator
                                                > {
public:
    typedef typename GraphTraits<Graph>::VertexNodeIterator VertexNodeIterator;
    typedef typename GraphTraits<Graph>::EdgeNodeIterator EdgeNodeIterator;

protected:
    explicit GraphEdgeTraversal(Graph &graph)
        : GraphTraversal<Graph, EdgeNodeIterator, VertexNodeIterator>(graph, graph.nEdges()) {}
        
    GraphEdgeTraversal(Graph &graph, EdgeNodeIterator startEdge)
        : GraphTraversal<Graph, EdgeNodeIterator, VertexNodeIterator>(graph, graph.nEdges()) {
        if (startEdge != graph.edges().end())
            this->workList_.push_front(startEdge);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Depth-first traversal to visit edges in their natural order.
 *
 *  The advancing operators like @ref next and increment visit unvisited neighboring edges by following edges in their natural
 *  direction before they visit unvisited sibling edges.
 *
 * @sa GraphTraversal */
template<class Graph>
class DepthFirstForwardEdgeTraversal: public GraphEdgeTraversal<Graph> {
public:
    /** Const or non-const vertex node iterator. */
    typedef typename GraphTraits<Graph>::VertexNodeIterator VertexNodeIterator;

    /** Const or non-const edge node iterator. */
    typedef typename GraphTraits<Graph>::EdgeNodeIterator EdgeNodeIterator;

public:
    /** Start traversal at specified edge. */
    DepthFirstForwardEdgeTraversal(Graph &graph, EdgeNodeIterator startEdge): GraphEdgeTraversal<Graph>(graph, startEdge) {}

    /** Start traversal at specified vertex.
     *
     *  The @p startVertex outgoing edges are all added to the traversal and one of them is chosen as the initial edge. */
    DepthFirstForwardEdgeTraversal(Graph &graph, VertexNodeIterator startVertex): GraphEdgeTraversal<Graph>(graph) {
        if (startVertex != graph.vertices().end())
            pushPrimaries(startVertex);
    }

    /** Advance to next node.
     *
     *  This is the same as calling @ref increment except it also returns a reference to this traversal.
     *
     *  @sa GraphTraversal */
    DepthFirstForwardEdgeTraversal& operator++() {
        this->increment();
        return *this;
    }

private:
    virtual void pushPrimaries(VertexNodeIterator vertex) /*override*/ {
        for (EdgeNodeIterator ei=vertex->outEdges().begin(); ei!=vertex->outEdges().end(); ++ei)
            this->workList_.push_front(ei);
    }

    virtual boost::iterator_range<VertexNodeIterator> nextSecondaries(EdgeNodeIterator edge) const /*override*/ {
        VertexNodeIterator vBegin = edge->target();
        VertexNodeIterator vEnd = vBegin; ++vEnd;
        return boost::iterator_range<VertexNodeIterator>(vBegin, vEnd);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Depth-first traversal to visit edges in their reverse direction.
 *
 *  The advancing operators like @ref next and increment visit unvisited neighboring edges by following edges in their reverse
 *  direction (from target toward source) before they visit unvisited sibling edges.
 *
 * @sa GraphTraversal */
template<class Graph>
class DepthFirstReverseEdgeTraversal: public GraphEdgeTraversal<Graph> {
public:
    /** Const or non-const vertex node iterator. */
    typedef typename GraphTraits<Graph>::VertexNodeIterator VertexNodeIterator;

    /** Const or non-const edge node iterator. */
    typedef typename GraphTraits<Graph>::EdgeNodeIterator EdgeNodeIterator;

public:
    /** Start traversal at specified edge. */
    DepthFirstReverseEdgeTraversal(Graph &graph, EdgeNodeIterator startEdge): GraphEdgeTraversal<Graph>(graph, startEdge) {}

    /** Start traversal at specified vertex.
     *
     *  The @p startVertex incoming edges are all added to the traversal and one of them is chosen as the initial edge. */
    DepthFirstReverseEdgeTraversal(Graph &graph, VertexNodeIterator startVertex): GraphEdgeTraversal<Graph>(graph) {
        if (startVertex != graph.vertices().end())
            pushPrimaries(startVertex);
    }
    
    /** Advance to next node.
     *
     *  This is the same as calling @ref increment except it also returns a reference to this traversal.
     *
     *  @sa GraphTraversal */
    DepthFirstReverseEdgeTraversal& operator++() {
        this->increment();
        return *this;
    }

private:
    virtual void pushPrimaries(VertexNodeIterator vertex) /*override*/ {
        for (EdgeNodeIterator ei=vertex->inEdges().begin(); ei!=vertex->inEdges().end(); ++ei)
            this->workList_.push_front(ei);
    }

    virtual boost::iterator_range<VertexNodeIterator> nextSecondaries(EdgeNodeIterator edge) const /*override*/ {
        VertexNodeIterator vBegin = edge->source();
        VertexNodeIterator vEnd = vBegin; ++vEnd;
        return boost::iterator_range<VertexNodeIterator>(vBegin, vEnd);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Breadth-first traversal to visit edges in their natural order.
 *
 *  The advancing operators like @ref next and increment visit unvisited sibling edges before visiting neighboring edges by
 *  following edges in their natural direction.
 *
 * @sa GraphTraversal */
template<class Graph>
class BreadthFirstForwardEdgeTraversal: public GraphEdgeTraversal<Graph> {
public:
    /** Const or non-const vertex node iterator. */
    typedef typename GraphTraits<Graph>::VertexNodeIterator VertexNodeIterator;

    /** Const or non-const edge node iterator. */
    typedef typename GraphTraits<Graph>::EdgeNodeIterator EdgeNodeIterator;

public:
    /** Start traversal at specified edge. */
    BreadthFirstForwardEdgeTraversal(Graph &graph, EdgeNodeIterator startEdge): GraphEdgeTraversal<Graph>(graph, startEdge) {}

    /** Start traversal at specified vertex.
     *
     *  The @p startVertex outgoing edges are all added to the traversal and one of them is chosen as the initial edge. */
    BreadthFirstForwardEdgeTraversal(Graph &graph, VertexNodeIterator startVertex): GraphEdgeTraversal<Graph>(graph) {
        if (startVertex != graph.vertices().end())
            pushPrimaries(startVertex);
    }

    /** Advance to next node.
     *
     *  This is the same as calling @ref increment except it also returns a reference to this traversal.
     *
     *  @sa GraphTraversal */
    BreadthFirstForwardEdgeTraversal& operator++() {
        this->increment();
        return *this;
    }

private:
    virtual void pushPrimaries(VertexNodeIterator vertex) /*override*/ {
        for (EdgeNodeIterator ei=vertex->outEdges().begin(); ei!=vertex->outEdges().end(); ++ei)
            this->workList_.push_back(ei);
    }

    virtual boost::iterator_range<VertexNodeIterator> nextSecondaries(EdgeNodeIterator edge) const /*override*/ {
        VertexNodeIterator vBegin = edge->target();
        VertexNodeIterator vEnd = vBegin; ++vEnd;
        return boost::iterator_range<VertexNodeIterator>(vBegin, vEnd);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Breadth-first traversal to visit edges in their reverse direction.
 *
 *  The advancing operators like @ref next and increment visit unvisited sibling edges before visiting neighboring edges by
 *  following edges in their reverse direction (from target toward source).
 *
 * @sa GraphTraversal */
template<class Graph>
class BreadthFirstReverseEdgeTraversal: public GraphEdgeTraversal<Graph> {
public:
    /** Const or non-const vertex node iterator. */
    typedef typename GraphTraits<Graph>::VertexNodeIterator VertexNodeIterator;

    /** Const or non-const edge node iterator. */
    typedef typename GraphTraits<Graph>::EdgeNodeIterator EdgeNodeIterator;

public:
    /** Start traversal at specified edge. */
    BreadthFirstReverseEdgeTraversal(Graph &graph, EdgeNodeIterator startEdge): GraphEdgeTraversal<Graph>(graph, startEdge) {}

    /** Start traversal at specified vertex.
     *
     *  The @p startVertex incoming edges are all added to the traversal and one of them is chosen as the initial edge. */
    BreadthFirstReverseEdgeTraversal(Graph &graph, VertexNodeIterator startVertex): GraphEdgeTraversal<Graph>(graph) {
        if (startVertex != graph.vertices().end())
            pushPrimaries(startVertex);
    }

    /** Advance to next node.
     *
     *  This is the same as calling @ref increment except it also returns a reference to this traversal.
     *
     *  @sa GraphTraversal */
    BreadthFirstReverseEdgeTraversal& operator++() {
        this->increment();
        return *this;
    }

private:
    virtual void pushPrimaries(VertexNodeIterator vertex) /*override*/ {
        for (EdgeNodeIterator ei=vertex->outEdges().begin(); ei!=vertex->outEdges().end(); ++ei)
            this->workList_.push_back(ei);
    }

    virtual boost::iterator_range<VertexNodeIterator> nextSecondaries(EdgeNodeIterator edge) const /*override*/ {
        VertexNodeIterator vBegin = edge->source();
        VertexNodeIterator vEnd = vBegin; ++vEnd;
        return boost::iterator_range<VertexNodeIterator>(vBegin, vEnd);
    }
};

} // namespace
} // namespace
} // namespace

#endif

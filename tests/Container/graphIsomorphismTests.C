#include <Sawyer/GraphAlgorithm.h>
#include <Sawyer/Message.h>

using namespace Sawyer::Message::Common;

typedef Sawyer::Container::Graph<std::string, std::string> Graph;

//                      [ 4 ]
//                        ^
//                        |
//                        |
//         [ 2 ] -----> [ 3 ]
//           ^            ^
//            \          /
//              \      /
//                \  /
//                [ 1 ]
//                  ^
//                  |
//                  |
//                [ 0 ]
static Graph
buildGraph1() {
    Graph g;
    Graph::VertexIterator v0 = g.insertVertex("v0");
    Graph::VertexIterator v1 = g.insertVertex("v1");
    Graph::VertexIterator v2 = g.insertVertex("v2");
    Graph::VertexIterator v3 = g.insertVertex("v3");
    Graph::VertexIterator v4 = g.insertVertex("v4");

    g.insertEdge(v0, v1, "e01");
    g.insertEdge(v1, v2, "e12");
    g.insertEdge(v1, v3, "e13");
    g.insertEdge(v2, v3, "e23");
    g.insertEdge(v3, v4, "e34");

    return g;
}

//   [ 5 ] <----- [ 6 ]
//     ^            ^
//     \           /
//       \       /
//         \   /
//         [ 4 ]
//           ^
//           |
//           |
//         [ 2 ] <----- [ 3 ]
//           ^            ^
//            \          /
//              \      /
//                \  /
//                [ 1 ]
//                  ^
//                  |
//                  |
//                [ 0 ]
static Graph
buildGraph2() {
    Graph g;
    Graph::VertexIterator w0 = g.insertVertex("w0");
    Graph::VertexIterator w1 = g.insertVertex("w1");
    Graph::VertexIterator w2 = g.insertVertex("w2");
    Graph::VertexIterator w3 = g.insertVertex("w3");
    Graph::VertexIterator w4 = g.insertVertex("w4");
    Graph::VertexIterator w5 = g.insertVertex("w5");
    Graph::VertexIterator w6 = g.insertVertex("w6");

    g.insertEdge(w0, w1, "f01");
    g.insertEdge(w1, w2, "f12");
    g.insertEdge(w1, w3, "f13");
    g.insertEdge(w3, w2, "f32");
    g.insertEdge(w2, w4, "f24");
    g.insertEdge(w4, w5, "f45");
    g.insertEdge(w4, w6, "f46");
    g.insertEdge(w6, w5, "f65");

    return g;
}

static void
testSubgraphIsomorphism() {
    Sawyer::Message::Stream debug = Sawyer::Message::mlog[DEBUG];
    //debug.enable();

    Graph g1 = buildGraph1();
    Graph g2 = buildGraph2();

    Sawyer::Container::Algorithm::CommonSubgraphIsomorphism<Graph> csi(g1, g2, debug);
    csi.run();
}

int main() {
    Sawyer::initializeLibrary();
    testSubgraphIsomorphism();
}

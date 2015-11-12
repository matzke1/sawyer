// Tests multi-threaded workers.

#include <Sawyer/ThreadWorkers.h>
#include <iostream>

// This mutex is used around std::cerr::operator<< in order for worker threads to not get their output all mixed together
static boost::mutex outputMutex;

// Define a unit of work. For testing purposes, it's just the number of milliseconds that the worker should take to do
// this.
class WorkItem {
public:
    boost::posix_time::milliseconds duration;

    explicit WorkItem(unsigned long millies)
        : duration(millies) {}
};

// Define what it means to work on an item. This demo uses a class with an operator(), but you can also use function pointers
// and lambdas.  These functors are called from worker threads, not the main thread.
class WorkFunctor {
public:
    void operator()(size_t workItemId, const WorkItem &workItem) {
        //showProgress(workItemId);                       // these are vertex ID numbers of the lattice, 0 through |V|-1
        boost::this_thread::sleep(workItem.duration);   // this is the "work" done on the work item.
    }
private:
    void showProgress(size_t workItemId) {              // use std::cerr because it's line buffered
        boost::lock_guard<boost::mutex> lock(outputMutex);
        std::cerr <<"thread " <<boost::this_thread::get_id() <<" working on " <<workItemId <<"\n";
    }
};

// Define a dependency lattice to hold all the work items.  Graph is a superset of lattice, so we use that (the thread system
// verifies that the graph forms a forest of lattices).  An edge from vertex A to B means work A depends on work B.
typedef Sawyer::Container::Graph<WorkItem> Work;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// testing...

int
main() {
    // Create some work.
    static const size_t nWorkItems = 1024;
    static const unsigned long duration = 100;          // milliseconds that each work item takes
    Work work;                                          // all work items with dependency information
    for (size_t i=0; i<nWorkItems; ++i) {
        // Each work item is a vertex in the lattice. Think of the "VertexIterator" type as being a pointer rather than an
        // iterator -- it's an iterator pointing to some vertex but we never increment the iterator.  This is sort of like
        // pointers into arrays: they can be iterators or pointers.
        Work::VertexIterator newWork = work.insertVertex(WorkItem(duration));

        // We want to add some dependencies between work items for testing. We'll make the dependencies a binary tree.
        if (i>0) {
            Work::VertexIterator parent = work.findVertex((i-1)/2); // parent that depends on this work
            work.insertEdge(parent, newWork);                       // edge from parent to newWork
        }
    }

    // Perform the work. This doesn't return until the work is finished.
    static const size_t nThreads = 50;
    Sawyer::workInParallel(work, nThreads, WorkFunctor());
    std::cerr <<"All done (work item #0 should have been the final output)!\n";
}

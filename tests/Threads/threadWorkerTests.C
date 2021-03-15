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
        showStartProgress(workItemId);                  // these are vertex ID numbers of the lattice, 0 through |V|-1
        boost::this_thread::sleep(workItem.duration);   // this is the "work" done on the work item.
        showEndProgress(workItemId);
    }
private:
    void showStartProgress(size_t workItemId) {              // use std::cerr because it's line buffered
        boost::lock_guard<boost::mutex> lock(outputMutex);
        std::cerr <<"thread " <<boost::this_thread::get_id() <<" starting work on " <<workItemId <<"\n";
    }
    void showEndProgress(size_t workItemId) {              // use std::cerr because it's line buffered
        boost::lock_guard<boost::mutex> lock(outputMutex);
        std::cerr <<"thread " <<boost::this_thread::get_id() <<" finished work on " <<workItemId <<"\n";
    }
};

// Define a dependency lattice to hold all the work items.  Graph is a superset of lattice, so we use that (the thread system
// verifies that the graph forms a forest of lattices).  An edge from vertex A to B means work A depends on work B.
typedef Sawyer::Container::Graph<WorkItem> Work;

// Optionally define a functor to report status. The inProgress argument is the set of vertices of the "work" graph that are
// currently running (although they might finish running during this call).
class ReportStatus {
public:
    void operator()(const Work &work, size_t nFinished, const std::set<size_t> inProgress) {
        boost::lock_guard<boost::mutex> lock(outputMutex);
        std::cerr <<"*** Still working: "
                  <<(100.0 * nFinished / work.nVertices()) <<"% completed, "
                  <<inProgress.size() <<" workers busy\n";
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// testing...

int
main() {
    static const size_t nThreads = 200;                 // can be big since the "work" is only to sleep
    static const size_t nWorkItems = 255;               // 2^n-1 so we can see the binary tree dependencies in action
    static const unsigned long duration = 5000;         // milliseconds that each work item takes

    // Create some work.
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
            std::cerr <<parent->id() <<" depends on " <<newWork->id() <<"\n";
        }
    }

#if 0 // [Robb Matzke 2015-11-12]
    // Perform the work. This doesn't return until the work is finished.
    Sawyer::workInParallel(work, nThreads, WorkFunctor());
    std::cerr <<"All done (work item #0 should have been the final output)!\n";

#elif 1 // [Robb Matzke 2021-03-15]
    // Perform the work. This doesn't return until the work is finished.
    Sawyer::workInParallel(work, nThreads, WorkFunctor(), ReportStatus(), boost::chrono::seconds(3));

#elif 1 // [Robb Matzke 2015-11-11]
    // Run the work asynchronously so we can report on its progress
    {
        Sawyer::ThreadWorkers<Work, WorkFunctor> workers;
        workers.start(work, nThreads, WorkFunctor());
        while (!workers.isFinished()) {
            size_t nItemsFinished = workers.nFinished();
            std::pair<size_t, size_t> nw = workers.nWorkers();

            boost::unique_lock<boost::mutex> lock(outputMutex);
            std::cerr <<"*** Still working: "
                      <<(100.0*nItemsFinished/work.nVertices()) <<"% completed, "
                      <<nw.second <<" of " <<nw.first <<" workers busy\n";

            lock.unlock();
            boost::this_thread::sleep(boost::posix_time::milliseconds(250));
        }
        workers.wait();
    }

#elif 0 // [Robb Matzke 2015-11-12]
    // Make sure destructor works
    {
        Sawyer::ThreadWorkers<Work, WorkFunctor> workers;
        workers.start(work, nThreads, WorkFunctor());
        // destructor should block until work is done
    }
#endif

}

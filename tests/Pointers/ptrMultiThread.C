#include <sawyer/SharedPointer.h>
#include <sawyer/Stopwatch.h>

#include <boost/thread.hpp>
#include <iostream>
#include <stdlib.h>

static const size_t MAX_THREADS = 8;

// Some small object that does no work
struct Fast: Sawyer::SharedObject {
    size_t x;
    Fast(): x(0) {}
    void work(size_t n) {}
};

// Some small object that does some work
struct Slow: Sawyer::SharedObject {
    size_t x;
    Slow(): x(0) {}
    void work(size_t n) {
        for (size_t i=0; i<1000*n; ++i)
             x += i;
    }
};

template<class Object>
struct Worker {
    typedef Sawyer::SharedPointer<Object> Ptr;
    const std::vector<Ptr> &objects;
    std::vector<Ptr> pointers;
    const size_t nIterations;

    Worker(const std::vector<Ptr> &objects, size_t nIterations)
        : objects(objects), pointers(objects), nIterations(nIterations) {}

    void operator()() {
        // Assign objects pointers randomly and do some work between each
        for (size_t i=0; i<nIterations; ++i) {
            size_t src = rand() % objects.size();
            size_t dst = rand() % pointers.size();
            pointers[dst] = objects[src];
            pointers[dst]->work(dst);
        }
    }
};

template<class Object>
static void
test(size_t nthreads, size_t nObjects, size_t nIterations) {
    ASSERT_require(nthreads>0);
    ASSERT_require(nthreads <= MAX_THREADS);
    boost::thread threads[MAX_THREADS];
    std::cout <<nthreads <<(1==nthreads?" thread":" threads") <<"\n";

    // Allocate a build a bunch of objects
    typedef Sawyer::SharedPointer<Object> Ptr;
    std::vector<Ptr> objects(nObjects);
    for (size_t i=0; i<nObjects; ++i)
        objects[i] = Ptr(new Object);

    // Each worker will also have its own list of pointers to those objects, copied by the thread's c'tor
    Worker<Object> worker(objects, nIterations);

    Sawyer::Stopwatch timer;
    for (size_t i=0; i<nthreads; ++i)
        threads[i] = boost::thread(worker);
    for (size_t i=0; i<nthreads; ++i)
        threads[i].join();
    timer.stop();

    std::cout <<"  elapsed time: " <<timer <<" seconds\n";
    std::cout <<"  " <<(nIterations*nthreads / timer.report()) <<" operations per second\n";

    worker.pointers.clear();                            // now the ref counts should all be one again
    for (size_t i=0; i<nObjects; ++i)
        ASSERT_require(ownershipCount(objects[i]) == 1);
}

int
main() {
    std::cout <<"================== Slow objects (low lock contention) ========================\n";
    for (size_t nthreads=1; nthreads<=MAX_THREADS; ++nthreads)
        test<Slow>(nthreads, 100, 35000);

    std::cout <<"================== Fast objects (high lock contention) =======================\n";
    for (size_t nthreads=1; nthreads<=MAX_THREADS; ++nthreads)
        test<Fast>(nthreads, 100, 11500000); // about 1 second for one thread on a reasonable machine

    std::cout <<"pointer multi-threading tests passed\n";
}

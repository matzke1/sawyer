#include <Sawyer/PoolAllocator.h>
#include <Sawyer/SmallObject.h>
#include <Sawyer/Stopwatch.h>

#include <boost/version.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <stdlib.h>

static const size_t MAX_THREADS = 8;

// Some small object that does no work
struct Fast: Sawyer::SmallObject {
    size_t x;
    Fast(): x(0) {}
    void work(size_t /*n*/) {}
};

// Some small object that does some work
struct Slow: Sawyer::SmallObject {
    size_t x;
    Slow(): x(0) {}
    void work(size_t n) {
        for (size_t i=0; i<1000*n; ++i)
             x += i;
    }
};

template<class Object>
struct Worker {
    const size_t nObjects;
    const size_t nIterations;

    Worker(size_t nObjects, size_t nIterations)
        : nObjects(nObjects), nIterations(nIterations) {}

    void operator()() {
        std::vector<Object*> objects(nObjects, NULL);
        for (size_t iter=0; iter<nIterations; ++iter) {
            for (size_t i=0; i<nObjects; ++i) {
                size_t j = Sawyer::fastRandomIndex(nObjects);
                delete objects[j];
                objects[j] = new Object;
                objects[j]->work(i);
            }
        }

        for (size_t i=0; i<nObjects; ++i)
            delete objects[i];
    }
};

template<class Object>
static void
test(Worker<Object> &worker, size_t nthreads) {
    ASSERT_require(nthreads>0);
    ASSERT_require(nthreads <= MAX_THREADS);
    boost::thread threads[MAX_THREADS];
    std::cout <<nthreads <<(1==nthreads?" thread":" threads") <<"\n";

    // Optional: make a hot tub (warm up the pool)
    size_t highWater = 2 * worker.nObjects * nthreads;
    Object::poolAllocator().reserve(sizeof(Object), highWater);

    Sawyer::Stopwatch timer;
    for (size_t i=0; i<nthreads; ++i)
        threads[i] = boost::thread(worker);
    for (size_t i=0; i<nthreads; ++i)
        threads[i].join();
    timer.stop();

    std::cout <<"  elapsed time: " <<timer <<" seconds\n";
    std::cout <<"  " <<((worker.nObjects*worker.nIterations*nthreads) / timer.report()) <<" allocations per second\n";
    Object::poolAllocator().vacuum();
    std::ostringstream stats;
    Object::poolAllocator().showInfo(stats);
    if (!stats.str().empty()) {
        std::cout <<stats.str();
        ASSERT_always_require(stats.str().empty());
    }
}

int
main() {
    Sawyer::initializeLibrary();

    // This test should show that the allocations per second scales nearly linearly with the number of threads, but
    // since the lock contention is low this doesn't do much stress testing.
    std::cout <<"================== Slow objects (low lock contention) ========================\n";
    Worker<Slow> slowWorker(1845, 1); // about one second for one thread on a reasonably fast machine
    for (size_t nthreads=1; nthreads<=MAX_THREADS; ++nthreads)
        test(slowWorker, nthreads);

    // This is more of a stress test; the number of allocations per second should level off, but will be slower
    // with more threads than with few threads.
    std::cout <<"================== Fast objects (high lock contention) =======================\n";
    Worker<Fast> fastWorker(1000000, 7); // about one second for one thread on a reasonably fast machine
    for (size_t nthreads=1; nthreads<=MAX_THREADS; ++nthreads)
        test(fastWorker, nthreads);

    std::cout <<"pool multi-threading tests passed\n";
}

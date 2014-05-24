#include <sawyer/SmallObject.h>

#include <sawyer/Assert.h>
#include <sawyer/Stopwatch.h>

struct Object1 {
    int a, b, c;
};

struct Object2: Sawyer::SmallObject {
    int a, b, c;
};

int main() {
    static const size_t nObjects = 1000000;
    static const size_t nIterations = 50000000;

#if 0 // [Robb Matzke 2014-05-24]
    typedef Object1 Object;
#else
    typedef Object2 Object;
#endif

    std::vector<Object*> objects(nObjects, NULL);

    Sawyer::Stopwatch stopwatch;
    size_t nalloc=0, nfree=0;
    for (size_t iter=0; iter<nIterations; ++iter) {
        size_t i = rand() % nObjects;
        if (objects[i]) {
            delete objects[i];
            objects[i] = NULL;
            ++nfree;
        } else {
            objects[i] = new Object;
            ASSERT_always_not_null(objects[i]);
            ++nalloc;
        }
    }
    double elapsed = stopwatch.stop();

    std::cout <<"allocations:   " <<nalloc <<"\n";
    std::cout <<"deallocations: " <<nfree <<"\n";
    std::cout <<"elapsed:       " <<elapsed <<" seconds\n";
    std::cout <<"rate:          " <<((nalloc+nfree)/elapsed*1e-6) <<" MOPS\n";
}

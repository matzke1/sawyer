#include <sawyer/PoolAllocator.h>

#include <iostream>
#include <sawyer/Assert.h>
#include <sawyer/Message.h>
#include <sawyer/ProgressBar.h>
#include <sawyer/Stopwatch.h>
#include <stdlib.h>

struct Object {
    void *address;
    size_t size;
    Object(): address(0), size(0) {}
    Object(void *address, size_t size): address(address), size(size) {}
};

struct SystemAllocator {
    void *allocate(size_t size) {
        return ::operator new(size);
    }
    void deallocate(void *addr, size_t size) {
        return ::operator delete(addr);
    }
};

int main() {

    static const size_t nObjects = 1000000;
    static const size_t nIterations = 50000000;
    static const size_t maxSize = 128;

    std::vector<Object> objects(nObjects);

#if 1 // [Robb Matzke 2014-05-24]
    typedef Sawyer::PoolAllocator Allocator;
#else
    typedef SystemAllocator Allocator;
#endif
    Allocator allocator;

    // Allocate and deallocate at random
    Sawyer::Stopwatch stopwatch;
    size_t nalloc=0, nfree=0;
    for (size_t iter=0; iter<nIterations; ++iter) {
        size_t i = rand() % nObjects;
        if (objects[i].address) {
            allocator.deallocate(objects[i].address, objects[i].size);
            objects[i] = Object();
            ++nfree;
        } else {
            objects[i].size = rand() % maxSize + 1;
            objects[i].address = allocator.allocate(objects[i].size);
            ASSERT_always_not_null(objects[i].address);
            ++nalloc;
        }
    }
    double elapsed = stopwatch.stop();
    std::cout <<"allocations:   " <<nalloc <<"\n";
    std::cout <<"deallocations: " <<nfree <<"\n";
    std::cout <<"elapsed:       " <<elapsed <<" seconds\n";
    std::cout <<"rate:          " <<((nalloc+nfree)/elapsed*1e-6) <<" MOPS\n";

#if 0 // [Robb Matzke 2014-05-27]
    // Deallocate everything
    for (size_t i=0; i<objects.size(); ++i) {
        if (objects[i].address!=NULL) {
            allocator.deallocate(objects[i].address, objects[i].size);
            objects[i] = Object();
            ++nfree;
        }
    }
    allocator.vacuum();
    allocator.showInfo(std::cout);
#endif
}

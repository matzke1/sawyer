#ifndef Sawyer_PoolAllocator_H
#define Sawyer_PoolAllocator_H

#include <boost/static_assert.hpp>
#include <list>
#include <sawyer/Assert.h>

namespace Sawyer {

/** Small object allocation from memory pools.
 *
 *  This class manages allocation and deallocation of small objects from pools. This allocator has pools available for a
 *  variety of small object sizes, or falls back to the global <code>new</code> and <code>delete</code> operators for larger
 *  objects.  Each pool contains zero or more large chunks of contiguous memory from which storage for the small objects are
 *  obtained. The following template parameters control the number and sizes of pools:
 *
 *  @li @p smallestCell is the size in bytes of the smallest cells.  An "cell" is a small unit of storage and may be larger
 *      than what is requested when allocating memory for an object.  The value must be at least as large as a pointer.
 *      Memory requests that are smaller than a cell lead to internal fragmentation.
 *  @li @p sizeDelta is the difference in size in bytes between the cells of two neighboring pools.  The value must be
 *      positive. When looking for a pool that will satisfy an allocation request, the allocator chooses the pool having the
 *      smallest cells that are at least as large as the request.
 *  @li @p nPools is the total number of pools, each having a different size of cells. If pools are numbered zero through
 *      @em n then the size of cells is \f$\|cell_i\| = \|cell_0\| + i \Delta\f$
 *  @li @p chunkSize is the size of each chunk in bytes.  Chunks are the unit of allocation requested from the runtime. All
 *      chunks in the allocator are the same size regardless of the cell size, and this may lead to external
 *      fragmentation&mdash;extra space at the end of a chunk that is not large enough for a complete cell.  The chunk size
 *      should be large in relation to the largest cell size (that's the whole point of pool allocation).
 *
 *  The @ref PoolAllocator typedef provides reasonable template arguments.
 *
 *  Pool allocators cannot be copied. Deleting a pool allocator deletes all its pools, which deletes all the chunks, which
 *  deallocates memory that might be in use by objects allocated from this allocator.  In other words, don't destroy the
 *  allocator unless you're willing that the memory for any objects in use will suddenly be freed without even calling the
 *  destructors for those objects. */
template<size_t smallestCell, size_t sizeDelta, size_t nPools, size_t chunkSize>
class PoolAllocatorBase {
private:

    // Singly-linked list of cells (units of object backing store) that are not being used by the caller.
    struct FreeCell { FreeCell *next; };

    // Basic unit of allocation.
    class Chunk {
        unsigned char data_[chunkSize];
    public:
        BOOST_STATIC_ASSERT(chunkSize >= sizeof(FreeCell));

        FreeCell* fill(size_t cellSize) {               // create a free list for this chunk
            ASSERT_require(cellSize >= sizeof(FreeCell));
            ASSERT_require(cellSize <= chunkSize);
            FreeCell *retval = NULL;
            size_t nCells = chunkSize / cellSize;
            for (size_t i=nCells; i>0; --i) {           // free list in address order is easier to debug at no extra expense
                FreeCell *cell = reinterpret_cast<FreeCell*>(data_+(i-1)*cellSize);
                cell->next = retval;
                retval = cell;
            }
            return retval;
        }
    };

    // Manages a list of chunks for cells that are all the same size.
    class Pool {
        size_t cellSize_;
        FreeCell *freeList_;
        std::list<Chunk*> chunks_;
    public:
        Pool(): cellSize_(0), freeList_(NULL) {}        // needed by std::vector
        Pool(size_t cellSize): cellSize_(cellSize), freeList_(NULL) {}

    public:
        ~Pool() {
            for (typename std::list<Chunk*>::iterator ci=chunks_.begin(); ci!=chunks_.end(); ++ci)
                delete *ci;
        }

        // Obtains the cell at the front of the free list, allocating more space if necessary.
        void* aquire() {                                // hot
            if (!freeList_) {
                Chunk *chunk = new Chunk;
                chunks_.push_back(chunk);
                freeList_ = chunk->fill(cellSize_);
            }
            ASSERT_not_null(freeList_);
            FreeCell *cell = freeList_;
            freeList_ = freeList_->next;
            cell->next = NULL;                          // optional
            return cell;
        }

        // Returns an cell to the front of the free list.
        void release(void *cell) {                      // hot
            ASSERT_not_null(cell);
            FreeCell *freedCell = reinterpret_cast<FreeCell*>(cell);
            freedCell->next = freeList_;
            freeList_ = freedCell;
        }
    };

private:
    std::vector<Pool> pools_;

    static size_t poolNumber(size_t size) {             // maps request size to pool number, assuming infinite number of pools
        return (size - smallestCell - 1) / sizeDelta;
    }

    static size_t poolSize(size_t poolNumber) {
        return smallestCell + poolNumber * sizeDelta;
    }

    void init() {
        pools_.reserve(nPools);
        for (size_t i=0; i<nPools; ++i)
            pools_.push_back(Pool(poolSize(i)));
    }

public:
    /** Default constructor. */
    PoolAllocatorBase() {
        init();
    }

private:
    // Pool allocators are not copyable.
    PoolAllocatorBase(const PoolAllocatorBase&);
    PoolAllocatorBase& operator=(const PoolAllocatorBase&);

public:
    /** Destructor.
     *
     *  Destroying a pool allocator destroys all its pools, which means that any objects that use storage managed by this pool
     *  will have their storage deleted. */
    virtual ~PoolAllocatorBase() {}

    /** Allocate one object of specified size.
     *
     *  Allocates one cell from an allocation pool, using the pool with the smallest-sized cells that are large enough to
     *  satisfy the request.  If the request is larger than that available from any pool then the global "new" operator is
     *  used.
     *
     *  The requested size must be positive. */
    void *allocate(size_t size) {                       // hot
        ASSERT_require(size>0);
        size_t pn = poolNumber(size);
        return pn < nPools ? pools_[pn].aquire() : ::operator new(size);
    }

    /** Deallocate an object of specified size.
     *
     *  The @p addr must be an object address that was previously returned by the @ref allocate method and which hasn't been
     *  deallocated in the interim.  The @p size must be the same as the argument passed to the @ref allocate call that
     *  returned this address. */
    void deallocate(void *addr, size_t size) {          // hot
        ASSERT_not_null(addr);
        ASSERT_require(size>0);
        size_t pn = poolNumber(size);
        if (pn < nPools) {
            pools_[pn].release(addr);
        } else {
            ::operator delete(addr);
        }
    }
};

/** Small object allocation from memory pools.
 *
 *  See @ref PoolAllocatorBase for details. */
typedef PoolAllocatorBase<sizeof(void*), 4, 32, 40960> PoolAllocator;

} // namespace
#endif

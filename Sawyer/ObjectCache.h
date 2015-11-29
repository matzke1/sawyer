#ifndef Sawyer_ObjectCache_H
#define Sawyer_ObjectCache_H

#ifdef SAWYER_MULTI_THREADED
#include <boost/thread/condition_variable.hpp>
#endif

namespace Sawyer {

class CachableObject;

/** Cache for objects.
 *
 *  A cache manages objects and decides when an object should reduce its memory footprint. An object can be managed by zero or
 *  one cache. When the cache decides that one if its managed object hasn't been used for a while, it asynchronously invokes
 *  that object's @ref CachableObject::evict "evict" method.  The object should respond by deleting any large internal state if
 *  possible.  See also, @ref CachableObject, from which all cache-managed objects must inherit.
 *
 *  When an object is deleted, usually because its reference count reaches zero, it erases itself from any cache that owns
 *  it. Being a member of a cache does not count toward an object's reference count. */
class SAWYER_EXPORT ObjectCache {
    mutable SAWYER_THREAD_TRAITS::Mutex mutex_;         // Protects the circular list, hand_
    CachableObject *hand_;                              // Circular list of owned objects
    size_t nObjects_;                                   // Number of objects in circular list
#ifdef SAWYER_MULTI_THREADED
    boost::thread evictionThread_;                      // the thread that calls the objects' evict method
    boost::condition_variable objectListChanged_;       // signaled whenever the eviction thread should immediately wake up.
#endif
    boost::posix_time::milliseconds evictionWakeup_;    // how long the eviction thread should sleep
    size_t evictionRatioDenominator_;                   // reciprocal of the ratio of the list to process each time
    bool pleaseExit_;                                   // when true, the eviction thread will terminate

public:
    /** Constructor.
     *
     *  Constructs a new object cache and optionally starts the eviction thread.  In a single threaded environment (%Sawyer
     *  configured without multi-thread support, or when @p useEvictionThread is false, it is up to the user to explicitly call
     *  @ref runEviction periodically. */
    explicit ObjectCache(bool useEvictionThread = true)
        : hand_(NULL), nObjects_(0), evictionWakeup_(10000), evictionRatioDenominator_(10), pleaseExit_(false) {
        init(useEvictionThread);
    }

    /** Destructor.
     *
     *  Destroying a cache removes all objects from the cache. */
    ~ObjectCache();

    /** Insert an object into this cache.
     *
     *  The object is inserted into the cache. An object can belong to at most one cache. If @p obj is already in this cache
     *  then this is a no-op; if the object is in some other cache then an @c std::runtime_error is thrown.
     *
     *  @{ */
    void insert(const SharedPointer<CachableObject> &obj);
    void insert(CachableObject *obj);
    /** @} */

    /** Erase an object from this cache.
     *
     *  The specified object is erased from this cache. This is a no-op if the object belongs to no cache; an @c
     *  std::runtime_error is thrown if the object belongs to some other cache.
     *
     *  @{ */
    void erase(const SharedPointer<CachableObject> &obj);
    void erase(CachableObject *obj);
    /** @} */

    /** Determine if object belongs to this cache.
     *
     *  Returns true if the object belongs to this cache, false otherwise.
     *
     * @{ */
    bool exists(const SharedPointer<CachableObject> &obj) const;
    bool exists(const CachableObject *obj) const;
    /** @} */

    /** How often the eviction process runs.
     *
     *  The eviction thread indefinitely repeats processing the object list and sleeping. This property is the number of
     *  milliseconds that the thread sleeps. Changing the property causes the thread to immediately wake up.
     *
     * @{ */
    void evictionWakeup(boost::posix_time::milliseconds);
    boost::posix_time::milliseconds evictionWakeup() const;
    /** @} */

    /** How much of the list to process.
     *
     *  Each time the eviction thread wakes up it processes 1/Nth of its object list, incrementing a counter for each object
     *  that's processed. This counter is reset to zero by the object's @ref CachableObject::touched "touched" method, and when
     *  the counter reaches two the object's @ref CachableObject::evict "evict" method is invoked.  Changing the value of N
     *  does not cause the eviction process to immediately wake up.
     *
     * @{ */
    void evictionListRatio(size_t denominator);
    size_t evictionListRatio() const;
    /** @} */

    /** Run one iteration of the eviction algorithm.
     *
     *  This is intended mostly for single-threaded programs where the user controls when the eviction algorithm runs. */
    void runEviction();


public:
    // Internal: Eviction worker thread. This thread wakes up every so often and looks for objects that should be evicted.
    void evictContinuously();

private:
    // Called by constructor
    void init(bool useEvictionThread);

    // Verify that the object belongs to this cache.
    void verifyObjectNS(const CachableObject *obj);

    // Run one iteration of the eviction algorithm.
    void runEvictionNS(SAWYER_THREAD_TRAITS::UniqueLock &lock);
};


} // namespace

#endif

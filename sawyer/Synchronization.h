#ifndef Sawyer_Synchronization_H
#define Sawyer_Synchronization_H

namespace Sawyer {

/** Tag indicating that an algorithm or API should assume multiple threads.
 *
 *  Indicating that multiple threads are present by the use of this tag does not necessarily ensure that the affected algorithm
 *  or API is completely thread-safe. The alrogihm or API's documentation will expound on the details. */
struct MultiThreadedTag {};

/** Tag indicating that an algorithm or API can assume only a single thread.
 *
 *  This typically means that the algorithm or API will not perform any kind of synchronization itself, but requires that the
 *  callers coordinate to serialize calls to the algorithm or API. */
struct SingleThreadedTag {};

// Used internally as a mutex in a single-threaded environment.
class NullMutex {
public:
    void lock() {}
    void unlock() {}
    bool try_lock() { return true; }
};

// Used internally as a lock guard in a single-threaded environment.
class NullLockGuard {
public:
    NullLockGuard(NullMutex) {}
};

/** Locks multiple mutexes. */
template<typename Mutex>
class LockGuard2 {
    Mutex &m1_, &m2_;
public:
    LockGuard2(Mutex &m1, Mutex &m2): m1_(m1), m2_(m2) {
        boost::lock(m1, m2);
    }
    ~LockGuard2() {
        m1_.unlock();
        m2_.unlock();
    }
};


/** Traits for thread synchronization. */
template<typename SyncTag>
struct SynchronizationTraits {};

template<>
struct SynchronizationTraits<MultiThreadedTag> {
    typedef boost::mutex Mutex;
    typedef boost::shared_mutex SharedMutex;
    typedef boost::lock_guard<boost::mutex> LockGuard;
    typedef boost::lock_guard<boost::shared_mutex> SharedLockGuard;
};

template<>
struct SynchronizationTraits<SingleThreadedTag> {
    typedef NullMutex Mutex;
    typedef NullMutex SharedMutex;
    typedef NullLockGuard LockGuard;
    typedef NullLockGuard SharedLockGuard;
};


} // namespace
#endif

#include <Sawyer/CachableObject.h>
#include <Sawyer/SharedPointer.h>

namespace Sawyer {

static void startEviction(ObjectCache *cache) {
    cache->evictContinuously();
}

void
ObjectCache::init(bool useEvictionThread) {
    if (useEvictionThread)
        evictionThread_ = boost::thread(startEviction, this);
}

ObjectCache::~ObjectCache() {
    while (hand_ != NULL)
        erase(hand_);
    {
        SAWYER_THREAD_TRAITS::LockGuard guard(mutex_);
        pleaseExit_ = true;
    }
    objectListChanged_.notify_all();
    evictionThread_.join();
}

void
ObjectCache::insert(const SharedPointer<CachableObject> &ptr) {
    ASSERT_not_null(ptr);
    return insert(&*ptr);
}

void
ObjectCache::insert(CachableObject *obj) {
    ASSERT_not_null(obj);
    SAWYER_THREAD_TRAITS::LockGuard guard(mutex_);
    if (obj->cache_ == NULL) {
        obj->cache_ = this;
        if (hand_ == NULL) {
            obj->cacheNext_ = obj;
            obj->cachePrev_ = obj;
            hand_ = obj;
        } else {
            obj->cacheNext_ = hand_;
            obj->cachePrev_ = hand_->cachePrev_;
            hand_->cachePrev_->cacheNext_ = obj;
            hand_->cachePrev_ = obj;
        }
        ++nObjects_;
    } else if (obj->cache_ != this) {
        throw std::runtime_error("object belongs to another cache already");
    }
    verifyObjectNS(obj);
}

void
ObjectCache::erase(const SharedPointer<CachableObject> &ptr) {
    ASSERT_not_null(ptr);
    return erase(&*ptr);
}

void
ObjectCache::erase(CachableObject *obj) {
    ASSERT_not_null(obj);
    SAWYER_THREAD_TRAITS::LockGuard guard(mutex_);
    if (obj->CachableObject::cache_ == this) {
        ASSERT_require(nObjects_ > 0);
        verifyObjectNS(obj);
        CachableObject *next = obj->cacheNext_;
        CachableObject *prev = obj->cachePrev_;
        if (next == obj) {
            ASSERT_require(hand_ == obj);
            ASSERT_require(next == prev);
            hand_ = NULL;
        } else {
            next->cachePrev_ = prev;
            prev->cacheNext_ = next;
            if (hand_ == obj)
                hand_ = next;
        }
        obj->cacheNext_ = NULL;
        obj->cachePrev_ = NULL;
        obj->cache_ = NULL;
        --nObjects_;
    } else if (obj->cache_ != NULL) {
        throw std::runtime_error("object belongs to another cache");
    }
}

bool
ObjectCache::exists(const SharedPointer<CachableObject> &ptr) const {
    ASSERT_not_null(ptr);
    return exists(&*ptr);
}

bool
ObjectCache::exists(const CachableObject *obj) const {
    ASSERT_not_null(obj);
    SAWYER_THREAD_TRAITS::LockGuard guard(mutex_);
    return obj->cache_ == this;
}

void
ObjectCache::evictionWakeup(boost::posix_time::milliseconds duration) {
    SAWYER_THREAD_TRAITS::LockGuard guard(mutex_);
    bool changed = evictionWakeup_ != duration;
    evictionWakeup_ = duration;
    if (changed)
        objectListChanged_.notify_one();
}

boost::posix_time::milliseconds
ObjectCache::evictionWakeup() const {
    SAWYER_THREAD_TRAITS::LockGuard guard(mutex_);
    return evictionWakeup_;
}

void
ObjectCache::evictionListRatio(size_t denominator) {
    SAWYER_THREAD_TRAITS::LockGuard guard(mutex_);
    evictionRatioDenominator_ = std::max((size_t)1, denominator);
}

size_t
ObjectCache::evictionListRatio() const {
    SAWYER_THREAD_TRAITS::LockGuard guard(mutex_);
    return evictionRatioDenominator_;
}

void
ObjectCache::verifyObjectNS(const CachableObject *obj) {
    ASSERT_not_null(obj);
    ASSERT_require(obj->cache_ == this);
    ASSERT_not_null(obj->cacheNext_);                   // this is a circular list, so the object at least points to
    ASSERT_not_null(obj->cachePrev_);                   // itself when it belongs to an object cache.
}

void
ObjectCache::runEviction() {
    SAWYER_THREAD_TRAITS::UniqueLock lock(mutex_);
    runEvictionNS(lock);
}

void
ObjectCache::runEvictionNS(SAWYER_THREAD_TRAITS::UniqueLock &lock) {
    size_t n = std::max((size_t)1, nObjects_/evictionRatioDenominator_);
    for (size_t i=0; i<n && hand_; ++i) {
        if (++hand_->cacheTimer_ == 2) {
            CachableObject *obj = hand_;
            hand_ = hand_->cacheNext_;
            lock.unlock(); {
                obj->evict();
            } lock.lock();
        }
    }
}

void
ObjectCache::evictContinuously() {
#ifdef SAWYER_MULTI_THREADED
    SAWYER_THREAD_TRAITS::UniqueLock lock(mutex_);
    while (1) {
        if (!pleaseExit_)
            objectListChanged_.timed_wait(lock, evictionWakeup_);
        if (pleaseExit_) {
            return;
        } else if (hand_) {
            runEvictionNS(lock);
        }
    }
#endif
}

} // namespace

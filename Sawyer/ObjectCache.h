#ifndef Sawyer_ObjectCache_H
#define Sawyer_ObjectCache_H

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
    CachableObject *hand_;                              // Circular list of owned objects

public:
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

private:
    // Verify that the object belongs to this cache.
    void verifyObject(const CachableObject *obj);

    // Eviction worker thread. This thread wakes up every so often and looks for objects that should be evicted.
    void evictContinuously() {
        TODO("[Robb Matzke 2015-11-27]");
    }
};


} // namespace

#endif

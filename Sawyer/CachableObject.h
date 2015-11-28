#ifndef Sawyer_CachableObject_H
#define Sawyer_CachableObject_H

#include <Sawyer/Sawyer.h>
#include <Sawyer/Assert.h>
#include <Sawyer/SharedPointer.h>
#include <Sawyer/ObjectCache.h>

namespace Sawyer {


/** Interface for cachable objects.
 *
 *  Any object which is cachable should inherit directly or indirectly from this class, which is itself a subclass of @ref
 *  SharedObject. The user's object should not explicitly inherit from @ref SharedObject.
 *
 *  If a CachableObject belongs to a cache (see @ref ObjectCache::insert) then the cache may occassionally invoke the object's
 *  @ref evict method, informing the object that it should decrease its memory footprint by deleting (after possibely saving)
 *  large data members. In order to prevent the cache from calling @ref evict during and shortly after times when the large
 *  data members are being used, any access to those data members should call the object's @ref touched method.  The object is
 *  responsible for restoring or regenerating as necessary its large data members whenever a caller needs them. */
class SAWYER_EXPORT CachableObject: public SharedObject {
    friend class ObjectCache;
    ObjectCache *cache_;
    CachableObject *cacheNext_, *cachePrev_;
    int cacheTimer_;
public:
    /** Default constructor.
     *
     *  A default-constructed cachable object does not belong to any cache. It must be explicitly inserted into a cache in
     *  order for it to be managed by a cache and have its @ref evict method called by that cache. */
    CachableObject()
        : cache_(NULL), cacheNext_(NULL), cachePrev_(NULL), cacheTimer_(0) {}

    /** Destructor.
     *
     *  When the object is destroyed it's cache (if any) is notified so it can be removed from the cache. */
    ~CachableObject() {
        if (cache_)
            cache_->erase(this);
    }

    /** Copy constructor.
     *
     *  It is permissible to copy one cachable object to another. The @p other @ref restore method is not invoked during the
     *  copy because it has presumably already been restored due to its pointer having been dereferenced in order to pass it to
     *  this constructor. */
    CachableObject(const CachableObject &other)
        : SharedObject(other), cache_(NULL), cacheNext_(NULL), cachePrev_(NULL), cacheTimer_(0) {
    }

    /** Assignment operator.
     *
     *  Assigning a source object to a cachable destination object does not change whether either object is cached. The @ref
     *  restore method of the @p other object is not called first since that presumably has already occurred by dereferencing
     *  a pointer in order pass a reference to this method. */
    CachableObject& operator=(const CachableObject &other) {
        SharedObject::operator=(other);
        cacheTimer_ = 0;
        return *this;
    }

    /** Place object into evicted state if possible.
     *
     *  Causes this object's large data members to be deleted, perhaps after storing them somewhere first. Returns true if the
     *  eviction is allowed and happened, otherwise returns false.  Since eviction happens asynchronously, the object should
     *  take care that it doesn't evict itself while something is using it.  For instance, if an object returns a reference to
     *  some internal state, then it should provide some mechanism by which the user of that state can indicate when they're
     *  finished, before which time this @ref evict method should return false. */
    virtual bool evict() = 0;

    /** Notify cache that object has been used.
     *
     *  Invoking this method informs this object's cache (if any) that the object has been recently used. This should be called
     *  whenever evictable data members are accessed. Failing to call this might result in the cache calling the @ref evict
     *  method when large data members are being actively used, resulting in the object thrashing as its evicted and then
     *  almost immediately restored, over and over. */
    void touched() {
        cacheTimer_ = 0;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


} // namespace

#endif

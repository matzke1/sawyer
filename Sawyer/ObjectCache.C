#include <Sawyer/CachableObject.h>
#include <Sawyer/SharedPointer.h>

namespace Sawyer {

ObjectCache::~ObjectCache() {
    while (hand_ != NULL)
        erase(hand_);
    TODO("kill eviction thread");
}

void
ObjectCache::insert(const SharedPointer<CachableObject> &ptr) {
    ASSERT_not_null(ptr);
    return insert(&*ptr);
}

void
ObjectCache::insert(CachableObject *obj) {
    ASSERT_not_null(obj);
    if (obj->CachableObject::cache_ == NULL) {
        obj->CachableObject::cache_ = this;
        if (hand_ == NULL) {
            obj->CachableObject::cacheNext_ = obj;
            obj->CachableObject::cachePrev_ = obj;
            hand_ = obj;
        } else {
            obj->CachableObject::cacheNext_ = hand_;
            obj->CachableObject::cachePrev_ = hand_->CachableObject::cachePrev_;
            hand_->CachableObject::cachePrev_->CachableObject::cacheNext_ = obj;
            hand_->CachableObject::cachePrev_ = obj;
        }
    } else if (obj->CachableObject::cache_ != this) {
        throw std::runtime_error("object belongs to another cache already");
    }
    verifyObject(obj);
}

void
ObjectCache::erase(const SharedPointer<CachableObject> &ptr) {
    ASSERT_not_null(ptr);
    return erase(&*ptr);
}

void
ObjectCache::erase(CachableObject *obj) {
    ASSERT_not_null(obj);
    if (obj->CachableObject::cache_ == this) {
        verifyObject(obj);
        CachableObject *next = obj->CachableObject::cacheNext_;
        CachableObject *prev = obj->CachableObject::cachePrev_;
        if (next == obj) {
            ASSERT_require(hand_ == obj);
            ASSERT_require(next == prev);
            hand_ = NULL;
        } else {
            next->CachableObject::cachePrev_ = prev;
            prev->CachableObject::cacheNext_ = next;
            if (hand_ == obj)
                hand_ = next;
        }
        obj->CachableObject::cacheNext_ = NULL;
        obj->CachableObject::cachePrev_ = NULL;
        obj->CachableObject::cache_ = NULL;
    } else if (obj->CachableObject::cache_ != NULL) {
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
    return obj->CachableObject::cache_ == this;
}

void
ObjectCache::verifyObject(const CachableObject *obj) {
    ASSERT_not_null(obj);
    ASSERT_require(obj->CachableObject::cache_ == this);
    ASSERT_not_null(obj->CachableObject::cacheNext_);// this is a circular list, so the object at least points to
    ASSERT_not_null(obj->CachableObject::cachePrev_);// itself when it belongs to an object cache.
}


} // namespace

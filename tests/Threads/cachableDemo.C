#include <Sawyer/CachableObject.h>
#include <Sawyer/Synchronization.h>

static Sawyer::ObjectCache theCache;

class MyObject: public Sawyer::CachableObject {
public:
    typedef Sawyer::SharedPointer<MyObject> Ptr;

private:
    mutable SAWYER_THREAD_TRAITS::Mutex mutex_;         // protects the following data members
    size_t size_;
    int *bigData_;                                      // deleted when object is in evicted state

protected:
    MyObject()
        : size_(0), bigData_(NULL) {}

    explicit MyObject(size_t size)
        : size_(size), bigData_(NULL) {
        restoreNS();
    }

public:
    static Ptr instance(size_t n) {
        Ptr p(new MyObject(n));
        theCache.insert(p);
        return p;
    }

    virtual bool evict() /*override*/ {
        SAWYER_THREAD_TRAITS::LockGuard guard(mutex_);
        delete[] bigData_;
        bigData_ = NULL;
        return true;
    }

    int operator[](size_t i) {
        SAWYER_THREAD_TRAITS::LockGuard guard(mutex_);
        if (bigData_ == NULL)
            restoreNS();
        touched();
        return bigData_[i];
    }
private:
    virtual void restoreNS() {
        bigData_ = new int[size_];
        if (size_ >= 1)
            bigData_[0] = 1;
        if (size_ >= 2)
            bigData_[1] = 1;
        for (size_t i=2; i<size_; ++i)
            bigData_[i] = bigData_[i-2] + bigData_[i-1];
    }
};


int
main() {
    MyObject::Ptr m1 = MyObject::instance(100);
}


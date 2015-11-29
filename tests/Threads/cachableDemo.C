#include <Sawyer/CachableObject.h>
#include <Sawyer/Synchronization.h>

#include <iostream>

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
        std::cerr <<"Evicting large data members\n";
        delete[] bigData_;
        bigData_ = NULL;
        return true;
    }

    bool isEvicted() const {
        SAWYER_THREAD_TRAITS::LockGuard guard(mutex_);
        return bigData_ == NULL;
    }
    
    int get(size_t i) {
        SAWYER_THREAD_TRAITS::LockGuard guard(mutex_);
        if (bigData_ == NULL)
            restoreNS();
        touched();
        return bigData_[i];
    }
private:
    virtual void restoreNS() {
        std::cerr <<"Generating large data members\n";
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
    // Create an object with some large data. It is immediately subject to eviction, but the default eviction timeout is quite
    // long, so it won't happen within five seconds.
    std::cerr <<"created m1\n";
    MyObject::Ptr m1 = MyObject::instance(100);
    boost::this_thread::sleep(boost::posix_time::seconds(5));
    ASSERT_forbid(m1->isEvicted());

    // Turn up the rate at which things are evicted.  We'll make the evictor wake up every couple seconds and process it's
    // entire list of objects each time.
    std::cerr <<"speeding up eviction thread...\n";
    theCache.evictionListRatio(1);
    theCache.evictionWakeup(boost::posix_time::milliseconds(2000));
    boost::this_thread::sleep(boost::posix_time::seconds(8));
    ASSERT_require(m1->isEvicted());

    // Accessing the object's large data (which has been evicted by now) will cause it to be regenerated.
    std::cerr <<"reading data...\n";
    m1->get(10);
    ASSERT_forbid(m1->isEvicted());

    // Sleeping for a while will cause it to be evicted again.
    std::cerr <<"sleeping...\n";
    boost::this_thread::sleep(boost::posix_time::seconds(8));
    ASSERT_require(m1->isEvicted());
}


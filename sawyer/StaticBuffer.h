#ifndef Sawyer_StaticBuffer_H
#define Sawyer_StaticBuffer_H

#include <sawyer/Assert.h>
#include <sawyer/Buffer.h>
#include <sawyer/Sawyer.h>

namespace Sawyer {
namespace Container {

/** Points to static data.
 *
 *  This buffer object points to storage which is not owned by this object and which is therefore not deleted when this object
 *  is deleted. */
template<class A, class T>
class StaticBuffer: public Buffer<A, T> {
public:
    typedef A Address;
    typedef T Value;

private:
    Value *values_;
    Address size_;

protected:
    StaticBuffer(Value *values, Address size): values_(values), size_(size) {
        ASSERT_require(size==0 || values!=NULL);
    }

public:
    /** Construct from caller-supplied data.
     *
     *  The caller supplies a pointer to data and the size of that data.  The new buffer object does not take ownership of the
     *  data or copy it, thus the caller-supplied data must continue to exist for as long as this buffer exists. */
    static typename Buffer<A, T>::Ptr instance(Value *values, Address size) {
        return typename Buffer<A, T>::Ptr(new StaticBuffer(values, size));
    }

    Address available(Address start) const /*override*/ {
        return start < size_ ? size_-start : 0;
    }

    void resize(Address newSize) /*override*/ {
        if (newSize != size_)
            throw std::runtime_error("unable to resize StaticBuffer");
    }

    Address read(Value *buf, Address address, Address n) const /*override*/ {
        n = std::min(n, available(address));
        if (buf)
            memcpy(buf, values_+address, n*sizeof(values_[0]));
        return n;
    }

    Address write(const Value *buf, Address address, Address n) /*override*/ {
        n = std::min(n, available(address));
        if (buf)
            memcpy(values_+address, buf, n*sizeof(values_[0]));
        return n;
    }
};

} // namespace
} // namespace

#endif

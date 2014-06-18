#ifndef Sawyer_AllocatingBuffer_H
#define Sawyer_AllocatingBuffer_H

#include <sawyer/Buffer.h>
#include <sawyer/Sawyer.h>
#include <vector>

namespace Sawyer {
namespace Container {

/** Allocates memory as needed.
 *
 *  Allocates as many elements as requested and manages the storage.  There is no requirement that the elements be contiguous
 *  in memory, although this implementation does it that way. */
template<class A, class T>
class AllocatingBuffer: public Buffer<A, T> {
public:
    typedef A Address;                                  /** Type of addresses used to index the stored data. */
    typedef T Value;                                    /** Type of data that is stored. */

private:
    Address size_;
    std::vector<Value> values_;

protected:
    AllocatingBuffer(Address size): size_(size), values_(size) {}

public:
    /** Allocating constructor.
     *
     *  Allocates a new buffer of the specified size.  The values in the buffer are default-constructed, and deleted when this
     *  buffer is deleted. */
    static typename Buffer<A, T>::Ptr instance(Address size) {
        return typename Buffer<A, T>::Ptr(new AllocatingBuffer(size));
    }

    Address available(Address start) const /*override*/ {
        return start < size_ ? size_-start : 0;
    }

    void resize(Address newSize) /*override*/ {
        values_.resize(newSize);
    }

    Address read(Value *buf, Address address, Address n) const /*override*/ {
        n = std::min(n, available(address));
        if (buf && n>0)
            memcpy(buf, &values_[address], n*sizeof(values_[0]));
        return n;
    }

    Address write(const Value *buf, Address address, Address n) /*override*/ {
        n = std::min(n, available(address));
        if (buf && n>0)
            memcpy(&values_[address], buf, n*sizeof(values_[0]));
        return n;
    }
};

} // namespace
} // namespace

#endif

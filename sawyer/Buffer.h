#ifndef Sawyer_Buffer_H
#define Sawyer_Buffer_H

#include <boost/shared_ptr.hpp>
#include <string>

namespace Sawyer {
namespace Container {

/** Base class for all buffers.
 *
 *  A buffer stores a sequence of elements somewhat like a vector, but often associated with a file. */
template<class A, class T>
class Buffer {
    std::string name_;
protected:
    Buffer() {}
public:
    typedef boost::shared_ptr<Buffer> Ptr;

    /** Key type for addressing data.
     *
     *  The key type should be an unsigned integral type and is the type used to index the data. */
    typedef A Address;

    /** Type of values stored in the buffer. */
    typedef T Value;

    virtual ~Buffer() {}

    /** Distance to end of buffer.
     *
     *  The distance in units of @ref Value from the specified address (inclusive) to the last element of the buffer
     *  (inclusive).  If the address is beyond the end of the buffer then a distance of zero is returned rather than a negative
     *  distance. */
    virtual Address available(Address address) const = 0;
    
    /** Size of buffer.
     *
     *  Returns the number of @ref Value elements stored in the buffer. In other words, this is the first address beyond the
     *  end of the buffer. */
    virtual Address size() const { return available(Address(0)); }

    /** Change the size of the buffer.
     *
     *  Truncates the buffer to a smaller size, or extends the buffer as necessary to make its size @p n values. */
    virtual void resize(Address n) = 0;

    /** Synchronize buffer with persistent storage.
     *
     *  If the buffer is backed by a file of some sort, then synchronize the buffer contents with the file by writing to the
     *  file any buffer values that have changed. */
    virtual void sync() {}

    /** Property: Name.
     *
     *  An arbitrary string stored as part of the buffer, usually used for debugging.
     *
     *  @{ */
    const std::string& name() const { return name_; }
    void name(const std::string &s) { name_ = s; }
    /** @} */

    /** Reads data from a buffer.
     *
     *  Reads up to @p n values from this buffer beginning at the specified address and copies them to the caller-supplied
     *  argument. Returns the number of values actually copied, which may be smaller than the number requested. The output
     *  buffer is not zero-padded for short reads.
     *
     *  As a special case, if @p buf is a null pointer, then no data is copied and the return value indicates the number of
     *  values that would have been copied had @p buf been non-null. */
    virtual Address read(Value *buf, Address address, Address n) const = 0;

    /** Writes data to a buffer.
     *
     *  Writes up to @p n values from @p buf into this buffer starting at the specified address.  Returns the number of values
     *  actually written, which might be smaller than @p n. The return value will be less than @p n if an error occurs. */
    virtual Address write(const Value *buf, Address address, Address n) = 0;
};

} // namespace
} // namespace

#endif

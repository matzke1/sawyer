#ifndef Sawyer_AddressMap_H
#define Sawyer_AddressMap_H

#include <sawyer/AddressSegment.h>
#include <sawyer/Assert.h>
#include <sawyer/Interval.h>
#include <sawyer/IntervalMap.h>
#include <boost/cstdint.hpp>

namespace Sawyer {
namespace Container {

template<class A, class T>
class SegmentMergePolicy {
public:
    typedef A Address;
    typedef T Value;
    typedef AddressSegment<A, T> Segment;

    bool merge(const Interval<Address> &leftInterval, Segment &leftSegment,
               const Interval<Address> &rightInterval, Segment &rightSegment) {
        ASSERT_forbid(leftInterval.isEmpty());
        ASSERT_forbid(rightInterval.isEmpty());
        ASSERT_require(leftInterval.upper() + 1 == rightInterval.lower());
        return (leftSegment.accessibility() == rightSegment.accessibility() &&
                leftSegment.buffer() == rightSegment.buffer() &&
                leftSegment.offset() + leftInterval.size() == rightSegment.offset());
    }

    Segment split(const Interval<Address> &interval, Segment &segment, Address splitPoint) {
        ASSERT_forbid(interval.isEmpty());
        ASSERT_require(interval.isContaining(splitPoint));
        Segment right = segment;
        right.offset(segment.offset() + splitPoint - interval.lower());
        return right;
    }

    void truncate(const Interval<Address> &interval, Segment &segment, Address splitPoint) {
        ASSERT_forbid(interval.isEmpty());
        ASSERT_require(interval.isContaining(splitPoint));
    }
};

 
/** A mapping from address space to values.
 *
 *  This object maps addresses (actually, intervals thereof) to values. Addresses must be an integral unsigned type but values
 *  may be any type as long as it is default constructible and copyable.  The address type is usually a type whose width is the
 *  log base 2 of the size of the address space; the value type is often unsigned 8-bit bytes.
 *
 *  An address map accomplishes the mapping by inheriting from an @ref IntervalMap, whose intervals are
 *  <code>Interval<A></code> and whose values are <code>AddressSegment<A,T></code>. The @ref AddressSegment objects
 *  point to reference-counted @ref Buffer objects that hold the values.  The same values can be mapped at different addresses
 *  by inserting segments at those addresses that point to a common buffer.
 *
 *  An address map implements read and write concepts for copying values between user-supplied buffers and the storage areas
 *  referenced by the map.  It also provides a notion of access permissions, which the read and write operations optionally
 *  check.  Reads and writes may be partial, copying fewer values than requested. */
template<class A, class T = boost::uint8_t>
class AddressMap: public IntervalMap<Interval<A>, AddressSegment<A, T>, SegmentMergePolicy<A, T> > {
    typedef              IntervalMap<Interval<A>, AddressSegment<A, T>, SegmentMergePolicy<A, T> > Super;
public:
    typedef A Address;                                  /**< Type for addresses. This should be an unsigned type. */
    typedef T Value;                                    /**< Type of data stored in the address space. */
    typedef AddressSegment<A, T> Segment;               /**< Type of segments stored by this map. */
    typedef typename Super::ValueIterator SegmentIterator; /**< Iterates over segments in the map. */
    typedef typename Super::ConstValueIterator ConstSegmentIterator; /**< Iterators over segments in the map. */
    typedef typename Super::ConstKeyIterator ConstIntervalIterator; /**< Iterates over address intervals in the map. */
    typedef typename Super::NodeIterator NodeIterator;  /**< Iterates over address interval, segment pairs in the map. */
    typedef typename Super::ConstNodeIterator ConstNodeIterator; /**< Iterates over address interval, segment pairs in the map. */

    AddressMap() {}
    AddressMap(const AddressMap &other): Super(other) {}

    /** Iterator range for address intervals.
     *
     *  This is just an alias for the @ref keys method defined in the super class. */
    boost::iterator_range<ConstIntervalIterator> intervals() const { return this->keys(); }

    /** Iterator range for segments.
     *
     *  This is just an alias for the @ref values method defined in the super class.
     *
     *  @{ */
    boost::iterator_range<SegmentIterator> segments() { return this->values(); }
    boost::iterator_range<ConstSegmentIterator> segments() const { return this->values(); }
    /** @} */

    /** Number of segments contained in the map. */
    Address nSegments() const { return this->nIntervals(); }

    /** Checks accessibility.
     *
     *  Determines which part of the address space beginning at @p start is mapped and accessible.  Returns an address
     *  interval, which will be empty if start itself is not mapped and accessible. */
    Interval<Address> available(Address start, unsigned requiredAccess=0, unsigned prohibitedAccess=0) const {
        Interval<Address> retval;
        ConstNodeIterator found = this->find(start);
        if (found != this->nodes().end()) {
            retval = Interval<Address>(start, found->key().upper());
            for (++found; found!=this->nodes().end(); ++found) {
                if (found->key().lower() != retval.upper() + 1)
                    break;                              // discontinuity
                if (!isAccessAllowed(found->value().accessibility(), requiredAccess, prohibitedAccess))
                    break;                              // disallowed
                retval = Interval<Address>(start, found->key().upper());
            }
        }
        return retval;
    }
    
    /** Reads data into the supplied buffer.
     *
     *  Fills the supplied buffer, @p buf, by copying values from the underlying buffers corresponding to the specified address
     *  range.  The read is terminated at the first address where any of the following conditions are satisfied:
     *
     *  @li The requsted number of values have been copied.
     *  @li The address is not a mapped address.
     *  @li The segment lacks the required accessibility.
     *  @li The read operation on the underlying buffer fails.
     *
     *  Returns the interval for the values that were copied into the supplied buffer. */
    Interval<Address> read(Value *buf, const Interval<Address> &where,
                           unsigned requiredAccess=0, unsigned prohibitedAccess=0) const {
        Interval<Address> retval;
        if (!where.isEmpty()) {
            for (ConstNodeIterator found = this->find(where.lower()); found!=this->nodes().end(); ++found) {
                if (!isAccessAllowed(found->value().accessibility(), requiredAccess, prohibitedAccess))
                    break;
                Interval<Address> part = where.intersection(found->key());
                if (part.isEmpty() || (!retval.isEmpty() && retval.upper()+1 != part.lower()))
                    break;
                Address bufferOffset = part.lower() - found->key().lower() + found->value().offset();
                Address nread = found->value().buffer()->read(buf, bufferOffset, part.size());
                if (nread!=part.size())                 // short read from buffer
                    return retval.hull(Interval<Address>::baseSize(part.lower(), nread));
                buf += nread;
                retval = retval.hull(part);
            }
        }
        return retval;
    }
    
    /** Writes data from the supplied buffer.
     *
     *  Copies data from buffer @p buf into the underlying buffers corresponding to the specified address range.  The write is
     *  terminated at the first address where any of the following conditions are satisfied:
     *
     *  @li The requested number of values have been written.
     *  @li The address is not a mapped address.
     *  @li The segment lacks the required accessibility.
     *  @li The write operation on the underlying buffer fails.
     *
     *  Returns the interval for the values that were copied into this map. */
    Interval<Address> write(const Value *buf, const Interval<Address> &where,
                            unsigned requiredAccess=0, unsigned prohibitedAccess=0) {
        Interval<Address> retval;
        if (!where.isEmpty()) {
            for (ConstNodeIterator found = this->find(where.lower()); found!=this->nodes().end(); ++found) {
                if (!isAccessAllowed(found->value().accessibility(), requiredAccess, prohibitedAccess))
                    break;
                Interval<Address> part = where.intersection(found->key());
                if (part.isEmpty() || (!retval.isEmpty() && retval.upper()+1 != part.lower()))
                    break;
                Address bufferOffset = part.lower() - found->key().lower() + found->value().offset();
                Address nwritten = found->value().buffer()->write(buf, bufferOffset, part.size());
                if (nwritten!=part.size())              // short write to buffer
                    return retval.hull(Interval<Address>::baseSize(part.lower(), nwritten));
                buf += nwritten;
                retval = retval.hull(part);
            }
        }
        return retval;
    }

private:
    static bool isAccessAllowed(unsigned has, unsigned required, unsigned prohibited) {
        return (has & required)==required && (~has & prohibited)==prohibited;
    }
};

} // namespace
} // namespace

#endif

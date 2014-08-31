#ifndef Sawyer_AddressMap_H
#define Sawyer_AddressMap_H

#include <sawyer/AddressSegment.h>
#include <sawyer/Assert.h>
#include <sawyer/Interval.h>
#include <sawyer/IntervalMap.h>
#include <sawyer/Sawyer.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/cstdint.hpp>
#include <boost/foreach.hpp>
#include <boost/integer_traits.hpp>

namespace Sawyer {
namespace Container {

// Used internally to split and merge segments
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
        ASSERT_require(leftInterval.greatest() + 1 == rightInterval.least());
        return (leftSegment.accessibility() == rightSegment.accessibility() &&
                leftSegment.buffer() == rightSegment.buffer() &&
                leftSegment.offset() + leftInterval.size() == rightSegment.offset());
    }

    Segment split(const Interval<Address> &interval, Segment &segment, Address splitPoint) {
        ASSERT_forbid(interval.isEmpty());
        ASSERT_require(interval.isContaining(splitPoint));
        Segment right = segment;
        right.offset(segment.offset() + splitPoint - interval.least());
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
 *  referenced by the map. Many of the address map methods operate over a region of the map described by a set of constraints
 *  that are matched within the map.  Constraints are indicated by listing them before the map I/O operation, but they do not
 *  modify the map in any way--they exist outside the map.  Constraints are combined by logical conjunction. For instance, the
 *  @ref AddressMap::next method returns the lowest address that satisfies the given constraints, or nothing.  If we wanted
 *  to search for the lowest address beteen 1000 and 1999 inclusive, that has read access but no execute access, we would
 *  do so like this:
 *
 * @code
 *  Optional<Address> x = map.within(1000,1999).require(READABLE).prohibit(WRITABLE).next();
 * @endcode
 *
 *  In fact, by making use of the @ref Sawyer::Optional API, we can write a loop that iterates over such addresses, although
 *  there may be more efficient ways to do this than one address at a time:
 *
 * @code
 *  for (Address x=1000; map.within(x,1999).require(READABLE).prohibit(WRITABLE).next().assignTo(x); ++x) ...
 * @endcode
 *
 *  The next example shows how to read a buffer's worth of values anchored at a particular starting value.  The @ref read
 *  method returns an address interval to indicate what addresses were accessed, but in this case we're only interested in the
 *  number of such addresses since we know the starting address.
 *
 * @code
 *  Value buf[10];
 *  size_t nAccessed = map.at(1000).limit(10).read(buf).size();
 * @endcode
 *
 *  An interval return value is more useful when we don't know where the operation occurs until after it occurs.  For instance,
 *  to read up to 10 values that are readable at or beyond some address:
 *
 * @code
 *  Value buf[10]
 *  Interval<Address> accessed = map.atOrAfter(1000).limit(10).require(READABLE).read(buf);
 * @endcode 
 *
 *  Here's an example that creates two buffers (they happen to point to arrays that the Buffer objects do not own), maps them
 *  at addresses in such a way that part of the smaller of the two buffers occludes the larger buffer, and then
 *  performs a write operation that touches parts of both buffers.  We then rewrite part of the mapping and do another write
 *  operation:
 *
 * @code
 *  using namespace Sawyer::Container;
 *
 *  typedef unsigned Address;
 *  typedef Interval<Address> Addresses;
 *  typedef Buffer<Address, char>::Ptr BufferPtr;
 *  typedef AddressSegment<Address, char> Segment;
 *  typedef AddressMap<Address, char> MemoryMap;
 *  
 *  // Create some buffer objects
 *  char data1[15];
 *  memcpy(data1, "---------------", 15);
 *  BufferPtr buf1 = Sawyer::Container::StaticBuffer<Address, char>::instance(data1, 15);
 *  char data2[5];
 *  memcpy(data2, "##########", 10);
 *  BufferPtr buf2 = Sawyer::Container::StaticBuffer<Address, char>::instance(data2, 5); // using only first 5 bytes
 *  
 *  // Map data2 into the middle of data1
 *  MemoryMap map;
 *  map.insert(Addresses::baseSize(1000, 15), Segment(buf1));
 *  map.insert(Addresses::baseSize(1005,  5), Segment(buf2)); 
 *  
 *  // Write across both buffers and check that data2 occluded data1
 *  Addresses accessed = map.at(1001).limit(13).write("bcdefghijklmn");
 *  ASSERT_always_require(accessed.size()==13);
 *  ASSERT_always_require(0==memcmp(data1, "-bcde-----klmn-", 15));
 *  ASSERT_always_require(0==memcmp(data2,      "fghij#####", 10));
 *  
 *  // Map the middle of data1 over the top of data2 again and check that the mapping has one element. I.e., the three
 *  // separate parts were recombined into a single entry since they are three consecutive areas of a single buffer.
 *  map.insert(Addresses::baseSize(1005, 5), Segment(buf1, 5));
 *  ASSERT_always_require(map.nSegments()==1);
 *  
 *  // Write some data again
 *  accessed = map.at(1001).limit(13).write("BCDEFGHIJKLMN");
 *  ASSERT_always_require(accessed.size()==13);
 *  ASSERT_always_require(0==memcmp(data1, "-BCDEFGHIJKLMN-", 15));
 *  ASSERT_always_require(0==memcmp(data2,      "fghij#####", 10));
 * @endcode
 *
 * @section errors Microsoft C++ compilers
 *
 * Microsoft Visual Studio 12 2013 (MSVC 18.0.30501.0) and possibly other versions look up non-dependent names in template base
 * classes in violation of C++ standards and apparently no switch to make their behavior compliant with the standard.  This
 * causes problems for AddressMap because unqualified references to <tt>%Interval</tt> should refer to the
 * Sawyer::Container::Interval class template, but instead they refer to the @ref IntervalMap::Interval "Interval" typedef in
 * the base class.  Our work around is to qualify all occurrences of <tt>%Interval</tt> where Microsoft compilers go wrong. */
template<class A, class T = boost::uint8_t>
class AddressMap: public IntervalMap<Interval<A>, AddressSegment<A, T>, SegmentMergePolicy<A, T> > {
    // "Interval" is qualified to work around bug in Microsoft compilers. See doxygen note above.
    typedef IntervalMap<Sawyer::Container::Interval<A>, AddressSegment<A, T>, SegmentMergePolicy<A, T> > Super;

public:
    typedef A Address;                                  /**< Type for addresses. This should be an unsigned type. */
    typedef T Value;                                    /**< Type of data stored in the address space. */
    typedef AddressSegment<A, T> Segment;               /**< Type of segments stored by this map. */
    typedef typename Super::Node Node;                  /**< Storage node containing interval/segment pair. */
    typedef typename Super::ValueIterator SegmentIterator; /**< Iterates over segments in the map. */
    typedef typename Super::ConstValueIterator ConstSegmentIterator; /**< Iterators over segments in the map. */
    typedef typename Super::ConstIntervalIterator ConstIntervalIterator; /**< Iterates over address intervals in the map. */
    typedef typename Super::NodeIterator NodeIterator;  /**< Iterates over address interval, segment pairs in the map. */
    typedef typename Super::ConstNodeIterator ConstNodeIterator; /**< Iterates over address interval/segment pairs in the map. */

    static const unsigned EXECUTABLE    = 0x00000001;   /**< Execute accessibility bit. */
    static const unsigned WRITABLE      = 0x00000002;   /**< Write accessibility bit. */
    static const unsigned READABLE      = 0x00000004;   /**< Read accessibility bit. */
    static const unsigned IMMUTABLE     = 0x00000008;   /**< Underlying buffer is immutable. E.g., mmap'd read-only. */
    static const unsigned RESERVED_MASK = 0x000000ff;   /**< Accessibility bits reserved for use by the library. */
    static const unsigned USERDEF_MASK  = 0xffffff00;   /**< Accessibility bits available to users. */

    class MatchedConstraints {
        friend class AddressMap;
        Interval<A> interval_;
        boost::iterator_range<ConstNodeIterator> nodes_;
    };

    /** Constraints are used to select addresses from a memory map.
     *
     *  Users don't normally see this class since it's almost always created as temporary. In fact, all of the public methods
     *  in this class are also present in the AddressMap class, and that's where they're documented.
     *
     *  The purpose of this class is to curry the arguments that would otherwise need to be passed to the various map I/O
     *  methods and which would significantly complicate the API since many of these arguments are optional. */
    class Constraints {
        friend class AddressMap;
        const AddressMap *map_;                         // map to which constraints apply
        unsigned requiredAccess_;                       // access bits that must be set in the segment
        unsigned prohibitedAccess_;                     // access bits that must be clear in the segment
        Optional<Address> least_;                       // least possible valid address
        Optional<Address> greatest_;                    // greatest possible valid address
        Optional<Address> anchored_;                    // anchored least or greatest address
        std::string nameSubstring_;                     // segment name must contain substring
        size_t maxSize_;                                // result size is limited
        bool singleSegment_;                            // do not cross a segment boundary
        bool never_;                                    // never match anything (e.g., when least_ > greatest_)
        explicit Constraints(const AddressMap *map)
            : map_(map), requiredAccess_(0), prohibitedAccess_(0), maxSize_(-1), singleSegment_(false), never_(false) {}
    public: // Modify constraints
        Constraints& require(unsigned bits) {
            requiredAccess_ |= bits;
            return *this;
        }
        Constraints& prohibit(unsigned bits) {
            prohibitedAccess_ |= bits;
            return *this;
        }
        Constraints& named(const std::string &s) {
            ASSERT_require(nameSubstring_.empty() || nameSubstring_==s);// substring conjunction not supported
            nameSubstring_ = s;
            return *this;
        }
        Constraints& any() {
            return *this;
        }
        Constraints& none() {
            never_=true;
            return *this;
        }
        Constraints& at(Address x) {
            if (anchored_ && *anchored_!=x) {
                none();
            } else {
                anchored_=x;
            }
            return *this;
        }
        Constraints& at(const Interval<Address> &x) {
            return x.isEmpty() ? none() : at(x.least()).atOrAfter(x.least()).atOrBefore(x.greatest());
        }
        Constraints& limit(size_t x) {
            maxSize_ = std::min(maxSize_, x);
            return *this;
        }
        Constraints& atOrAfter(Address least) {
            if (least_) {
                least_ = std::max(*least_, least);
            } else {
                least_ = least;
            }
            return *this;
        }
        Constraints& atOrBefore(Address greatest) {
            if (greatest_) {
                greatest_ = std::min(*greatest_, greatest);
            } else {
                greatest_ = greatest;
            }
            return *this;
        }
        Constraints& within(const Interval<Address> &x) {
            return x.isEmpty() ? none() : atOrAfter(x.least()).atOrBefore(x.greatest());
        }
        Constraints& within(Address lo, Address hi) {
            return lo<=hi ? within(Interval<Address>::hull(lo, hi)) : none();
        }
        Constraints& after(Address x) {
            return x==boost::integer_traits<Address>::const_max ? none() : atOrAfter(x+1);
        }
        Constraints& before(Address x) {
            return x==boost::integer_traits<Address>::const_min ? none() : atOrBefore(x-1);
        }
    public: // Methods that directly call the AddressMap
        Optional<Address> next() const { return map_->next(*this); }
        Sawyer::Container::Interval<Address> available() const { return map_->available(*this); }
        Sawyer::Container::Interval<Address> read(Value *buf /*out*/) const { return map_->read(buf, *this); }
        Sawyer::Container::Interval<Address> write(const Value *buf) const { return map_->write(buf, *this); }
    };

    /** Constructs an empty address map. */
    AddressMap() {}

    /** Copy constructor.
     *
     *  The new address map has the same addresses mapped to the same buffers as the @p other map.  The buffers themselves are
     *  not copied since they are reference counted. */
    AddressMap(const AddressMap &other): Super(other) {}

    /** Constraint: required access bits.
     *
     *  Constrains address to those that have all of the access bits that are set in @p x. */
    Constraints require(unsigned x) const { return Constraints(this).require(x); }

    /** Constraint: prohibited access bits.
     *
     *  Constrains addresses to those that have none of the access bits that are set in @p x. */
    Constraints prohibit(unsigned x) const { return Constraints(this).prohibit(x); }

    /** Constraint: segment name substring.
     *
     *  Constrains addresses to those that belong to a segment that contains string @p x as part of its name. */
    Constraints named(const std::string &x) const { return Constraints(this).named(x); }

    /** Constraint: anchor point.
     *
     *  Constrains addresses to a sequence that begins at @p x.  If address @p x is not part of the addresses matched by the
     *  other constraints, then no address matches. */
    Constraints at(Address x) const { return Constraints(this).at(x); }

    /** Constraint: anchored interval.
     *
     *  Constrains addresses to those that are within the specified interval, and requires that the least address of the
     *  interval satisfies all constraints.  If the least address does not satisfy the constraints then no address matches. */
    Constraints at(const Interval<Address> &x) const { return Constraints(this).at(x); }

    /** Constraint: limit matched size.
     *
     *  Constrains the matched addresses so that at most @p x addresses match.  Forward matching matches the first @p x
     *  addresses while backward matching matches the last @p x addresses. */
    Constraints limit(size_t x) const { return Constraints(this).limit(x); }

    /** Constraint: address lower bound.
     *
     *  Constrains matched addresses so that they are all greater than or equal to @p x. */
    Constraints atOrAfter(Address x) const { return Constraints(this).atOrAfter(x); }

    /** Constraint: address upper bound.
     *
     *  Constrains matched addresses so that they are all less than or equal to @p x. */
    Constraints atOrBefore(Address x) const { return Constraints(this).atOrBefore(x); }

    /** Constraint: address lower and upper bounds.
     *
     *  Constrains matched addresses so they are all within the specified interval. */
    Constraints within(const Interval<Address> &x) const { return Constraints(this).within(x); }

    /** Constraint: address lower and upper bounds.
     *
     *  Constrains matched addresses so they are all greater than or equal to @p x and less than or equal to @p y. */
    Constraints within(Address x, Address y) const { return Constraints(this).within(x, y); }

    /** Constraint: address lower bound.
     *
     *  Constrains matched addresses so that they are all greater than @p x. */
    Constraints after(Address x) const { return Constraints(this).after(x); }

    /** Constraint: address upper bound.
     *
     *  Constrains matched addresses so that they are all less than @p x. */
    Constraints before(Address x) const { return Constraints(this).before(x); }

    /** Constraint: matches anything.
     *
     *  The null constraint matches any mapped address. */
    Constraints any() const { return Constraints(this); }

    /** Constraint: matches nothing.
     *
     *  Constrains addresses so that none of them can match. */
    Constraints none() const { return Constraints(this).none(); }

    /** Iterator range for segments.
     *
     *  This is just an alias for the @ref values method defined in the super class.
     *
     *  @{ */
    boost::iterator_range<SegmentIterator> segments() { return this->values(); }
    boost::iterator_range<ConstSegmentIterator> segments() const { return this->values(); }
    /** @} */

    /** Segments that overlap with constraints.
     *
     *  Returns an iterator range for the segments that satisfy the specified constraints. This is most often used in loops to
     *  iterate over the segments that satisfy some condition.  Constraints always match contiguous addresses, and therefore
     *  match only contiguous segments also.
     *
     *  The following example iterates over the contiguous segments starting with the segment that contains address 1000.
     *
     * @code
     *  BOOST_FOREACH (const Segment &segment, map.at(1000).segments())
     *      std::cout <<"segment " <<segment.name() <<"\n";
     * @endcode */
    boost::iterator_range<ConstSegmentIterator> segments(const Constraints &c) const {
        MatchedConstraints m = matchForward(c);
        return boost::iterator_range<ConstSegmentIterator>(m.nodes_.begin(), m.nodes_.end());
    }

    /** Iterator range for nodes.
     *
     *  This is just an alias for the @ref nodes method defined in the super class.  See also the overloaded method of the same
     *  name that takes a constraint and thus returns only some nodes.
     *
     * @{ */
    boost::iterator_range<NodeIterator> nodes() { return Super::nodes(); }
    boost::iterator_range<ConstNodeIterator> nodes() const { return Super::nodes(); }
    /** @} */

    /** Nodes that overlap with constraints.
     *
     *  Returns a iterator range for the nodes that satisfy the specified constraints.  This is most often used in loops to
     *  iterate over the segments that satisfy some condition.  Constraints always match contiguous addresses, and therefore
     *  match only contiguous segments also.
     *
     *  The following example iterates over the contiguous segments starting with the segment that contains  address 1000.
     *
     * @code
     *  BOOST_FOREACH (const Node &node, map.at(1000).nodes())
     *      std::cout <<"segment at " <<node.key() <<" named " <<node.value().name() <<"\n";
     * @endcode */
    boost::iterator_range<ConstNodeIterator> nodes(const Constraints &c) const {
        return matchForward(c).nodes_;
    }

    /** Number of segments contained in the map.
     *
     *  Multiple segments may be pointing to the same underlying buffer, and the number of segments is not necessarily the same
     *  as the net number of segments inserted and erased.  For instance, if a segment is inserted for addresses [0,99] and
     *  then a different segment is inserted at [50,59], the map will contain three segments at addresses [0,49], [50,59], and
     *  [60,99], although the first and third segment point into different parts of the same buffer. */
    Address nSegments() const { return this->nIntervals(); }

    /** Next higher address that satisfies constraints. */
    Optional<Address> next(Constraints c) const {
        c.limit(1);                                     // no need to test segments beyond the first match
        MatchedConstraints m = matchForward(c);
        return m.interval_.isEmpty() ? Sawyer::Nothing() : m.interval_.least();
    }

    /** Adress interval that satisfies constraints. */
    Sawyer::Container::Interval<Address> available(const Constraints &c) const {
        return matchForward(c).interval_;
    }

    /** Reads data into the supplied buffer.
     *
     *  Reads data into the buffer according to the specified constraints.  The buffer can be an array or STL vector. The
     *  constraints are augmented by also requiring that the addresses be contiguous, and in the case of STL vectors that not
     *  more data is read into the vector than the size of the vector.  The return value is the interval of addresses that were
     *  read.
     *
     *  The constraints are usually curried before the actual read call, as in this example that reads up to 10 values starting
     *  at some address and returns the number of values read:
     *
     * @code
     *  Value buf[10];
     *  size_t nRead = map.at(start).limit(10).read(buf).size();
     * @endcode
     *
     * @{ */
    Sawyer::Container::Interval<Address> read(Value *buf /*out*/, const Constraints &c) const {
        MatchedConstraints m = matchForward(c);
        BOOST_FOREACH (const Node &node, m.nodes_) {
            Sawyer::Container::Interval<Address> part = m.interval_.intersection(node.key());// part of segment to read
            ASSERT_forbid(part.isEmpty());
            Address bufferOffset = part.least() - node.key().least() + node.value().offset();
            Address nBytes = node.value().buffer()->read(buf, bufferOffset, part.size());
            ASSERT_require(nBytes==part.size());
            buf += nBytes;
        }
        return m.interval_;
    }
    Sawyer::Container::Interval<Address> read(std::vector<Value> &buf /*out*/, Constraints c) const {
        c.limit(buf.size());
        return buf.empty() ? Sawyer::Container::Interval<Address>() : read(&buf[0], c);
    }
    /** @} */

    /** Writes data from the supplied buffer.
     *
     *  Copies data from buffer @p buf into the underlying address map buffers corresponding to the specified constraints.  The
     *  buffer can be an array or STL vector.  The constraints are agumented by also requiring that the addresses be contiguous
     *  and lack the IMMUTABLE bit, and in the case of STL vectors that not more data is written thn what is in the vector.
     *  The return value is the interval of addresses that were written.
     *
     *  Note that the memory map object (@c this) is constant because a write operation doesn't change the mapping, it only
     *  copies data into the existing mapped buffers.  The @ref IMMUTABLE bit is usually used to indicate that a buffer cannot
     *  be modified (for instance, the buffer is memory allocated with read-only access by POSIX @c mmap).
     *
     *  The constraints are usually curried before the actual read call, as in this example that writes the vector's values
     *  into the map at the first writable address greater than or equal to 1000.
     *
     * @code
     *  std::vector<Value> buffer = {...};
     *  Interval<Address> written = map.atOrAfter(1000).require(WRITABLE).write(buffer);
     * @endcode
     *
     * @{ */
    Sawyer::Container::Interval<Address> write(const Value *buf, Constraints c) const {
        c.prohibit(IMMUTABLE);                          // don't ever write to buffers that can't be modified
        MatchedConstraints m = matchForward(c);
        BOOST_FOREACH (const Node &node, m.nodes_) {
            Sawyer::Container::Interval<Address> part = m.interval_.intersection(node.key());// part of segment to write
            ASSERT_forbid(part.isEmpty());
            Address bufferOffset = part.least() - node.key().least() + node.value().offset();
            Address nBytes = node.value().buffer()->write(buf, bufferOffset, part.size());
            ASSERT_require(nBytes==part.size());
            buf += nBytes;
        }
        return m.interval_;
    }
    Sawyer::Container::Interval<Address> write(const std::vector<Value> &buf, Constraints c) const {
        c.limit(buf.size());
        return buf.empty() ? Sawyer::Container::Interval<Address>() : write(&buf[0], c);
    }
    /** @} */

private:
    // Finds the minimum possible address and node iterator for the specified constraints in this map and returns that
    // iterator.  Returns the end iterator if the constraints match no address.  If a non-end iterator is returned then minAddr
    // is adjusted to be the minimum address that satisfies the constraint (it will be an address within the returned node, but
    // not necessarily the least address for the node).  If useAnchor is set and the constraints specify an anchor, then the
    // anchor address must be present in the map and satisfy any address constraints that might also be present.
    ConstNodeIterator constraintLowerBound(const Constraints &c, bool useAnchor, Address &minAddr) const {
        if (this->isEmpty() || c.never_)
            return this->nodes().end();

        if (useAnchor && c.anchored_) {                 // forward matching if useAnchor is set
            if ((c.least_ && *c.least_ > *c.anchored_) || (c.greatest_ && *c.greatest_ < *c.anchored_))
                return this->nodes().end();             // anchor is outside of allowed interval
            ConstNodeIterator lb = this->lowerBound(*c.anchored_);
            if (lb==this->nodes().end() || *c.anchored_ < lb->key().least())
                return this->nodes().end();             // anchor is not present in this map
            minAddr = *c.anchored_;
            return lb;
        }

        if (c.least_) {
            ConstNodeIterator lb = this->lowerBound(*c.least_);
            if (lb==this->nodes().end())
                return lb;                              // least is above all segments
            minAddr = std::max(*c.least_, lb->key().least());
            return lb;
        }

        ConstNodeIterator lb = this->nodes().begin();
        if (lb!=this->nodes().end())
            minAddr = lb->key().least();
        return lb;
    }

    // Finds the maximum possible address and node for the specified constraints in this map, and returns an iterator to the
    // following node.  Returns the begin iterator if the constraints match no address.  If a non-begin iterator is returned
    // then maxAddr is adjusted to be the maximum address that satisfies the constraint (it will be an address that belongs to
    // the node immediately prior to the one pointed to by the returned iterator, but not necessarily the greatest address for
    // that node).  If useAnchor is set and the constraints specify an anchor, then the anchor address must be present in the
    // map and satisfy any address constraints that might also be present.
    ConstNodeIterator constraintUpperBound(const Constraints &c, bool useAnchor, Address &maxAddr) const {
        if (this->isEmpty() || c.never_)
            return this->nodes().begin();

        if (useAnchor && c.anchored_) {                 // backward matching if useAnchor is set
            if ((c.least_ && *c.least_ > *c.anchored_) || (c.greatest_ && *c.greatest_ < *c.anchored_))
                return this->nodes().begin();           // anchor is outside allowed interval
            ConstNodeIterator ub = this->findPrior(*c.anchored_);
            if (ub==this->nodes().end() || *c.anchored_ > ub->key().greatest())
                return this->nodes().begin();           // anchor is not present in this map
            maxAddr = *c.anchored_;
            return ub;
        }

        if (c.greatest_) {
            ConstNodeIterator ub = this->findPrior(*c.greatest_);
            if (ub==this->nodes().end())
                return this->nodes().begin();           // greatest is below all segments
            maxAddr = std::min(ub->key().greatest(), *c.greatest_);
            return ++ub;                                // return node after the one containing the maximum
        }
        
        maxAddr = this->hull().greatest();
        return this->nodes().end();
    }

    // Matches constraints in a forward direction.
    MatchedConstraints matchForward(const Constraints &c) const {
        MatchedConstraints retval;
        if (c.never_ || this->isEmpty())
            return retval;
        if (c.map_ && c.map_ != this)
            throw std::runtime_error("AddressMap constraints are not transferable across maps");

        // Find a lower bound for the minimum address
        Address minAddr = 0;
        ConstNodeIterator begin = constraintLowerBound(c, true, minAddr /*out*/);
        if (begin == this->nodes().end())
            return retval;

        // Find an upper bound for the maximum address.
        Address maxAddr = 0;
        ConstNodeIterator end = constraintUpperBound(c, false, maxAddr /*out*/);
        if (end==this->nodes().begin())
            return retval;

        // Advance the lower-bound until it satisfies the other (non-address) constraints
        for (/*void*/; begin != end; ++begin) {
            const Segment &segment = begin->value();
            if (!segment.isAccessible(c.requiredAccess_, c.prohibitedAccess_))
                continue;                               // wrong segment permissions
            if (!boost::contains(segment.name(), c.nameSubstring_))
                continue;                               // wrong segment name
            break;
        }
        if (begin==end)
            return retval;
        minAddr = std::max(minAddr, begin->key().least());

        // Iterate forward until the constraints are no longer satisfied
        Address addr = minAddr;
        ConstNodeIterator iter = begin;
        for (/*void*/; iter!=end; ++iter) {
            if (iter!=begin) {                          // already tested the first segment above
                if (c.singleSegment_)
                    break;                              // we crossed a segment boundary
                if (addr+1 != iter->key().least())
                    break;                              // gap between segments
                const Segment &segment = begin->value();
                if (!segment.isAccessible(c.requiredAccess_, c.prohibitedAccess_))
                    break;                              // wrong segment permissions
                if (!boost::contains(segment.name(), c.nameSubstring_))
                    break;                              // wrong segment name
            }
            if (iter->key().greatest() - minAddr >= c.maxSize_) {
                addr = minAddr + c.maxSize_ - 1;
                ++iter;
                break;
            }
            addr = iter->key().greatest();
        }

        // Build the result
        retval.interval_ = Interval<Address>::hull(minAddr, addr);
        retval.nodes_ = boost::iterator_range<ConstNodeIterator>(begin, iter);
        return retval;
    }

    /** Match constraints in a backward direction. */
    MatchedConstraints matchBackward(const Constraints &c) const {
        abort();                                        // FIXME[Robb Matzke 2014-08-30]: not implemented yet
    }

};

} // namespace
} // namespace

#endif

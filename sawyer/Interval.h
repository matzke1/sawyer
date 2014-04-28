#ifndef Sawyer_Interval_H
#define Sawyer_Interval_H

#include <sawyer/Assert.h>

namespace Sawyer {
namespace Container {

template<class T>
class Interval {
public:
    typedef T Value;
private:
    T lo_, hi_;
public:
    /** Constructs an empty interval. */
    Interval(): lo_(1), hi_(0) {}

    /** Copy-constructs an interval. */
    Interval(const Interval &other): lo_(other.lo_), hi_(other.hi_) {}

    /** Constructs a singleton interval. */
    Interval(T value): lo_(value), hi_(value) {}

    /** Constructs an interval from endpoints.
     *
     *  The first end point must be less than or equal to the second end point.  If both endpoints are equal then a singleton
     *  interval is constructed. */
    Interval(T lo, T hi): lo_(lo), hi_(hi) {
        ASSERT_require(lo <= hi);
    }

    /** Construct an interval from two endpoints.
     *
     *  Returns the smallest interal that contains both points. */
    static Interval hull(T v1, T v2) {
        return Interval(std::min(v1, v2), std::max(v1, v2));
    }

    /** Construct an interval from one endpoint and a size.
     *
     *  Returns the smallest interval that contains @p lo (inclusive) through @p lo + @p size (exclusive).  If @p size is zero
     *  then an empty interval is created, in which case @p lo is irrelevant. */
    static Interval baseSize(T lo, T size) {
        ASSERT_require2(lo + size >= lo, "overflow");
        return 0==size ? Interval() : Interval(lo, lo+size-1);
    }

    /** Assignment from an interval. */
    Interval& operator=(const Interval &other) {
        lo_ = other.lo_;
        hi_ = other.hi_;
        return *this;
    }

    /** Assignment from a scalar. */
    Interval& operator=(T value) {
        lo_ = hi_ = value;
        return *this;
    }

    /** Returns lower limit. */
    T lower() const {
        ASSERT_forbid(isEmpty());
        return lo_;
    }

    /** Returns upper limit. */
    T upper() const {
        ASSERT_forbid(isEmpty());
        return hi_;
    }

    /** True if interval is empty. */
    bool isEmpty() const { return 1==lo_ && 0==hi_; }

    /** True if interval is a singleton. */
    bool isSingleton() const { return lo_ == hi_; }

    /** True if interval covers entire space. */
    bool isWhole() const { return !isEmpty && hi_ + 1 == lo_; }

    /** True if two intervals overlap.
     *
     *  An empty interval never overlaps with any other interval, empty or not. */
    bool isOverlapping(const Interval &other) const {
        return !intersection(other).isEmpty();
    }

    /** Containment predicate.
     *
     *  Returns true if this interval contains all of the @p other interval.  An empty interval is always contained in any
     *  other interval, even another empty interval. */
    bool isContaining(const Interval &other) const {
        return (other.isEmpty() ||
                (!isEmpty() && lower()<=other.lower() && upper()>=other.upper()));
    }

    /** Adjacency predicate.
     *
     *  Returns true if the two intervals are adjacent.  An empty interval is adjacent to all other intervals, including
     *  another empty interval.
     *
     *  @{ */
    bool isLeftAdjacent(const Interval &right) const {
        return isEmpty() || right.isEmpty() || (!isWhole() && upper()+1 == right.lower());
    }
    bool isRightAdjacent(const Interval &left) const {
        return isEmpty() || left.isEmpty() || (!left.isWhole() && left.upper()+1 == lower());
    }
    bool isAdjacent(const Interval &other) const {
        return (isEmpty() || other.isEmpty() ||
                (!isWhole() && upper()+1 == other.lower()) ||
                (!other.isWhole() && other.upper()+1 == lower()));
    }
    /** @} */

    /** Relative position predicate.
     *
     *  Returns true if the intervals do not overlap and one is positioned left or right of the other.  Empty intervals are
     *  considered to be both left and right of the other.
     *
     *  @{ */
    bool isLeftOf(const Interval &right) const {
        return isEmpty() || right.isEmpty() || upper() < right.lower();
    }
    bool isRightOf(const Interval &left) const {
        return isEmpty() || left.isEmpty() || left.upper() < lower();
    }
    /** @} */

    /** Size of interval.
     *
     *  If the interval is the whole space then the return value is zero due to overflow. */
    Value size() const { return isEmpty() ? 0 : hi_ - lo_ + 1; }

    /** Equality test.
     *
     *  Two intervals are equal if they have the same lower and upper bound, and unequal if either bound differs.
     *
     *  @{ */
    bool operator==(const Interval &other) const {
        return lo_==other.lo_ && hi_==other.hi_;
    }
    bool operator!=(const Interval &other) const {
        return lo_!=other.lo_ || hi_!=other.hi_;
    }
    /** @} */

    /** Intersection.
     *
     *  Returns an interval which is the intersection of this interval with another. */
    Interval intersection(const Interval &other) const {
        if (isEmpty() || other.isEmpty() || upper()<other.lower() || lower()>other.upper())
            return Interval();
        return Interval(std::max(lower(), other.lower()), std::min(upper(), other.upper()));
    }

    /** Hull.
     *
     *  Returns the smallest interval that contains both this interval and the @p other interval. */
    Interval hull(const Interval &other) const {
        if (isEmpty()) {
            return other;
        } else if (other.isEmpty()) {
            return *this;
        } else {
            return Interval(std::min(lower(), other.lower()), std::max(upper(), other.upper()));
        }
    }

    /** Hull.
     *
     *  Returns the smallest interval that contains both this interval and another value. */
    Interval hull(T value) const {
        if (isEmpty()) {
            return Interval(value);
        } else {
            return Interval(std::min(lower(), value), std::max(upper(), value));
        }
    }
};

} // namespace
} // namespace

#endif

#ifndef Sawyer_SlidingWindow_H
#define Sawyer_SlidingWindow_H

#include <sawyer/Assert.h>
#include <sawyer/Sawyer.h>
#include <vector>

namespace Sawyer {
namespace Container {

/** Holds a sliding window of data.
 *
 *  Holds a sliding window (first in, first out queue) of up to @p N items of type @p T. When an item is inserted into the
 *  window the @c insert method of the @p Statistics object (which is provided during construction) is called, and when a item
 *  is removed from the window (because the window is full), the @p Statistics @c erase method is called.  Both methods take a
 *  copy of the item being inserted or erased. */
template<typename T, class Statistics>
class SlidingWindow {
public:
    typedef T Value;
private:
    std::vector<Value> buffer_;
    size_t start_, size_;
    const size_t capacity_;
    Statistics stats_;
public:
    /** Default constructor.
     *
     *  Creates an empty sliding window with a default constructed statistics object. The window's buffer is filled with
     *  default-constructed values, but these values do not contribute to the size reported by the @ref size method.
     *
     *  This constructor operates in linear time proportional to the @p capacity. */
    explicit SlidingWindow(size_t capacity)
        : buffer_(capacity), start_(0), size_(0), capacity_(capacity) {
        ASSERT_require(capacity>0);
    }

    /** Construct with initial statistics.
     *
     *  Creates an empty sliding window with the specified statistics.  The statistics are copied into the window.  The
     *  window's buffer is filled with default-constructed values, but these values do not contribute to the size reported by
     *  the @ref size method.
     *
     *  This constructor operates in linear time proportional to the @p capacity. */
    SlidingWindow(size_t capacity, const Statistics &stats)
        : buffer_(capacity, 0), start_(0), size_(0), capacity_(capacity), stats_(stats) {
        ASSERT_require(capacity>0);
    }

    /** Insert one item.
     *
     *  Inserts one item into the window and calls the statistics @p insert method with a copy of that item.  If the window is
     *  full, then the oldest item is removed first and the statistics @p erase method is called with a copy of the item being
     *  erased.  The new item is assigned to a slot in the window's buffer, replacing either a previously inserted value or a
     *  default-constructed value.
     *
     *  This is a constant-time operation if the statistics @c insert and @c erase calls are also constant time. */
    void insert(Value x) {
        if (size_==capacity_) {
            stats_.erase(buffer_[start_]);
            buffer_[start_] = x;
            start_ = (start_ + 1) % capacity_;
        } else {
            size_t idx = (start_ + size_) % capacity_;
            buffer_[idx] = x;
            ++size_;
        }
        stats_.insert(x);
    }

    /** Returns a reference to the statistics.
     *  @{ */
    const Statistics& stats() const {
        return stats_;
    }
    Statistics& stats() {
        return stats_;
    }
    /** @} */

    /** Number of items in the window.
     *
     *  Returns the number of items currently in the window.  Windows start off empty, increase in size until the declared size
     *  is reached, and then no longer grow.
     *
     *  This is a constant-time operation. */
    size_t size() const {
        return size_;
    }

    /** Access individual item.
     *
     *  Returns a reference to the indicated item.  The oldest item in the window is number zero, the next oldest is number
     *  one, etc., up to <code>size()-1</code>.
     *
     *  This is a constant-time operation.
     *
     *  @{ */
    const T& operator[](size_t idx) const {
        ASSERT_require2(idx < size(), "index " + boost::lexical_cast<std::string>(idx) + " is out of range");
        return buffer_[(start_ + idx) % capacity_];
    }
    T& operator[](size_t idx) {
        ASSERT_require2(idx < size(), "index " + boost::lexical_cast<std::string>(idx) + " is out of range");
        return buffer_[(start_ + idx) % capacity_];
    }
    /** @} */

    /** Removes all items from the window.
     *
     *  Removes items from the window by invoking the statistics @c erase method in the order in the same order that items were
     *  added until the window is empty.  The existing objects are removed from the window's buffer by assigning a default
     *  constructed value to the buffer.
     *
     *  Clearing the list operates in linear time proportional to the number of items currently in the list if the statistics
     *  @c erase call is constant time. */
    void clear() {
        while (size_ > 0) {
            stats.erase(buffer_[start_]);
            buffer_[start_] = Value();
            start_ = (start_+1) % capacity_;
            --size_;
        }
        start_ = 0;
    }
};

} // namespace
} // namespace
#endif

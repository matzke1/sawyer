#ifndef Sawyer_Histogram_H
#define Sawyer_Histogram_H

#include <map>
#include <sawyer/Assert.h>

namespace Sawyer {

/** %Histogram of symbols.
 *
 *  This class is a histogram of symbols of type @p T.  Symbols are inserted with the @ref insert method and erased with the @p
 *  erase method.  At any point, various statistics can be queried. */
template<typename T, class Cmp = std::less<T> >
class Histogram {
public:
    typedef T Value;
protected:
    typedef std::map<Value, size_t, Cmp> Map;
    Map map_;
public:

    /** Insert a symbol.
     *
     *  The symbol @p x is inserted into this histogram and statistics are updated. */
    void insert(T x) {
        std::pair<typename Map::iterator, bool> inserted = map_.insert(std::make_pair(x, 1));
        if (!inserted.second)
            ++inserted.first->second;
    }

    /** Erase a symbol.
     *
     *  The symbol @p x, which must exist in the histogram, is removed from the histogram and statistics are updated. */
    void erase(T x) {
        typename Map::iterator found = map_.find(x);
        ASSERT_require(found!=map_.end());
        ASSERT_require(found->second > 0);
        if (1==found->second) {
            map_.erase(found);
        } else {
            --found->second;
        }
    }

    /** Number of unique symbols.
     *
     *  Returns the number of distinct symbols currently represented by this histogram. */
    size_t nUnique() const {
        return map_.size();
    }

    /** Number of occurrences of a symbol.
     *
     *  Returns the number of times symbol @p x appears in this histogram. */
    size_t count(T x) const {
        typename Map::const_iterator found = map_.find(x);
        return found==map_.end() ? 0 : found->second;
    }
};

} // namespace
#endif

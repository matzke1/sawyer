#ifndef Sawyer_Histogram_H
#define Sawyer_Histogram_H

#include <Sawyer/Assert.h>
#include <Sawyer/Map.h>
#include <boost/foreach.hpp>

namespace Sawyer {

/** %Histogram of symbols.
 *
 *  This class is a histogram of symbols of type @p T.  Symbols are inserted with the @ref insert method and erased with the @p
 *  erase method.  At any point, various statistics can be queried. */
template<typename T, class Cmp = std::less<T> >
class Histogram {
public:
    typedef T Value;
    typedef Sawyer::Container::Map<Value, size_t, Cmp> ForwardMap;
    typedef Sawyer::Container::Map<size_t, std::vector<Value> > ReverseMap;
protected:
    ForwardMap map_;
public:

    /** Insert a symbol.
     *
     *  The symbol is inserted into this histogram and statistics are updated. */
    void insert(T symbol) {
        size_t &count = map_.insertMaybe(symbol, 0);
        ++count;
    }

    /** Erase a symbol.
     *
     *  The symbol, which must exist in the histogram, is removed from the histogram and statistics are updated. */
    void erase(T symbol) {
        typename ForwardMap::NodeIterator found = map_.find(symbol);
        ASSERT_require(found!=map_.nodes().end());
        ASSERT_require(found->value() > 0);
        if (1==found->value()) {
            map_.eraseAt(found);
        } else {
            --found->value();
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
     *  Returns the number of times the symbol appears in this histogram. */
    size_t count(T symbol) const {
        return map_.getOptional(symbol).orElse(0);
    }

    /** Histogram map.
     *
     *  Returns the histogram map whose keys are the symbols and whose values are the counts. */
    const ForwardMap& symbols() const {
        return map_;
    }

    /** Returns the inverted map.
     *
     *  Returns a map whose keys are the counts and whose values are lists of symbols having those counts. */
    ReverseMap counts() const {
        ReverseMap cmap;
        BOOST_FOREACH (const typename ForwardMap::Node &node, map_.nodes())
            cmap.insertMaybeDefault(node.value()).push_back(node.key());
        return cmap;
    }

    /** Returns the symbols with the highest frequency. */
    std::vector<Value> mostFrequentSymbols() const {
        std::vector<Value> bestSymbols;
        size_t bestFrequency;
        BOOST_FOREACH (const typename ForwardMap::Node &node, map_.nodes()) {
            if (bestSymbols.empty() || node.value()>bestFrequency) {
                bestSymbols.clear();
                bestSymbols.push_back(node.key());
                bestFrequency = node.value();
            } else if (node.value() == bestFrequency) {
                bestSymbols.push_back(node.key());
            }
        }
        return bestSymbols;
    }
};

} // namespace
#endif

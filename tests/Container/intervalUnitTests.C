#include <sawyer/IntervalSet.h>
#include <boost/foreach.hpp>
#include <boost/numeric/interval.hpp>
#include <boost/integer_traits.hpp>

// Allow empty intervals without throwing an exception.
template<typename T>
struct AllowEmptyIntervals: boost::numeric::interval_lib::policies<
    boost::numeric::interval_lib::rounded_math<T>,
    boost::numeric::interval_lib::checking_no_nan<T> > {};

// Intervals using any unsigned type and allowing them to be empty
template<typename T>
class UnsignedExtent: public boost::numeric::interval<T, AllowEmptyIntervals<T> > {
    typedef                  boost::numeric::interval<T, AllowEmptyIntervals<T> > Super;
public:
    UnsignedExtent(): Super(Super::empty()) {}
    UnsignedExtent(T value): Super(value) {}
    UnsignedExtent(T lo, T hi): Super(lo, hi) {}
    UnsignedExtent(const boost::numeric::interval<T, AllowEmptyIntervals<T> > &other): Super(other) {}

    UnsignedExtent& operator=(T val) {
        Super::operator=(val);
        return *this;
    }

    UnsignedExtent& operator=(const boost::numeric::interval<T, AllowEmptyIntervals<T> > &other) {
        Super::operator=(other);
        return *this;
    }

    bool isEmpty() const {
        return Super::traits_type::checking::is_empty(this->lower(), this->upper());
    }

    T size() const {
        return isEmpty() ? 0 : width(*this) + 1;
    }
};

template<typename T, class Policy>
std::ostream& operator<<(std::ostream &o, const boost::numeric::interval<T, Policy> &interval) {
    o <<"[" <<interval.lower() <<"," <<interval.upper() <<"]";
    return o;
}

template<class Interval, class T, class Policy>
static void show(const Sawyer::Container::IntervalMap<Interval, T, Policy> &imap) {
    typedef typename Sawyer::Container::IntervalMap<Interval, T, Policy> Map;
    std::cerr <<"  size = " <<(boost::uint64_t)imap.size() <<" in " <<imap.nIntervals() <<"\n";
    std::cerr <<"  nodes = {";
    for (typename Map::ConstNodeIterator iter=imap.nodes().begin(); iter!=imap.nodes().end(); ++iter) {
        const Interval &interval = iter->key();
        const T &value = iter->value();
        std::cerr <<" [" <<interval.lower() <<"," <<interval.upper() <<"]=" <<value;
    }
    std::cerr <<" }\n";
}

template<class Interval>
static void show(const Sawyer::Container::IntervalSet<Interval> &iset) {
    std::cerr <<"  size = " <<(boost::uint64_t)iset.size() <<" in " <<iset.nIntervals() <<"\n";
    std::cerr <<"  nodes = {";
    typedef typename Sawyer::Container::IntervalSet<Interval>::ConstNodeIterator Iter;
    for (Iter iter=iset.nodes().begin(); iter!=iset.nodes().end(); ++iter) {
        const Interval &interval = *iter;
        std::cerr <<" [" <<interval.lower() <<"," <<interval.upper() <<"]";
    }
    std::cerr <<" }\n";
}

// Some basic interval tests
template<class Interval>
static void interval_tests() {

    // Unlike boost::numeric::interval, our default constructor creates an empty interval
    Interval e1;
    ASSERT_require(empty(e1));
    ASSERT_require(width(e1)==0);
    ASSERT_require(e1.size()==0);

    // boost::numeric::interval_lib::width() is weird: it is as if upper() is not included in the interval.
    // Our version of size() is better, but it can overflow to zero.
    Interval e2(1);
    ASSERT_forbid(empty(e2));
    ASSERT_require(width(e2)==0);
    ASSERT_require(e2.size()==1);

    Interval e3(0, boost::integer_traits<typename Interval::base_type>::const_max);
    ASSERT_forbid(empty(e3));
    ASSERT_require(width(e3)==boost::integer_traits<typename Interval::base_type>::const_max);
    ASSERT_require(e1.size()==0);                       // because of overflow
}

template<class Interval, class Value>
static void imap_tests(const Value &v1, const Value &v2) {
    typedef Sawyer::Container::IntervalMap<Interval, Value> Map;
    Map imap;

    std::cerr <<"insert([100,119], v1)\n";
    imap.insert(Interval(100, 119), v1);
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==1);

    // Test lowerBound against one node
    ASSERT_require(imap.lowerBound(99)==imap.nodes().begin());
    ASSERT_require(imap.lowerBound(100)==imap.nodes().begin());
    ASSERT_require(imap.lowerBound(118)==imap.nodes().begin());
    ASSERT_require(imap.lowerBound(119)==imap.nodes().begin());
    ASSERT_require(imap.lowerBound(120)==imap.nodes().end());

    // Test findPrior against one node
    ASSERT_require(imap.findPrior(99)==imap.nodes().end());
    ASSERT_require(imap.findPrior(100)==imap.nodes().begin());
    ASSERT_require(imap.findPrior(119)==imap.nodes().begin());
    ASSERT_require(imap.findPrior(120)==imap.nodes().begin());

    // Erase the right part
    std::cerr <<"erase([110,119])\n";
    imap.erase(Interval(110, 119));
    show(imap);
    ASSERT_require(imap.size()==10);
    ASSERT_require(imap.nIntervals()==1);

    // Re-insert the right part
    std::cerr <<"insert([110,119], v1)\n";
    imap.insert(Interval(110, 119), v1);
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==1);

    // Erase the left part
    std::cerr <<"erase([100,109])\n";
    imap.erase(Interval(100, 109));
    show(imap);
    ASSERT_require(imap.size()==10);
    ASSERT_require(imap.nIntervals()==1);

    // Re-insert the left part
    std::cerr <<"insert([100,109], v1)\n";
    imap.insert(Interval(100, 109), v1);
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==1);

    // Erase the middle part
    std::cerr <<"erase([105,114])\n";
    imap.erase(Interval(105, 114));
    show(imap);
    ASSERT_require(imap.size()==10);
    ASSERT_require(imap.nIntervals()==2);

    // Test lowerBound against multiple nodes
    typename Map::NodeIterator secondIter = imap.nodes().begin(); ++secondIter;
    ASSERT_require(imap.lowerBound(99)==imap.nodes().begin());
    ASSERT_require(imap.lowerBound(100)==imap.nodes().begin());
    ASSERT_require(imap.lowerBound(104)==imap.nodes().begin());
    ASSERT_require(imap.lowerBound(105)==secondIter);
    ASSERT_require(imap.lowerBound(114)==secondIter);
    ASSERT_require(imap.lowerBound(115)==secondIter);
    ASSERT_require(imap.lowerBound(119)==secondIter);
    ASSERT_require(imap.lowerBound(120)==imap.nodes().end());

    // Test findPrior against multiple nodes
    ASSERT_require(imap.findPrior(99)==imap.nodes().end());
    ASSERT_require(imap.findPrior(100)==imap.nodes().begin());
    ASSERT_require(imap.findPrior(104)==imap.nodes().begin());
    ASSERT_require(imap.findPrior(105)==imap.nodes().begin());
    ASSERT_require(imap.findPrior(114)==imap.nodes().begin());
    ASSERT_require(imap.findPrior(115)==secondIter);
    ASSERT_require(imap.findPrior(119)==secondIter);
    ASSERT_require(imap.findPrior(120)==secondIter);

    // Test find against multiple nodes
    ASSERT_require(imap.find(99)==imap.nodes().end());
    ASSERT_require(imap.find(100)==imap.nodes().begin());
    ASSERT_require(imap.find(104)==imap.nodes().begin());
    ASSERT_require(imap.find(105)==imap.nodes().end());
    ASSERT_require(imap.find(114)==imap.nodes().end());
    ASSERT_require(imap.find(115)==secondIter);
    ASSERT_require(imap.find(119)==secondIter);
    ASSERT_require(imap.find(120)==imap.nodes().end());

    // Re-insert the middle part
    std::cerr <<"insert([105,114], v1)\n";
    imap.insert(Interval(105, 114), v1);
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==1);

    // Overwrite the right end with a different value
    std::cerr <<"insert(119, v2)\n";
    imap.insert(119, v2);
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==2);

    // Overwrite the left end with a different value
    std::cerr <<"insert(100, v2)\n";
    imap.insert(100, v2);
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==3);

    // Overwrite in the middle
    std::cerr <<"insert(109, v2)\n";
    imap.insert(109, v2);
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==5);

    // Overwrite joining to the left
    std::cerr <<"insert(110, v2)\n";
    imap.insert(110, v2);
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==5);

    // Overwrite joining to the right
    std::cerr <<"insert(108, v2)\n";
    imap.insert(108, v2);
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==5);

    // Overwrite joining on both sides
    std::cerr <<"insert([101,107], v2)\n";
    imap.insert(Interval(101, 107), v2);
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==3);

    // Overwrite and delete joining on both sides
    std::cerr <<"insert([109,118], v2)\n";
    imap.insert(Interval(109, 118), v2);
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==1);
}

// Test splitting and joining in more complex ways.  We'll store values that are the same as the intervals where they're
// stored.
template<class I>
class IntervalPolicy {
public:
    typedef I Interval;
    typedef I Value;

    bool merge(const Interval &leftInterval, Value &leftValue, const Interval &rightInterval, Value &rightValue) {
        ASSERT_forbid(empty(leftInterval));
        ASSERT_forbid(empty(rightInterval));
        ASSERT_require(equal(leftInterval, leftValue));
        ASSERT_require(equal(rightInterval, rightValue));
        ASSERT_require(leftInterval.upper() + 1 == rightInterval.lower());
        leftValue = Interval(leftValue.lower(), rightValue.upper());
        return true;
    }

    Value split(const Interval &interval, Value &value, const typename Interval::base_type &splitPoint) {
        ASSERT_forbid(empty(interval));
        ASSERT_require(equal(interval, value));
        ASSERT_require(splitPoint > interval.lower() && splitPoint <= interval.upper());
        Value right(splitPoint, value.upper());
        value = Value(value.lower(), splitPoint-1);
        return right;
    }

    void truncate(const Interval &interval, Value &value, const typename Interval::base_type &splitPoint) {
        split(interval, value, splitPoint);
    }
};

template<class Interval>
static void imap_policy_tests() {
    typedef Sawyer::Container::IntervalMap<Interval, Interval, IntervalPolicy<Interval> > Map;
    Map imap;

    std::cerr <<"insert([100,119], [100,119])\n";
    imap.insert(Interval(100, 119), Interval(100, 119));
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==1);

    // Erase the right part
    std::cerr <<"erase([110,119])\n";
    imap.erase(Interval(110, 119));
    show(imap);
    ASSERT_require(imap.size()==10);
    ASSERT_require(imap.nIntervals()==1);

    // Re-insert the right part
    std::cerr <<"insert([110,119], [110,119])\n";
    imap.insert(Interval(110, 119), Interval(110, 119));
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==1);

    // Erase the left part
    std::cerr <<"erase([100,109])\n";
    imap.erase(Interval(100, 109));
    show(imap);
    ASSERT_require(imap.size()==10);
    ASSERT_require(imap.nIntervals()==1);

    // Re-insert the left part
    std::cerr <<"insert([100,109], [100,109])\n";
    imap.insert(Interval(100, 109), Interval(100, 109));
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==1);

    // Erase the middle part
    std::cerr <<"erase([105,114])\n";
    imap.erase(Interval(105, 114));
    show(imap);
    ASSERT_require(imap.size()==10);
    ASSERT_require(imap.nIntervals()==2);

    // Re-insert the middle part
    std::cerr <<"insert([105,114], [105,114])\n";
    imap.insert(Interval(105, 114), Interval(105, 114));
    show(imap);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==1);
}

static void search_tests() {
    typedef boost::numeric::interval<int> Interval;
    typedef Sawyer::Container::IntervalMap<Interval, int> IMap;
    typedef Sawyer::Container::IntervalMap<Interval, float> IMap2;
    IMap imap;
    IMap2 map2;

    imap.insert(Interval(100, 109), 0);
    imap.insert(Interval(120, 129), 0);
    ASSERT_require(imap.size()==20);
    ASSERT_require(imap.nIntervals()==2);

    IMap::NodeIterator first = imap.nodes().begin();
    IMap::NodeIterator second = first; ++second;
    IMap::NodeIterator none = imap.nodes().end();

    ASSERT_require(imap.findFirstOverlap(Interval(-1, 10))==none);
    ASSERT_require(imap.findFirstOverlap(Interval(98, 99))==none);

    ASSERT_require(imap.findFirstOverlap(Interval(99, 100))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(99, 109))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(99, 110))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(99, 119))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(99, 120))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(99, 129))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(99, 130))==first);
    
    ASSERT_require(imap.findFirstOverlap(Interval(100, 100))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(100, 109))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(100, 110))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(100, 119))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(100, 120))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(100, 129))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(100, 130))==first);
    
    ASSERT_require(imap.findFirstOverlap(Interval(109, 109))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(109, 110))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(109, 119))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(109, 120))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(109, 129))==first);
    ASSERT_require(imap.findFirstOverlap(Interval(109, 130))==first);
    
    ASSERT_require(imap.findFirstOverlap(Interval(110, 110))==none);
    ASSERT_require(imap.findFirstOverlap(Interval(110, 119))==none);
    ASSERT_require(imap.findFirstOverlap(Interval(110, 120))==second);
    ASSERT_require(imap.findFirstOverlap(Interval(110, 129))==second);
    ASSERT_require(imap.findFirstOverlap(Interval(110, 130))==second);
    
    ASSERT_require(imap.findFirstOverlap(Interval(119, 119))==none);
    ASSERT_require(imap.findFirstOverlap(Interval(119, 120))==second);
    ASSERT_require(imap.findFirstOverlap(Interval(119, 129))==second);
    ASSERT_require(imap.findFirstOverlap(Interval(119, 130))==second);
    
    ASSERT_require(imap.findFirstOverlap(Interval(120, 120))==second);
    ASSERT_require(imap.findFirstOverlap(Interval(120, 129))==second);
    ASSERT_require(imap.findFirstOverlap(Interval(120, 130))==second);

    ASSERT_require(imap.findFirstOverlap(Interval(129, 129))==second);
    ASSERT_require(imap.findFirstOverlap(Interval(129, 130))==second);
    
    ASSERT_require(imap.findFirstOverlap(Interval(130, 130))==none);

    map2.clear();
    map2.insert(Interval(10, 99), 1.0);
    map2.insert(Interval(110, 119), 2.0);
    map2.insert(Interval(130, 139), 3.0);
    ASSERT_require(imap.findFirstOverlap(imap.nodes().begin(), map2, map2.nodes().begin()).first==none);

    map2.clear();
    map2.insert(Interval(10, 99), 1.0);
    map2.insert(105, 1.5);
    map2.insert(Interval(110, 119), 2.0);
    map2.insert(Interval(130, 139), 3.0);
    ASSERT_require(imap.findFirstOverlap(imap.nodes().begin(), map2, map2.nodes().begin()).first==first);

    map2.clear();
    map2.insert(Interval(10, 99), 1.0);
    map2.insert(Interval(110, 139), 2.0);
    ASSERT_require(imap.findFirstOverlap(imap.nodes().begin(), map2, map2.nodes().begin()).first==second);
}

// Use std::pair as a value.  Works since it satisfies the minimal API: copy constructor, assignment operator, and equality.
// The output function is only needed for this test file so we can print the contents of the IntervalMap
typedef std::pair<int, long> Pair;
std::ostream& operator<<(std::ostream &o, const Pair &p) {
    o <<"(" <<p.first <<"," <<p.second <<")";
    return o;
}

// All we need for storing a value in an IntervalMap is that it has a copy constructor, an assignment operator,
// and an equality operator
class MinimalApi {
    int id;
    MinimalApi();
public:
    MinimalApi(int id): id(id) {}                                       // only for testing
    int getId() const { return id; }                                    // only for testing

    MinimalApi(const MinimalApi &other) { id=other.id; }
    MinimalApi& operator=(const MinimalApi &other) { id=other.id; return *this; }
    bool operator==(const MinimalApi &other) {
        return id==other.id;
    }
};
std::ostream& operator<<(std::ostream &o, const MinimalApi &v) {        // only for testing
    o <<"MinimalApi(" <<v.getId() <<")";
    return o;
}

template<class Interval>
static void basic_set_tests() {
    typedef Sawyer::Container::IntervalSet<Interval> Set;
    Set set;

    Interval all(boost::integer_traits<typename Interval::base_type>::const_min,
                 boost::integer_traits<typename Interval::base_type>::const_max);

    std::cerr <<"invert() empty set\n";
    typename Interval::base_type allSize = (all.upper()-all.lower()) + 1;// must cast because width(all) returns wrong type
    set.invert();
    show(set);
    ASSERT_require(!set.isEmpty());
    ASSERT_require(set.size()==allSize);
    ASSERT_require(set.nIntervals()==1);

    std::cerr <<"invert() all set\n";
    set.invert();
    show(set);
    ASSERT_require(set.isEmpty());
    ASSERT_require(set.size()==0);
    ASSERT_require(set.nIntervals()==0);

    std::cerr <<"insert([100,119])\n";
    set.insert(Interval(100, 119));
    show(set);
    ASSERT_require(set.size()==20);
    ASSERT_require(set.nIntervals()==1);

    // Erase the right part
    std::cerr <<"erase([110,119])\n";
    set.erase(Interval(110, 119));
    show(set);
    ASSERT_require(set.size()==10);
    ASSERT_require(set.nIntervals()==1);

    // Re-insert the right part
    std::cerr <<"insert([110,119])\n";
    set.insert(Interval(110, 119));
    show(set);
    ASSERT_require(set.size()==20);
    ASSERT_require(set.nIntervals()==1);

    // Erase the left part
    std::cerr <<"erase([100,109])\n";
    set.erase(Interval(100, 109));
    show(set);
    ASSERT_require(set.size()==10);
    ASSERT_require(set.nIntervals()==1);

    // Re-insert the left part
    std::cerr <<"insert([100,109])\n";
    set.insert(Interval(100, 109));
    show(set);
    ASSERT_require(set.size()==20);
    ASSERT_require(set.nIntervals()==1);

    // Erase the middle part
    std::cerr <<"erase([105,114])\n";
    set.erase(Interval(105, 114));
    show(set);
    ASSERT_require(set.size()==10);
    ASSERT_require(set.nIntervals()==2);

    // Invert selection
    std::cerr <<"invert()\n";
    set.invert();
    show(set);
    ASSERT_require(set.size()==typename Interval::base_type(allSize-10));
    ASSERT_require(set.nIntervals()==3);

    // Invert again
    std::cerr <<"invert()\n";
    set.invert();
    show(set);
    ASSERT_require(set.size()==10);
    ASSERT_require(set.nIntervals()==2);
    
    // Test exist against multiple nodes
    ASSERT_require(set.contains(99)==false);
    ASSERT_require(set.contains(100)==true);
    ASSERT_require(set.contains(104)==true);
    ASSERT_require(set.contains(105)==false);
    ASSERT_require(set.contains(114)==false);
    ASSERT_require(set.contains(115)==true);
    ASSERT_require(set.contains(119)==true);
    ASSERT_require(set.contains(120)==false);

    // Re-insert the middle part
    std::cerr <<"insert([105,114])\n";
    set.insert(Interval(105, 114));
    show(set);
    ASSERT_require(set.size()==20);
    ASSERT_require(set.nIntervals()==1);
}

template<class Interval>
static void set_ctor_tests() {
    typedef Sawyer::Container::IntervalMap<Interval, float> Map;
    Map map1;
    map1.insert(Interval(70, 79), 3.141);
    map1.insert(Interval(100, 109), 1.414);

    Map map2;
    map2.insert(65, 6.28);

    std::cerr <<"construct set from map\n";
    typedef Sawyer::Container::IntervalSet<Interval> Set;
    Set set1(map1);
    show(set1);
    ASSERT_require(set1.size()==20);
    ASSERT_require(set1.nIntervals()==2);

    std::cerr <<"assign set from map\n";
    typedef Sawyer::Container::IntervalSet<Interval> Set;
    set1 = map2;
    show(set1);
    ASSERT_require(set1.size()==1);
    ASSERT_require(set1.nIntervals()==1);
}

template<class Interval>
static void set_iterators_tests() {
    std::cerr <<"value iterator\n";

    typedef Sawyer::Container::IntervalSet<Interval> Set;
    Set set;

    set.insert(1);
    set.insert(Interval(3, 5));
    set.insert(Interval(7, 9));
    show(set);

    std::cerr <<"  scalar value iterator {";
    BOOST_FOREACH (const typename Set::Scalar &scalar, set.scalars())
        std::cerr <<" " <<scalar;
    std::cerr <<" }\n";
}

int main() {

    // Test that UnsignedExtent behaves as an interval
    interval_tests<UnsignedExtent<unsigned> >();
    interval_tests<UnsignedExtent<unsigned long> >();
    interval_tests<UnsignedExtent<boost::uint64_t> >();
    interval_tests<UnsignedExtent<unsigned short> >();
    interval_tests<UnsignedExtent<boost::uint8_t> >();

    // Test that UnsignedExtent can be used in an IntervalMap
    imap_tests<UnsignedExtent<unsigned>, int>(1, 2);
    imap_tests<UnsignedExtent<unsigned long>, int>(1, 2);
    imap_tests<UnsignedExtent<boost::uint64_t>, int>(1, 2);
    imap_tests<UnsignedExtent<unsigned short>, int>(1, 2);
    imap_tests<UnsignedExtent<boost::uint8_t>, int>(1, 2);

    // Test that other intervals can be used in an IntervalMap even if they don't behave
    // like UnsignedExtent (IntervalMap should not depend on anything specific to UnsignedExtent).
    imap_tests<boost::numeric::interval<int>, int>(1, 2);
    imap_tests<boost::numeric::interval<boost::uint64_t>, int>(1, 2);
    imap_tests<boost::numeric::interval<double>, int>(1, 2);

    // Test whether other types can be used as the values in an IntervalMap
    imap_tests<UnsignedExtent<unsigned>, short>(1, 2);
    imap_tests<UnsignedExtent<unsigned>, double>(1.0, 2.0);
    imap_tests<UnsignedExtent<unsigned>, bool>(false, true);
    imap_tests<UnsignedExtent<unsigned>, Pair>(Pair(1, 2), Pair(3, 4));
    imap_tests<UnsignedExtent<unsigned>, MinimalApi>(MinimalApi(0), MinimalApi(1));

    // Check that merging and splitting of values works correctly.
    imap_policy_tests<UnsignedExtent<unsigned> >();
    imap_policy_tests<boost::numeric::interval<boost::uint64_t> >();
    imap_policy_tests<boost::numeric::interval<double> >();

    // others
    search_tests();

    // Basic IntervalSet tests
    basic_set_tests<UnsignedExtent<unsigned> >();
    basic_set_tests<UnsignedExtent<unsigned long> >();
    basic_set_tests<UnsignedExtent<boost::uint64_t> >();
    basic_set_tests<UnsignedExtent<unsigned short> >();
    basic_set_tests<UnsignedExtent<boost::uint8_t> >();

    // Basic IntervalSet tests using boost intervals
    basic_set_tests<boost::numeric::interval<int> >();
    basic_set_tests<boost::numeric::interval<boost::uint64_t> >();

    // IntervalSet constructors
    set_ctor_tests<UnsignedExtent<unsigned> >();
    set_ctor_tests<UnsignedExtent<unsigned long> >();
    set_ctor_tests<UnsignedExtent<boost::uint64_t> >();
    set_ctor_tests<UnsignedExtent<unsigned short> >();
    set_ctor_tests<UnsignedExtent<boost::uint8_t> >();
    set_ctor_tests<boost::numeric::interval<int> >();
    set_ctor_tests<boost::numeric::interval<boost::uint64_t> >();

    set_iterators_tests<UnsignedExtent<unsigned> >();

    return 0;
}

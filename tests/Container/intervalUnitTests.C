#include <sawyer/Interval.h>
#include <sawyer/IntervalSet.h>
#include <sawyer/Optional.h>
#include <boost/foreach.hpp>
#include <iostream>

template<typename T>
std::ostream& operator<<(std::ostream &o, const Sawyer::Container::Interval<T> &interval) {
    o <<"[" <<interval.least() <<"," <<interval.greatest() <<"]";
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
        std::cerr <<" [" <<interval.least() <<"," <<interval.greatest() <<"]=" <<value;
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
        std::cerr <<" [" <<interval.least() <<"," <<interval.greatest() <<"]";
    }
    std::cerr <<" }\n";
}

// Some basic interval tests
template<class Interval>
static void interval_tests() {

    // Unlike boost::numeric::interval, our default constructor creates an empty interval
    Interval e1;
    ASSERT_always_require(e1.isEmpty());
    ASSERT_always_require(e1.size()==0);

    // boost::numeric::interval_lib::width() is weird: it is as if greatest() is not included in the interval.
    // Our version of size() is better, but it can overflow to zero.
    Interval e2(1);
    ASSERT_always_forbid(e2.isEmpty());
    ASSERT_always_require(e2.size()==1);

    Interval e3 = Interval::hull(0, boost::integer_traits<typename Interval::Value>::const_max);
    ASSERT_always_forbid(e3.isEmpty());
    ASSERT_always_require(e1.size()==0);                       // because of overflow
}

template<class Interval, class Value>
static void imap_tests(const Value &v1, const Value &v2) {
    typedef Sawyer::Container::IntervalMap<Interval, Value> Map;
    typedef typename Interval::Value Scalar;
    Map imap;
    Sawyer::Optional<Scalar> opt;

    std::cerr <<"insert([100,119], v1)\n";
    imap.insert(Interval::hull(100, 119), v1);
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==1);

    // Test lowerBound against one node
    ASSERT_always_require(imap.lowerBound(99)==imap.nodes().begin());
    ASSERT_always_require(imap.lowerBound(100)==imap.nodes().begin());
    ASSERT_always_require(imap.lowerBound(118)==imap.nodes().begin());
    ASSERT_always_require(imap.lowerBound(119)==imap.nodes().begin());
    ASSERT_always_require(imap.lowerBound(120)==imap.nodes().end());

    // Test findPrior against one node
    ASSERT_always_require(imap.findPrior(99)==imap.nodes().end());
    ASSERT_always_require(imap.findPrior(100)==imap.nodes().begin());
    ASSERT_always_require(imap.findPrior(119)==imap.nodes().begin());
    ASSERT_always_require(imap.findPrior(120)==imap.nodes().begin());

    // Test least() against one node
    ASSERT_always_require(imap.least()==100);
    opt = imap.least(Scalar(99));
    ASSERT_always_require(opt && *opt==100);
    opt = imap.least(Scalar(100));
    ASSERT_always_require(opt && *opt==100);
    opt = imap.least(Scalar(101));
    ASSERT_always_require(opt && *opt==101);
    opt = imap.least(Scalar(119));
    ASSERT_always_require(opt && *opt==119);
    opt = imap.least(Scalar(120));
    ASSERT_always_require(!opt);

    // Test greatest() against one node
    ASSERT_always_require(imap.greatest()==119);
    opt = imap.greatest(Scalar(120));
    ASSERT_always_require(opt && *opt==119);
    opt = imap.greatest(Scalar(119));
    ASSERT_always_require(opt && *opt==119);
    opt = imap.greatest(Scalar(118));
    ASSERT_always_require(opt && *opt==118);
    opt = imap.greatest(Scalar(100));
    ASSERT_always_require(opt && *opt==100);
    opt = imap.greatest(Scalar(99));
    ASSERT_always_require(!opt);

    // Erase the right part
    std::cerr <<"erase([110,119])\n";
    imap.erase(Interval::hull(110, 119));
    show(imap);
    ASSERT_always_require(imap.size()==10);
    ASSERT_always_require(imap.nIntervals()==1);

    // Re-insert the right part
    std::cerr <<"insert([110,119], v1)\n";
    imap.insert(Interval::hull(110, 119), v1);
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==1);

    // Erase the left part
    std::cerr <<"erase([100,109])\n";
    imap.erase(Interval::hull(100, 109));
    show(imap);
    ASSERT_always_require(imap.size()==10);
    ASSERT_always_require(imap.nIntervals()==1);

    // Re-insert the left part
    std::cerr <<"insert([100,109], v1)\n";
    imap.insert(Interval::hull(100, 109), v1);
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==1);

    // Erase the middle part
    std::cerr <<"erase([105,114])\n";
    imap.erase(Interval::hull(105, 114));
    show(imap);
    ASSERT_always_require(imap.size()==10);
    ASSERT_always_require(imap.nIntervals()==2);

    // Test lowerBound against multiple nodes
    typename Map::NodeIterator secondIter = imap.nodes().begin(); ++secondIter;
    ASSERT_always_require(imap.lowerBound(99)==imap.nodes().begin());
    ASSERT_always_require(imap.lowerBound(100)==imap.nodes().begin());
    ASSERT_always_require(imap.lowerBound(104)==imap.nodes().begin());
    ASSERT_always_require(imap.lowerBound(105)==secondIter);
    ASSERT_always_require(imap.lowerBound(114)==secondIter);
    ASSERT_always_require(imap.lowerBound(115)==secondIter);
    ASSERT_always_require(imap.lowerBound(119)==secondIter);
    ASSERT_always_require(imap.lowerBound(120)==imap.nodes().end());

    // Test findPrior against multiple nodes
    ASSERT_always_require(imap.findPrior(99)==imap.nodes().end());
    ASSERT_always_require(imap.findPrior(100)==imap.nodes().begin());
    ASSERT_always_require(imap.findPrior(104)==imap.nodes().begin());
    ASSERT_always_require(imap.findPrior(105)==imap.nodes().begin());
    ASSERT_always_require(imap.findPrior(114)==imap.nodes().begin());
    ASSERT_always_require(imap.findPrior(115)==secondIter);
    ASSERT_always_require(imap.findPrior(119)==secondIter);
    ASSERT_always_require(imap.findPrior(120)==secondIter);

    // Test find against multiple nodes
    ASSERT_always_require(imap.find(99)==imap.nodes().end());
    ASSERT_always_require(imap.find(100)==imap.nodes().begin());
    ASSERT_always_require(imap.find(104)==imap.nodes().begin());
    ASSERT_always_require(imap.find(105)==imap.nodes().end());
    ASSERT_always_require(imap.find(114)==imap.nodes().end());
    ASSERT_always_require(imap.find(115)==secondIter);
    ASSERT_always_require(imap.find(119)==secondIter);
    ASSERT_always_require(imap.find(120)==imap.nodes().end());

    // Overwrite the right end with a different value
    std::cerr <<"insert(119, v2)\n";
    imap.insert(119, v2);
    show(imap);
    ASSERT_always_require(imap.size()==10);
    ASSERT_always_require(imap.nIntervals()==3);

    // Overwrite the left end with a different value
    std::cerr <<"insert(100, v2)\n";
    imap.insert(100, v2);
    show(imap);
    ASSERT_always_require(imap.size()==10);
    ASSERT_always_require(imap.nIntervals()==4);

    // Test least against multiple nodes
    ASSERT_always_require(imap.least()==100);
    opt = imap.least(Scalar(99));  ASSERT_always_require(opt && *opt==100);
    opt = imap.least(Scalar(100)); ASSERT_always_require(opt && *opt==100);
    opt = imap.least(Scalar(101)); ASSERT_always_require(opt && *opt==101);
    opt = imap.least(Scalar(102)); ASSERT_always_require(opt && *opt==102);
    opt = imap.least(Scalar(103)); ASSERT_always_require(opt && *opt==103);
    opt = imap.least(Scalar(104)); ASSERT_always_require(opt && *opt==104);
    opt = imap.least(Scalar(105)); ASSERT_always_require(opt && *opt==115);
    opt = imap.least(Scalar(114)); ASSERT_always_require(opt && *opt==115);
    opt = imap.least(Scalar(115)); ASSERT_always_require(opt && *opt==115);
    opt = imap.least(Scalar(116)); ASSERT_always_require(opt && *opt==116);
    opt = imap.least(Scalar(117)); ASSERT_always_require(opt && *opt==117);
    opt = imap.least(Scalar(118)); ASSERT_always_require(opt && *opt==118);
    opt = imap.least(Scalar(119)); ASSERT_always_require(opt && *opt==119);
    opt = imap.least(Scalar(120)); ASSERT_always_require(!opt);

    // Test greatest() against multiple nodes
    ASSERT_always_require(imap.greatest()==119);
    opt = imap.greatest(Scalar(99));  ASSERT_always_require(!opt);
    opt = imap.greatest(Scalar(100)); ASSERT_always_require(opt && *opt==100);
    opt = imap.greatest(Scalar(101)); ASSERT_always_require(opt && *opt==101);
    opt = imap.greatest(Scalar(102)); ASSERT_always_require(opt && *opt==102);
    opt = imap.greatest(Scalar(103)); ASSERT_always_require(opt && *opt==103);
    opt = imap.greatest(Scalar(104)); ASSERT_always_require(opt && *opt==104);
    opt = imap.greatest(Scalar(105)); ASSERT_always_require(opt && *opt==104);
    opt = imap.greatest(Scalar(114)); ASSERT_always_require(opt && *opt==104);
    opt = imap.greatest(Scalar(115)); ASSERT_always_require(opt && *opt==115);
    opt = imap.greatest(Scalar(116)); ASSERT_always_require(opt && *opt==116);
    opt = imap.greatest(Scalar(117)); ASSERT_always_require(opt && *opt==117);
    opt = imap.greatest(Scalar(118)); ASSERT_always_require(opt && *opt==118);
    opt = imap.greatest(Scalar(119)); ASSERT_always_require(opt && *opt==119);
    opt = imap.greatest(Scalar(120)); ASSERT_always_require(opt && *opt==119);

    // Test leastUnmapped against multiple nodes
    opt = imap.leastUnmapped(Scalar(0  )); ASSERT_always_require(opt && *opt==0);
    opt = imap.leastUnmapped(Scalar(99 )); ASSERT_always_require(opt && *opt==99);
    opt = imap.leastUnmapped(Scalar(100)); ASSERT_always_require(opt && *opt==105);
    opt = imap.leastUnmapped(Scalar(101)); ASSERT_always_require(opt && *opt==105);
    opt = imap.leastUnmapped(Scalar(102)); ASSERT_always_require(opt && *opt==105);
    opt = imap.leastUnmapped(Scalar(103)); ASSERT_always_require(opt && *opt==105);
    opt = imap.leastUnmapped(Scalar(104)); ASSERT_always_require(opt && *opt==105);
    opt = imap.leastUnmapped(Scalar(105)); ASSERT_always_require(opt && *opt==105);
    opt = imap.leastUnmapped(Scalar(114)); ASSERT_always_require(opt && *opt==114);
    opt = imap.leastUnmapped(Scalar(115)); ASSERT_always_require(opt && *opt==120);
    opt = imap.leastUnmapped(Scalar(116)); ASSERT_always_require(opt && *opt==120);
    opt = imap.leastUnmapped(Scalar(117)); ASSERT_always_require(opt && *opt==120);
    opt = imap.leastUnmapped(Scalar(118)); ASSERT_always_require(opt && *opt==120);
    opt = imap.leastUnmapped(Scalar(119)); ASSERT_always_require(opt && *opt==120);
    opt = imap.leastUnmapped(Scalar(120)); ASSERT_always_require(opt && *opt==120);
    opt = imap.leastUnmapped(Scalar(121)); ASSERT_always_require(opt && *opt==121);

    // Test greatestUnmapped against multiple nodes
    opt = imap.greatestUnmapped(Scalar(0  )); ASSERT_always_require(opt && *opt==0);
    opt = imap.greatestUnmapped(Scalar(99 )); ASSERT_always_require(opt && *opt==99);
    opt = imap.greatestUnmapped(Scalar(100)); ASSERT_always_require(opt && *opt==99);
    opt = imap.greatestUnmapped(Scalar(101)); ASSERT_always_require(opt && *opt==99);
    opt = imap.greatestUnmapped(Scalar(102)); ASSERT_always_require(opt && *opt==99);
    opt = imap.greatestUnmapped(Scalar(103)); ASSERT_always_require(opt && *opt==99);
    opt = imap.greatestUnmapped(Scalar(104)); ASSERT_always_require(opt && *opt==99);
    opt = imap.greatestUnmapped(Scalar(105)); ASSERT_always_require(opt && *opt==105);
    opt = imap.greatestUnmapped(Scalar(114)); ASSERT_always_require(opt && *opt==114);
    opt = imap.greatestUnmapped(Scalar(115)); ASSERT_always_require(opt && *opt==114);
    opt = imap.greatestUnmapped(Scalar(116)); ASSERT_always_require(opt && *opt==114);
    opt = imap.greatestUnmapped(Scalar(117)); ASSERT_always_require(opt && *opt==114);
    opt = imap.greatestUnmapped(Scalar(118)); ASSERT_always_require(opt && *opt==114);
    opt = imap.greatestUnmapped(Scalar(119)); ASSERT_always_require(opt && *opt==114);
    opt = imap.greatestUnmapped(Scalar(120)); ASSERT_always_require(opt && *opt==120);
    opt = imap.greatestUnmapped(Scalar(121)); ASSERT_always_require(opt && *opt==121);

    // Re-insert the middle part
    std::cerr <<"insert([105,114], v1)\n";
    imap.insert(Interval::hull(105, 114), v1);
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==3);

    // Overwrite in the middle
    std::cerr <<"insert(109, v2)\n";
    imap.insert(109, v2);
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==5);

    // Overwrite joining to the left
    std::cerr <<"insert(110, v2)\n";
    imap.insert(110, v2);
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==5);

    // Overwrite joining to the right
    std::cerr <<"insert(108, v2)\n";
    imap.insert(108, v2);
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==5);

    // Overwrite joining on both sides
    std::cerr <<"insert([101,107], v2)\n";
    imap.insert(Interval::hull(101, 107), v2);
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==3);

    // Overwrite and delete joining on both sides
    std::cerr <<"insert([109,118], v2)\n";
    imap.insert(Interval::hull(109, 118), v2);
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==1);
}

// Test splitting and joining in more complex ways.  We'll store values that are the same as the intervals where they're
// stored.
template<class I>
class IntervalPolicy {
public:
    typedef I Interval;
    typedef I Value;

    bool merge(const Interval &leftInterval, Value &leftValue, const Interval &rightInterval, Value &rightValue) {
        ASSERT_always_forbid(leftInterval.isEmpty());
        ASSERT_always_forbid(rightInterval.isEmpty());
        ASSERT_always_require(leftInterval == leftValue);
        ASSERT_always_require(rightInterval == rightValue);
        ASSERT_always_require(leftInterval.greatest() + 1 == rightInterval.least());
        leftValue = Interval::hull(leftValue.least(), rightValue.greatest());
        return true;
    }

    Value split(const Interval &interval, Value &value, const typename Interval::Value &splitPoint) {
        ASSERT_always_forbid(interval.isEmpty());
        ASSERT_always_require(interval == value);
        ASSERT_always_require(splitPoint > interval.least() && splitPoint <= interval.greatest());
        Value right = Value::hull(splitPoint, value.greatest());
        value = Value::hull(value.least(), splitPoint-1);
        return right;
    }

    void truncate(const Interval &interval, Value &value, const typename Interval::Value &splitPoint) {
        split(interval, value, splitPoint);
    }
};

template<class Interval>
static void imap_policy_tests() {
    typedef Sawyer::Container::IntervalMap<Interval, Interval, IntervalPolicy<Interval> > Map;
    Map imap;

    std::cerr <<"insert([100,119], [100,119])\n";
    imap.insert(Interval::hull(100, 119), Interval::hull(100, 119));
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==1);

    // Erase the right part
    std::cerr <<"erase([110,119])\n";
    imap.erase(Interval::hull(110, 119));
    show(imap);
    ASSERT_always_require(imap.size()==10);
    ASSERT_always_require(imap.nIntervals()==1);

    // Re-insert the right part
    std::cerr <<"insert([110,119], [110,119])\n";
    imap.insert(Interval::hull(110, 119), Interval::hull(110, 119));
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==1);

    // Erase the left part
    std::cerr <<"erase([100,109])\n";
    imap.erase(Interval::hull(100, 109));
    show(imap);
    ASSERT_always_require(imap.size()==10);
    ASSERT_always_require(imap.nIntervals()==1);

    // Re-insert the left part
    std::cerr <<"insert([100,109], [100,109])\n";
    imap.insert(Interval::hull(100, 109), Interval::hull(100, 109));
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==1);

    // Erase the middle part
    std::cerr <<"erase([105,114])\n";
    imap.erase(Interval::hull(105, 114));
    show(imap);
    ASSERT_always_require(imap.size()==10);
    ASSERT_always_require(imap.nIntervals()==2);

    // Re-insert the middle part
    std::cerr <<"insert([105,114], [105,114])\n";
    imap.insert(Interval::hull(105, 114), Interval::hull(105, 114));
    show(imap);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==1);
}

static void search_tests() {
    typedef Sawyer::Container::Interval<int> Interval;
    typedef Sawyer::Container::IntervalMap<Interval, int> IMap;
    typedef Sawyer::Container::IntervalMap<Interval, float> IMap2;
    IMap imap;
    IMap2 map2;

    imap.insert(Interval::hull(100, 109), 0);
    imap.insert(Interval::hull(120, 129), 0);
    ASSERT_always_require(imap.size()==20);
    ASSERT_always_require(imap.nIntervals()==2);

    IMap::NodeIterator first = imap.nodes().begin();
    IMap::NodeIterator second = first; ++second;
    IMap::NodeIterator none = imap.nodes().end();

    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(-1, 10))==none);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(98, 99))==none);

    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(99, 100))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(99, 109))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(99, 110))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(99, 119))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(99, 120))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(99, 129))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(99, 130))==first);
    
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(100, 100))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(100, 109))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(100, 110))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(100, 119))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(100, 120))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(100, 129))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(100, 130))==first);
    
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(109, 109))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(109, 110))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(109, 119))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(109, 120))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(109, 129))==first);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(109, 130))==first);
    
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(110, 110))==none);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(110, 119))==none);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(110, 120))==second);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(110, 129))==second);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(110, 130))==second);
    
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(119, 119))==none);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(119, 120))==second);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(119, 129))==second);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(119, 130))==second);
    
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(120, 120))==second);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(120, 129))==second);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(120, 130))==second);

    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(129, 129))==second);
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(129, 130))==second);
    
    ASSERT_always_require(imap.findFirstOverlap(Interval::hull(130, 130))==none);

    map2.clear();
    map2.insert(Interval::hull(10, 99), 1.0);
    map2.insert(Interval::hull(110, 119), 2.0);
    map2.insert(Interval::hull(130, 139), 3.0);
    ASSERT_always_require(imap.findFirstOverlap(imap.nodes().begin(), map2, map2.nodes().begin()).first==none);

    map2.clear();
    map2.insert(Interval::hull(10, 99), 1.0);
    map2.insert(105, 1.5);
    map2.insert(Interval::hull(110, 119), 2.0);
    map2.insert(Interval::hull(130, 139), 3.0);
    ASSERT_always_require(imap.findFirstOverlap(imap.nodes().begin(), map2, map2.nodes().begin()).first==first);

    map2.clear();
    map2.insert(Interval::hull(10, 99), 1.0);
    map2.insert(Interval::hull(110, 139), 2.0);
    ASSERT_always_require(imap.findFirstOverlap(imap.nodes().begin(), map2, map2.nodes().begin()).first==second);
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

    Interval all = Interval::hull(boost::integer_traits<typename Interval::Value>::const_min,
                                  boost::integer_traits<typename Interval::Value>::const_max);

    std::cerr <<"invert() empty set\n";
    typename Interval::Value allSize = (all.greatest()-all.least()) + 1;// must cast because width(all) returns wrong type
    set.invert(all);
    show(set);
    ASSERT_always_require(!set.isEmpty());
    ASSERT_always_require(set.size()==allSize);
    ASSERT_always_require(set.nIntervals()==1);

    std::cerr <<"invert() all set\n";
    set.invert(all);
    show(set);
    ASSERT_always_require(set.isEmpty());
    ASSERT_always_require(set.size()==0);
    ASSERT_always_require(set.nIntervals()==0);

    std::cerr <<"insert([100,119])\n";
    set.insert(Interval::hull(100, 119));
    show(set);
    ASSERT_always_require(set.size()==20);
    ASSERT_always_require(set.nIntervals()==1);

    // Erase the right part
    std::cerr <<"erase([110,119])\n";
    set.erase(Interval::hull(110, 119));
    show(set);
    ASSERT_always_require(set.size()==10);
    ASSERT_always_require(set.nIntervals()==1);

    // Re-insert the right part
    std::cerr <<"insert([110,119])\n";
    set.insert(Interval::hull(110, 119));
    show(set);
    ASSERT_always_require(set.size()==20);
    ASSERT_always_require(set.nIntervals()==1);

    // Erase the left part
    std::cerr <<"erase([100,109])\n";
    set.erase(Interval::hull(100, 109));
    show(set);
    ASSERT_always_require(set.size()==10);
    ASSERT_always_require(set.nIntervals()==1);

    // Re-insert the left part
    std::cerr <<"insert([100,109])\n";
    set.insert(Interval::hull(100, 109));
    show(set);
    ASSERT_always_require(set.size()==20);
    ASSERT_always_require(set.nIntervals()==1);

    // Erase the middle part
    std::cerr <<"erase([105,114])\n";
    set.erase(Interval::hull(105, 114));
    show(set);
    ASSERT_always_require(set.size()==10);
    ASSERT_always_require(set.nIntervals()==2);

    // Invert selection
    std::cerr <<"invert()\n";
    set.invert(all);
    show(set);
    ASSERT_always_require(set.size()==typename Interval::Value(allSize-10));
    ASSERT_always_require(set.nIntervals()==3);

    // Invert again
    std::cerr <<"invert()\n";
    set.invert(all);
    show(set);
    ASSERT_always_require(set.size()==10);
    ASSERT_always_require(set.nIntervals()==2);
    
    // Test exist against multiple nodes
    ASSERT_always_require(set.contains(99)==false);
    ASSERT_always_require(set.contains(100)==true);
    ASSERT_always_require(set.contains(104)==true);
    ASSERT_always_require(set.contains(105)==false);
    ASSERT_always_require(set.contains(114)==false);
    ASSERT_always_require(set.contains(115)==true);
    ASSERT_always_require(set.contains(119)==true);
    ASSERT_always_require(set.contains(120)==false);

    // Re-insert the middle part
    std::cerr <<"insert([105,114])\n";
    set.insert(Interval::hull(105, 114));
    show(set);
    ASSERT_always_require(set.size()==20);
    ASSERT_always_require(set.nIntervals()==1);
}

template<class Interval>
static void set_ctor_tests() {
    typedef Sawyer::Container::IntervalMap<Interval, float> Map;
    Map map1;
    map1.insert(Interval::hull(70, 79), 3.141);
    map1.insert(Interval::hull(100, 109), 1.414);

    Map map2;
    map2.insert(65, 6.28);

    std::cerr <<"construct set from map\n";
    typedef Sawyer::Container::IntervalSet<Interval> Set;
    Set set1(map1);
    show(set1);
    ASSERT_always_require(set1.size()==20);
    ASSERT_always_require(set1.nIntervals()==2);

    std::cerr <<"assign set from map\n";
    typedef Sawyer::Container::IntervalSet<Interval> Set;
    set1 = map2;
    show(set1);
    ASSERT_always_require(set1.size()==1);
    ASSERT_always_require(set1.nIntervals()==1);
}

template<class Interval>
static void set_iterators_tests() {
    std::cerr <<"value iterator\n";

    typedef Sawyer::Container::IntervalSet<Interval> Set;
    Set set;

    set.insert(1);
    set.insert(Interval::hull(3, 5));
    set.insert(Interval::hull(7, 9));
    show(set);

    std::cerr <<"  scalar value iterator {";
    BOOST_FOREACH (const typename Set::Scalar &scalar, set.scalars())
        std::cerr <<" " <<scalar;
    std::cerr <<" }\n";
}

int main() {

    // Basic interval tests
    interval_tests<Sawyer::Container::Interval<unsigned> >();
    interval_tests<Sawyer::Container::Interval<unsigned long> >();
    interval_tests<Sawyer::Container::Interval<boost::uint64_t> >();
    interval_tests<Sawyer::Container::Interval<unsigned short> >();
    interval_tests<Sawyer::Container::Interval<boost::uint8_t> >();

    // Test that Interval can be used in an IntervalMap
    imap_tests<Sawyer::Container::Interval<unsigned>, int>(1, 2);
    imap_tests<Sawyer::Container::Interval<unsigned long>, int>(1, 2);
    imap_tests<Sawyer::Container::Interval<boost::uint64_t>, int>(1, 2);
    imap_tests<Sawyer::Container::Interval<unsigned short>, int>(1, 2);
    imap_tests<Sawyer::Container::Interval<boost::uint8_t>, int>(1, 2);

    // Test some non-unsigned stuff
    imap_tests<Sawyer::Container::Interval<int>, int>(1, 2);
    imap_tests<Sawyer::Container::Interval<double>, int>(1, 2);

    // Test whether other types can be used as the values in an IntervalMap
    imap_tests<Sawyer::Container::Interval<unsigned>, short>(1, 2);
    imap_tests<Sawyer::Container::Interval<unsigned>, double>(1.0, 2.0);
    imap_tests<Sawyer::Container::Interval<unsigned>, bool>(false, true);
    imap_tests<Sawyer::Container::Interval<unsigned>, Pair>(Pair(1, 2), Pair(3, 4));
    imap_tests<Sawyer::Container::Interval<unsigned>, MinimalApi>(MinimalApi(0), MinimalApi(1));

    // Check that merging and splitting of values works correctly.
    imap_policy_tests<Sawyer::Container::Interval<unsigned> >();
    imap_policy_tests<Sawyer::Container::Interval<boost::uint64_t> >();
    imap_policy_tests<Sawyer::Container::Interval<double> >();

    // others
    search_tests();

    // Basic IntervalSet tests
    basic_set_tests<Sawyer::Container::Interval<unsigned> >();
    basic_set_tests<Sawyer::Container::Interval<unsigned long> >();
    basic_set_tests<Sawyer::Container::Interval<boost::uint64_t> >();
    basic_set_tests<Sawyer::Container::Interval<unsigned short> >();
    basic_set_tests<Sawyer::Container::Interval<boost::uint8_t> >();
    basic_set_tests<Sawyer::Container::Interval<int> >();

    // IntervalSet constructors
    set_ctor_tests<Sawyer::Container::Interval<unsigned> >();
    set_ctor_tests<Sawyer::Container::Interval<unsigned long> >();
    set_ctor_tests<Sawyer::Container::Interval<boost::uint64_t> >();
    set_ctor_tests<Sawyer::Container::Interval<unsigned short> >();
    set_ctor_tests<Sawyer::Container::Interval<boost::uint8_t> >();
    set_ctor_tests<Sawyer::Container::Interval<int> >();

    set_iterators_tests<Sawyer::Container::Interval<unsigned> >();

    return 0;
}

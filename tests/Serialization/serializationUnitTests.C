#include <Sawyer/BitVector.h>
#include <Sawyer/Interval.h>
#include <Sawyer/SharedObject.h>
#include <Sawyer/SharedPointer.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <sstream>

// Serialize, then unserialize
template<class T>
static void
serunser(const T &t_out, T &t_in) {
    std::ostringstream oss;
    boost::archive::text_oarchive out(oss);
    out <<t_out;
#if 0 // DEBUGGING [Robb Matzke 2016-10-30]
    std::cerr <<oss.str() <<"\n";
#endif

    std::istringstream iss(oss.str());
    boost::archive::text_iarchive in(iss);
    in >>t_in;
}

template<class T, class U>
static void
serunser(const T &t_out, const U &u_out, T &t_in, U &u_in) {
    std::ostringstream oss;
    boost::archive::text_oarchive out(oss);
    out <<t_out <<u_out;
#if 0 // DEBUGGING [Robb Matzke 2016-10-30]
    std::cerr <<oss.str() <<"\n";
#endif

    std::istringstream iss(oss.str());
    boost::archive::text_iarchive in(iss);
    in >>t_in >>u_in;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Polymorphic objects
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class T0_base {
public:
    int i;

    T0_base(): i(0) {}
    explicit T0_base(int i): i(i) {}
    virtual ~T0_base() {}
    virtual std::string name() const = 0;

    template<class S>
    void serialize(S &s, const unsigned version) {
        s & i;
    }
};

class T0_a: public T0_base {
public:
    int j;

    T0_a(): j(0) {}
    T0_a(int i, int j): T0_base(i), j(j) {}

    std::string name() const {
        return "T0_a";
    }

    template<class S>
    void serialize(S &s, const unsigned version) {
        s & boost::serialization::base_object<T0_base>(*this);
        s & j;
    }
};

class T0_b: public T0_base {
public:
    int j;

    T0_b(): j(0) {}
    T0_b(int i, int j): T0_base(i), j(j) {}

    std::string name() const {
        return "T0_b";
    }

    template<class S>
    void serialize(S &s, const unsigned version) {
        s & boost::serialization::base_object<T0_base>(*this);
        s & j;
    }
};

BOOST_CLASS_EXPORT(T0_a);
BOOST_CLASS_EXPORT(T0_b);

typedef Sawyer::SharedPointer<T0_base> T0_ptr;

static void
test00() {
    std::cerr <<"Polymorphic class A through base pointer\n";
    T0_a a1(1, 2);
    const T0_base *base_a_out = &a1, *base_a_in = NULL;
    serunser(base_a_out, base_a_in);
    ASSERT_always_require(base_a_in != NULL);
    ASSERT_always_require(base_a_in->name() == "T0_a");

    std::cerr <<"Polymorphic class B through base pointer\n";
    T0_b b1(3, 4);
    const T0_base *base_b_out = &b1, *base_b_in = NULL;
    serunser(base_b_out, base_b_in);
    ASSERT_always_require(base_b_in != NULL);
    ASSERT_always_require(base_b_in->name() == "T0_b");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BitVector
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void
test01() {
    std::cerr <<"BitVector: empty\n";
    Sawyer::Container::BitVector empty_out, empty_in(8, true);
    serunser(empty_out, empty_in);
    ASSERT_always_require(empty_out.compare(empty_in) == 0);

    std::cerr <<"BitVector: non-empty\n";
    Sawyer::Container::BitVector bv_out(800, true), bv_in;
    serunser(bv_out, bv_in);
    ASSERT_always_require(bv_out.compare(bv_in) == 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SharedPointer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct T02: Sawyer::SharedObject {
public:
    typedef Sawyer::SharedPointer<T02> Ptr;
private:
    int i_;
private:
    friend class boost::serialization::access;
    template<class S>
    void serialize(S &s, const unsigned version) {
        s & i_;
    }
protected:
    T02(int i = 0): i_(i) {}
public:
    static Ptr instance(int i = 0) {
        return Ptr(new T02(i));
    }
    bool operator==(const T02 &other) const {
        return i_ == other.i_;
    }
};

static void
test02() {
    T02::Ptr obj1 = T02::instance(1);

    std::cerr <<"SharedPointer: null\n";
    T02::Ptr p1_out, p1_in = obj1;
    ASSERT_always_require(ownershipCount(obj1) == 2);
    serunser(p1_out, p1_in);
    ASSERT_always_require(p1_in == NULL);
    ASSERT_always_require(ownershipCount(obj1) == 1);

    std::cerr <<"SharedPointer: non-null\n";
    T02::Ptr p2_out = obj1, p2_in;
    ASSERT_always_require(ownershipCount(obj1) == 2);
    serunser(p2_out, p2_in);
    ASSERT_always_require(p2_in != NULL);
    ASSERT_always_require(ownershipCount(obj1) == 2);
    ASSERT_always_require(ownershipCount(p2_in) == 1);  // new object was created

    std::cerr <<"SharedPointer: shared-object\n";
    T02::Ptr p3_out = p2_out, p3_in1, p3_in2;
    ASSERT_always_require(ownershipCount(p3_out) == 3);
    serunser(p2_out, p3_out, p3_in1, p3_in2);
    ASSERT_always_require(p3_in1 != NULL);
    ASSERT_always_require(p3_in1 == p3_in2);
    ASSERT_always_require(ownershipCount(p3_in1) == 2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Interval
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void
test03() {
    typedef Sawyer::Container::Interval<int> T03;

    std::cerr <<"Interval<int> empty\n";
    T03 empty_out, empty_in = 123;
    serunser(empty_out, empty_in);
    ASSERT_always_require(empty_in == empty_out);

    std::cerr <<"Interval<int> singleton\n";
    T03 single_out = -123, single_in;
    serunser(single_out, single_in);
    ASSERT_always_require(single_in == single_out);

    std::cerr <<"Interval<int> multiple values\n";
    T03 multi_out = T03::hull(-10, 10), multi_in;
    serunser(multi_out, multi_in);
    ASSERT_always_require(multi_in == multi_out);

    std::cerr <<"Interval<int> whole space\n";
    T03 whole_out = T03::whole(), whole_in;
    serunser(whole_out, whole_in);
    ASSERT_always_require(whole_in == whole_out);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int
main() {
    test00();
    test01();
    test02();
    test03();
}

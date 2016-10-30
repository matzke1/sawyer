#include <Sawyer/BitVector.h>
#include <Sawyer/SharedObject.h>
#include <Sawyer/SharedPointer.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
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

int
main() {
    test01();
    test02();
}

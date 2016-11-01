#include <Sawyer/AddressMap.h>
#include <Sawyer/AddressSegment.h>
#include <Sawyer/AllocatingBuffer.h>
#include <Sawyer/BitVector.h>
#include <Sawyer/FileSystem.h>
#include <Sawyer/Interval.h>
#include <Sawyer/IntervalMap.h>
#include <Sawyer/Map.h>
#include <Sawyer/MappedBuffer.h>
#include <Sawyer/NullBuffer.h>
#include <Sawyer/SharedObject.h>
#include <Sawyer/SharedPointer.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/filesystem.hpp>
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
// AllocatingBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef Sawyer::Container::AllocatingBuffer<size_t, int> T04;

// Since the derived class is a template, we can't call BOOST_CLASS_EXPORT until we know the template arguments.
BOOST_CLASS_EXPORT(T04);

static void
test04() {
    static const int data[] = {1, 2, 3};

    std::cerr <<"AllocatingBuffer<size_t,int> empty\n";
    T04::Ptr empty_out = T04::instance(0), empty_in;
    serunser(empty_out, empty_in);
    ASSERT_always_require(empty_in != NULL);
    ASSERT_always_require(empty_in->size() == 0);

    std::cerr <<"AllocatingBuffer<size_t,int> nonempty\n";
    T04::Ptr nonempty_out = T04::instance(3), nonempty_in;
    size_t nwrite = nonempty_out->write(data, 0, 3);
    ASSERT_always_require(nwrite == 3);
    serunser(nonempty_out, nonempty_in);
    ASSERT_always_require(nonempty_in != NULL);
    ASSERT_always_require(nonempty_in->size() == 3);
    int buf[3];
    buf[0] = buf[1] = buf[2] = 0;
    size_t nread = nonempty_in->read(buf, 0, 3);
    ASSERT_always_require(nread == 3);
    ASSERT_always_require(buf[0] = data[0]);
    ASSERT_always_require(buf[1] = data[1]);
    ASSERT_always_require(buf[2] = data[2]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MappedBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef Sawyer::Container::MappedBuffer<size_t, uint8_t> T05;

// Since the derived class is a template, we can't call BOOST_CLASS_EXPORT until we know the template arguments.
BOOST_CLASS_EXPORT(T05);

static void
test05() {
    static const uint8_t data[] = {1, 2, 3};

    Sawyer::FileSystem::TemporaryFile f;
    f.stream().write((const char*)data, sizeof data);
    ASSERT_always_require(f.stream().good());
    f.stream().close();

    std::cerr <<"MappedBuffer<size_t,uint8_t>\n";
    T05::Ptr out = T05::instance(f.name());
    ASSERT_always_require(out->size() == 3);

    T05::Ptr in;
    serunser(out, in);
    ASSERT_always_require(in != NULL);
    ASSERT_always_require(in->size() == 3);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NullBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef Sawyer::Container::NullBuffer<size_t, uint8_t> T06;

// Since the derived class is a template, we can't call BOOST_CLASS_EXPORT until we know the template arguments.
BOOST_CLASS_EXPORT(T06);

static void
test06() {
    std::cerr <<"NullBuffer<size_t,uint8_t>\n";
    T06::Ptr out = T06::instance(100);
    ASSERT_always_require(out->size() == 100);

    T06::Ptr in;
    serunser(out, in);
    ASSERT_always_require(in != NULL);
    ASSERT_always_require(in->size() == 100);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Map
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef Sawyer::Container::Map<int, int> T07;

static void
test07() {
    std::cerr <<"Map<int,int> empty\n";
    T07 empty_out, empty_in;
    empty_in.insert(1, 2);
    serunser(empty_out, empty_in);
    ASSERT_always_require(empty_in.isEmpty());

    std::cerr <<"Map<int,int> non-empty\n";
    T07 nonempty_out, nonempty_in;
    nonempty_out.insert(3, 4);
    nonempty_out.insert(5, 6);
    serunser(nonempty_out, nonempty_in);
    ASSERT_always_require(nonempty_in.size() == 2);
    ASSERT_always_require(nonempty_in.exists(3));
    ASSERT_always_require(nonempty_in[3] == 4);
    ASSERT_always_require(nonempty_in.exists(5));
    ASSERT_always_require(nonempty_in[5] == 6);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddressSegment<size_t, boost::uint8_t>
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef Sawyer::Container::AddressSegment<size_t, boost::uint8_t> T08;
typedef Sawyer::Container::AllocatingBuffer<size_t, boost::uint8_t> T08_buffer;

BOOST_CLASS_EXPORT(T08_buffer);

static void
test08() {
    static const boost::uint8_t data[] = {1, 2, 3, 4};

    std::cerr <<"AddressSegment<size_t, uint8_t>\n";

    T08_buffer::Ptr buf = T08_buffer::instance(4);
    size_t nwrite = buf->write(data, 0, 4);
    ASSERT_always_require(nwrite == 4);

    T08 out(buf, 1, Sawyer::Access::READABLE|Sawyer::Access::WRITABLE, "the_name");
    T08 in;
    serunser(out, in);

    ASSERT_always_require(in.buffer() != NULL);
    ASSERT_always_require(in.buffer()->size() == 4);
    ASSERT_always_require(in.offset() == 1);
    ASSERT_always_require(in.accessibility() == (Sawyer::Access::READABLE | Sawyer::Access::WRITABLE));
    ASSERT_always_require(in.name() == "the_name");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IntervalMap<unsigned, float>
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef Sawyer::Container::Interval<unsigned> T09_interval;
typedef Sawyer::Container::IntervalMap<T09_interval, float> T09;

static void
test09() {
    std::cerr <<"IntervalMap<unsigned, float> empty\n";
    T09 empty_out, empty_in;
    empty_in.insert(1, 2);
    serunser(empty_out, empty_in);
    ASSERT_always_require(empty_in.isEmpty());

    std::cerr <<"IntervalMap<unsigned, float> non-empty\n";
    T09 nonempty_out, nonempty_in;
    nonempty_out.insert(999, 100.0);
    nonempty_out.insert(T09_interval::hull(5, 7), 101.0);
    serunser(nonempty_out, nonempty_in);
    ASSERT_always_require(nonempty_in.size() == nonempty_out.size());
    ASSERT_always_require(nonempty_in.exists(999));
    ASSERT_always_require(nonempty_in[999] == nonempty_out[999]);
    ASSERT_always_require(nonempty_in.exists(5));
    ASSERT_always_require(nonempty_in[5] == nonempty_out[5]);
    ASSERT_always_require(nonempty_in.exists(6));
    ASSERT_always_require(nonempty_in[6] == nonempty_out[6]);
    ASSERT_always_require(nonempty_in.exists(7));
    ASSERT_always_require(nonempty_in[7] == nonempty_out[7]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddressMap<size_t, boost::uint8_t>
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef Sawyer::Container::Interval<size_t> T10_interval;
typedef Sawyer::Container::AddressMap<size_t, boost::uint8_t> T10;

static void
test10() {
    std::cerr <<"AddressMap empty\n";
    T10 empty_out, empty_in;
    serunser(empty_out, empty_in);
    ASSERT_always_require(empty_in.isEmpty());

    std::cerr <<"AddressMap non-empty\n";
    static const boost::uint8_t data[] = {65, 66, 67, 68};
    T10 nonempty_out, nonempty_in;
    nonempty_out.insert(T10_interval::baseSize(10, 4),
                        T10::Segment::anonymousInstance(4, Sawyer::Access::READABLE, "test_name"));
    size_t nwrite = nonempty_out.at(10).limit(4).write(data).size();
    ASSERT_always_require(nwrite == 4);
    serunser(nonempty_out, nonempty_in);
    ASSERT_always_require(nonempty_in.size() == nonempty_out.size());
    ASSERT_always_require(nonempty_in.at(10).available().size() == 4);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int
main() {
    test00();
    test01();
    test02();
    test03();
    test04();
    test05();
    test06();
    test07();
    test08();
    test09();
    test10();
}

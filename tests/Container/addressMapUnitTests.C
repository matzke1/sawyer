#include <sawyer/AddressMap.h>

#include <sawyer/AllocatingBuffer.h>
#include <sawyer/MappedBuffer.h>
#include <sawyer/StaticBuffer.h>

#include <iostream>

using namespace Sawyer::Container;

// Test that all operations that can be applied to constant maps compile whether the map is const or mutable
template<class Map>
void compile_test_const(Map &s) {
    typedef typename Map::Address Address;
    typedef typename Map::Value Value;
    typedef Interval<typename Map::Address> Addresses;
    typedef typename Map::Segment Segment;

    // Test constructors
    Map s2;
    Map s3(s);

    // Test that constraints can be generated from a map
    s.require(Map::READABLE);
    s.prohibit(Map::READABLE);
    s.substr("one");
    s.at(100);
    s.at(Addresses::hull(0, 10));
    s.limit(5);
    s.atOrAfter(200);
    s.atOrBefore(300);
    s.within(Addresses::hull(100, 200));
    s.within(100, 200);
    s.after(100);
    s.before(100);
    s.any();
    s.none();
    s.contiguous(false);

    // Test that constraints can be augmented
    s.any().require(Map::READABLE);
    s.any().prohibit(Map::READABLE);
    s.any().substr("one");
    s.any().at(100);
    s.any().at(Addresses::hull(0, 10));
    s.any().limit(5);
    s.any().atOrAfter(200);
    s.any().atOrBefore(300);
    s.any().within(Addresses::hull(100, 200));
    s.any().within(100, 200);
    s.any().after(100);
    s.any().before(100);
    s.any().any();
    s.any().none();
    s.any().contiguous(false);

    // Operations for const or non-const maps
    s.nSegments();

    s.segments();
    s.segments(s.any());
    s.any().segments();

    s.nodes();
    s.nodes(s.any());
    s.any().nodes();

    s.next(s.any());
    s.any().next();

    s.available(s.any());
    s.any().available();

    s.exists(s.any());
    s.any().exists();

    s.read(NULL, s.any());
    s.any().read(NULL);

    s.write(NULL, s.any());
    s.any().write(NULL);
}

// Test that all operations that can be applied to mutable maps compile
template<class Map>
void compile_test_mutable(Map &s) {
    s.prune(s.any());
    s.any().prune();

    s.keep(s.any());
    s.any().keep();

    s.changeAccess(0, 0, s.any());
    s.any().changeAccess(0, 0);
}

void compile_tests() {
    AddressMap<unsigned, char> s;
    compile_test_const(s);
    compile_test_mutable(s);
    const AddressMap<unsigned, char> &t = s;
    compile_test_const(t);
}

static void test01() {
    typedef unsigned Address;
    typedef Interval<Address> Addresses;
    typedef Buffer<Address, char>::Ptr BufferPtr;
    typedef AddressSegment<Address, char> Segment;
    typedef AddressMap<Address, char> MemoryMap;

    // Allocate a couple distinct buffers.
    BufferPtr buf1 = Sawyer::Container::AllocatingBuffer<Address, char>::instance(5);
    BufferPtr buf2 = Sawyer::Container::AllocatingBuffer<Address, char>::instance(5);

    // Map them to neighboring locations in the address space
    MemoryMap map;
    map.insert(Addresses::hull(1000, 1004), Segment(buf2));
    map.insert(Addresses::hull(1005, 1009), Segment(buf1));

    // Write something across the two buffers using mapped I/O
    static const char *data1 = "abcdefghij";
    Addresses accessed = map.at(Addresses::hull(1000, 1009)).write(data1);
    ASSERT_always_require(accessed.size()==10);

    // Read back the data
    char data2[10];
    memset(data2, 0, sizeof data2);
    accessed = map.at(1000).limit(10).read(data2);
    ASSERT_always_require(accessed.size()==10);
    ASSERT_always_require(0==memcmp(data1, data2, 10));

    // See what's in the individual buffers
    memset(data2, 0, sizeof data2);
    Address nread = buf1->read(data2, 0, 5);
    ASSERT_always_require(nread==5);
    ASSERT_always_require(0==memcmp(data2, "fghij", 5));

    memset(data2, 0, sizeof data2);
    nread = buf2->read(data2, 0, 5);
    ASSERT_always_require(nread==5);
    ASSERT_always_require(0==memcmp(data2, "abcde", 5));
}

static void test02() {
    typedef unsigned Address;
    typedef Interval<Address> Addresses;
    typedef Buffer<Address, char>::Ptr BufferPtr;
    typedef AddressSegment<Address, char> Segment;
    typedef AddressMap<Address, char> MemoryMap;

    // Create some buffer objects
    char data1[15];
    memcpy(data1, "---------------", 15);
    BufferPtr buf1 = Sawyer::Container::StaticBuffer<Address, char>::instance(data1, 15);
    char data2[5];
    memcpy(data2, "##########", 10);
    BufferPtr buf2 = Sawyer::Container::StaticBuffer<Address, char>::instance(data2, 5); // using only first 5 bytes

    // Map data2 into the middle of data1
    MemoryMap map;
    map.insert(Addresses::baseSize(1000, 15), Segment(buf1));
    map.insert(Addresses::baseSize(1005,  5), Segment(buf2)); 

    // Write across both buffers and check that data2 occluded data1
    Addresses accessed = map.at(1001).limit(13).write("bcdefghijklmn");
    ASSERT_always_require(accessed.size()==13);
    ASSERT_always_require(0==memcmp(data1, "-bcde-----klmn-", 15));
    ASSERT_always_require(0==memcmp(data2,      "fghij#####", 10));

    // Map the middle of data1 over the top of data2 again and check that the mapping has one element. I.e., the three
    // separate parts were recombined into a single entry since they are three consecutive areas of a single buffer.
    map.insert(Addresses::baseSize(1005, 5), Segment(buf1, 5));
    ASSERT_always_require(map.nSegments()==1);

    // Write some data again
    accessed = map.at(1001).limit(13).write("BCDEFGHIJKLMN");
    ASSERT_always_require(accessed.size()==13);
    ASSERT_always_require(0==memcmp(data1, "-BCDEFGHIJKLMN-", 15));
    ASSERT_always_require(0==memcmp(data2,      "fghij#####", 10));
}

static void test03() {
    typedef unsigned Address;
    typedef Interval<Address> Addresses;
    typedef Buffer<Address, char>::Ptr BufferPtr;
    typedef AddressSegment<Address, char> Segment;
    typedef AddressMap<Address, char> MemoryMap;

    boost::iostreams::mapped_file_params mfp;
    mfp.path = "/etc/passwd";
    mfp.flags = boost::iostreams::mapped_file_base::priv;
    BufferPtr buf1 = Sawyer::Container::MappedBuffer<Address, char>::instance(mfp);

    MemoryMap map;
    map.insert(Addresses::baseSize(1000, 64), Segment(buf1));

    char data[64];
    Address nread = map.at(1000).limit(64).read(data).size();
    data[sizeof(data)-1] = '\0';
    ASSERT_always_require(nread==64);
    std::cout <<data <<"...\n";
}

int main() {
    test01();
    test02();
    test03();
}

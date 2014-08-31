#include <sawyer/AddressMap.h>

#include <sawyer/AllocatingBuffer.h>
#include <sawyer/MappedBuffer.h>
#include <sawyer/StaticBuffer.h>

#include <iostream>

using namespace Sawyer::Container;

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

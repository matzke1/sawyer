#include <sawyer/Message.h>

#include <boost/thread.hpp>

static const size_t MAX_THREADS = 10;

using namespace Sawyer::Message::Common;
Facility mlog;

template<class Functor>
void
runTests(Functor &f) {
    boost::thread threads[MAX_THREADS];
    for (size_t nthreads=1; nthreads<=MAX_THREADS; ++nthreads) {
        std::cout <<"  nthreads=" <<nthreads <<"\n";
        for (size_t i=0; i<nthreads; ++i)
            threads[i] = boost::thread(f);
        for (size_t i=0; i<nthreads; ++i)
            threads[i].join();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test that threads can create message streams.

struct StreamCreationTester {
    void operator()() {
        for (size_t i=0; i<1000; ++i)
            Stream info = mlog[INFO];
    }
};

static void
testStreamCreation() {
    std::cout <<"testing multi-threaded stream creation\n";
    StreamCreationTester tester;
    runTests(tester);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test that multiple threads can write to the same stream.  The output will likely be interleaved within a line (one line of
// output might contain parts of messages from multiple threads), but each line will still have its own prefix area and the
// prefix areas are not mixed up.

struct StreamOutputTester {
    void operator()() {
        for (size_t i=0; i<1000; ++i) {
            mlog[INFO] <<"test long line of output ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz\n";
        }
    }
};

static void
testStreamOutput() {
    std::cout <<"testing multi-threaded stream output\n";
    StreamOutputTester tester;
    runTests(tester);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test that threads can write to their own streams even when those streams share the same piping. Each thread will have its
// own copy of mlog[INFO], therefore each line of output will contain text from only one stream.  However, during contention
// on the stream, two or more streams might interfere with each other in such a way that one or more of them outputs successive
// partial messages.

struct StreamOutputTester2 {
    Stream stream;
    StreamOutputTester2(const Stream &stream): stream(stream) {}

    void operator()() {
        for (size_t i=0; i<1000; ++i) {
            stream <<"test long line of output ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz\n";
        }
    }
};

static void
testStreamOutput2() {
    std::cout <<"testing multi-threaded stream output to copied streams\n";
    StreamOutputTester2 tester(mlog[INFO]);
    runTests(tester);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int
main() {
    mlog = Facility("test");
    testStreamCreation();
    //testStreamOutput();
    testStreamOutput2();
}


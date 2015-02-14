#include <sawyer/Message.h>

#include <boost/thread.hpp>

static const size_t MAX_THREADS = 10;
static const size_t NMESSAGES = 1000;
static const size_t MIN_MESSAGES_PER_THREAD = 10;

using namespace Sawyer::Message::Common;
Facility mlog;

template<class Functor>
void
runTests(Functor f) {
    boost::thread threads[MAX_THREADS];
    for (size_t nthreads=1; nthreads<=MAX_THREADS; ++nthreads) {
        f.nthreads = nthreads;
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
    size_t nthreads;
    void operator()() {
        const size_t n = std::max(NMESSAGES/nthreads, MIN_MESSAGES_PER_THREAD);
        for (size_t i=0; i<n; ++i)
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

struct StreamOutputTester1 {
    size_t nthreads;
    void operator()() {
        const size_t n = std::max(NMESSAGES/nthreads, MIN_MESSAGES_PER_THREAD);
        for (size_t i=0; i<n; ++i)
            mlog[INFO] <<"StreamOutputTester1: long line of output ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz\n";
    }
};

static void
testStreamOutput1() {
    std::cout <<"testing multi-threaded stream output, no copy\n";
    StreamOutputTester1 tester;
    runTests(tester);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test that threads can write to their own streams even when those streams share the same piping. Each thread will have its
// own copy of mlog[INFO], therefore each line of output will contain text from only one stream.  However, during contention
// on the stream, two or more streams might interfere with each other in such a way that one or more of them outputs successive
// partial messages.

struct StreamOutputTester2 {
    size_t nthreads;
    Stream stream;
    StreamOutputTester2(const Stream &stream): stream(stream) {}

    void operator()() {
        const size_t n = std::max(NMESSAGES/nthreads, MIN_MESSAGES_PER_THREAD);
        for (size_t i=0; i<n; ++i)
            stream <<"StreamOutputTester2: long line of output ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz\n";
    }
};

static void
testStreamOutput2() {
    std::cout <<"testing multi-threaded stream output to copied streams\n";
    Stream myStream = mlog[INFO];
    myStream.destination(Sawyer::Message::StreamSink::instance(std::cerr)->partialMessagesAllowed(false));

    StreamOutputTester2 tester(myStream);
    runTests(tester);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int
main() {
    mlog = Facility("test");
    
    testStreamCreation();
    testStreamOutput1();
    testStreamOutput2();
}


#include <iostream>
#include <Sawyer/Message.h>

#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

#include <boost/random/uniform_smallint.hpp>
#if BOOST_VERSION >= 104700
    #include <boost/random/mersenne_twister.hpp>
    typedef boost::random::mt11213b Prng;
    typedef boost::random::uniform_smallint<unsigned> Distributor;
#else
    #include <boost/random/linear_congruential.hpp>
    typedef boost::rand48 Prng;
    typedef boost::uniform_smallint<unsigned> Distributor;
#endif


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
        std::cerr <<"  nthreads=" <<nthreads <<"\n";
        for (size_t i=0; i<nthreads; ++i)
            threads[i] = boost::thread(f, i);
        for (size_t i=0; i<nthreads; ++i)
            threads[i].join();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test that threads can create message streams.

struct StreamCreationTester {
    size_t nthreads;
    void operator()(size_t) {
        const size_t n = std::max(NMESSAGES/nthreads, MIN_MESSAGES_PER_THREAD);
        for (size_t i=0; i<n; ++i)
            Stream info = mlog[INFO];
    }
};

static void
testStreamCreation() {
    std::cerr <<"testing multi-threaded stream creation\n";
    StreamCreationTester tester;
    runTests(tester);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test that multiple threads can write to the same stream.  The output will likely be interleaved within a line (one line of
// output might contain parts of messages from multiple threads), but each line will still have its own prefix area and the
// prefix areas are not mixed up.

struct StreamOutputTester1 {
    size_t nthreads;
    void operator()(size_t) {
        const size_t n = std::max(NMESSAGES/nthreads, MIN_MESSAGES_PER_THREAD);
        for (size_t i=0; i<n; ++i)
            mlog[INFO] <<"StreamOutputTester1: long line of output ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz\n";
    }
};

static void
testStreamOutput1() {
    std::cerr <<"testing multi-threaded stream output, no copy\n";
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

    void operator()(size_t) {
        const size_t n = std::max(NMESSAGES/nthreads, MIN_MESSAGES_PER_THREAD);
        for (size_t i=0; i<n; ++i)
            stream <<"StreamOutputTester2: long line of output ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz\n";
    }
};

static void
testStreamOutput2() {
    std::cerr <<"testing multi-threaded stream output to copied streams\n";
    Stream myStream = mlog[INFO];
    myStream.destination(Sawyer::Message::StreamSink::instance(std::cerr)->partialMessagesAllowed(false));

    StreamOutputTester2 tester(myStream);
    runTests(tester);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test that each thread's output doesn't interfere with other thread output even when they're writing to the exact same stream
// with output operators that take a long time to execute.
//
// Each line of output will have up to six words like "x.y.z" where "x" is the thread number, "y" is the message number within
// that thread, and "z" is the word number within the message. Therefore, since the streams are segregated by stream, each line
// will contain words where all the "x" and "y" values are the same, and teh "z" are increasing.
//
// If message buffering is disabled (the default) then most of the lines will be partial messages ending with "..." since the
// threads are continually interrupting each other's output.  If buffering is enabled, then each line will be a complete
// message (no "...") since each thread buffers the message until the "\n" is inserted.
struct StreamSlowTester {
    size_t nthreads;
    Stream &stream;                                     // all threads share the same stream

    StreamSlowTester(Stream &stream): stream(stream) {}

    std::string word(Prng &generator, size_t i, size_t j, size_t k) {
        Distributor distributor(0, 40);
        int millies = distributor(generator);

        boost::this_thread::sleep_for(boost::chrono::milliseconds(millies));
        return (" " + boost::lexical_cast<std::string>(i) +
                "." + boost::lexical_cast<std::string>(j) +
                "." + boost::lexical_cast<std::string>(k));
    }
    
    void operator()(size_t threadNumber) {
        Prng prng(threadNumber);

        for (size_t i = 0; i < 10; ++i) {
            stream <<word(prng, threadNumber, i, 1);
            stream <<word(prng, threadNumber, i, 2);
            stream <<word(prng, threadNumber, i, 3);
            stream <<word(prng, threadNumber, i, 4);
            stream <<word(prng, threadNumber, i, 5);
            stream <<word(prng, threadNumber, i, 6);
            stream <<"\n";
        }
    }
};

static void
testStreamSlowOutput(bool buffered) {
    std::cerr <<"testing slow output from threads all using the same stream\n";

    Stream myStream = mlog[INFO];
    myStream.destination(Sawyer::Message::StreamSink::instance(std::cerr)->partialMessagesAllowed(!buffered));
    StreamSlowTester tester(myStream);
    runTests(tester);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int
main() {
    mlog = Facility("test");
    
    testStreamCreation();
    testStreamOutput1();
    testStreamOutput2();
    testStreamSlowOutput(false /*unbuffered*/);
    testStreamSlowOutput(true /*buffered*/);
}


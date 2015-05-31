// Tests some message filters
#include <Sawyer/Message.h>
#include <iostream>

using namespace Sawyer::Message;

void test()
{
    DestinationPtr sink = FdSink::instance(2);          // standard error

    // Create some streams with sequential limiters. We use different importances only for the sake of their colors.
    Stream s0("allvals", ERROR, sink);
    Stream s1("first_1", DEBUG, SequenceFilter::instance(0, 0, 1)->to(sink));
    Stream s2("first_5", WHERE, SequenceFilter::instance(0, 0, 5)->to(sink));
    Stream s3("every20", INFO,  SequenceFilter::instance(0, 20, 0)->to(sink));
    static Stream s4("onetime", WARN, SequenceFilter::instance(0, 0, 1)->to(sink));

    for (size_t i=0; i<=100; ++i) {
        s0 <<"#";                                       // a poor-mans progress bar! Note how it gets reprinted as necessary.

        s1 <<"this (i=" <<i <<") is the";
        s1 <<" first iteration\n";

        s2 <<"this (i=" <<i <<") is one of";
        s2 <<" the first five iterations\n";

        s3 <<"this (i=" <<i <<") is";
        s3 <<" a multiple of 20\n";

        s4 <<"should be printed only once (ever) due to static\n";
    }
    s0 <<"\n";
}

int main()
{
    test();
    std::cerr <<"\n";
    test();
    return 0;
}

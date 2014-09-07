#include <sawyer/Message.h>
#include <sawyer/Optional.h>

#include <boost/config.hpp>
#include <cstdio>
#include <iostream>
#ifndef BOOST_WINDOWS
#include <syslog.h>
#endif

using namespace Sawyer::Message;

static void banner(const std::string &title) {
    std::cerr <<"\n=========== " <<title <<"\n";
}

// Some very basic stuff
void test1(const DestinationPtr &sink)
{
    banner("test1 - basic");
    Facility log("test1", sink);
    log[DEBUG] <<"debugging message\n";
    log[TRACE] <<"tracing message\n";
    log[WHERE] <<"major execution point\n";
    log[INFO]  <<"informational message\n";
    log[WARN]  <<"warning message\n";
    log[ERROR] <<"error message\n";
    log[FATAL] <<"fatal error message\n";
}

// Can streams be disabled?
void test2(const DestinationPtr &sink)
{
    banner("test2 - disable");
    Facility log("test2", sink);
    log[DEBUG] <<"message 1\n";
    log[DEBUG].disable();
    log[DEBUG] <<"SHOULD NOT APPEAR\n";
    log[DEBUG].enable();
    log[DEBUG] <<"message 2\n";
}

// Does Boolean context work?
void test3(const DestinationPtr &sink)
{
    banner("test3 - boolean");
    Facility log("test3", sink);
    log[INFO] <<"no other messages should be emitted for this test\n";
    if (!log[DEBUG])
        log[ERROR] <<"log[DEBUG] should be enabled now\n";
    log[DEBUG].disable();
    if (log[DEBUG])
        log[ERROR] <<"log[DEBUG] should be disabled now\n";
    
}

// Does short circuit work?
void test4(const DestinationPtr &sink)
{
    banner("test4 - short circuit");
    Facility log("test4", sink);
    int i=0;
    SAWYER_MESG(log[DEBUG]) <<"i=" <<++i <<"\n";
    SAWYER_MESG(log[DEBUG]) <<"i=" <<++i <<"\n";
    log[DEBUG] && log[DEBUG] <<"i=" <<++i <<"\n";
    log[DEBUG] && log[DEBUG] <<"i=" <<++i <<"\n";

    log[DEBUG].disable();
    SAWYER_MESG(log[DEBUG]) <<"i=" <<++i <<"\n";
    SAWYER_MESG(log[DEBUG]) <<"i=" <<++i <<"\n";
    log[DEBUG] && log[DEBUG] <<"i=" <<++i <<"\n";
    log[DEBUG] && log[DEBUG] <<"i=" <<++i <<"\n";

    log[DEBUG].enable();
    log[DEBUG] <<"final value for i=" <<i <<" (should be 4)\n";
}

// Simple unbuffered partial messages
void test5(const DestinationPtr &sink)
{
    banner("test5 - single partial");
    Facility log("test5", sink);
    log[DEBUG] <<"five characters, one per second (except on Windows) [";
    for (size_t i=0; i<5; ++i) {
#ifndef BOOST_WINDOWS                                        // FIXME[Robb Matzke 2014-06-10]: how on windows?
        sleep(1);
#endif
        log[DEBUG] <<"#";
    }
    log[DEBUG] <<"]\n";
}

// partial message completion
void test6(const DestinationPtr &sink)
{
    banner("test6 - partial completion");
    {
        Facility log("test6", sink);
        log[DEBUG] <<"partial";
    }
    std::cerr <<"[This text should be on its own line]\n";
}

// interferring partial messages
void test7(const DestinationPtr &sink)
{
    banner("test7 - interference");
    Facility log("test7", sink);
    log[DEBUG] <<"d1";
    log[TRACE] <<"t1";
    log[WHERE] <<"w1";
    
    log[DEBUG] <<" d2";
    log[TRACE] <<" t2";
    log[WHERE] <<" w2";

    log[DEBUG] <<" d_end\n";
    log[TRACE] <<" t_end\n";
    log[WHERE] <<" w_end\n";
}

// handle for partial messages
void test8a(const DestinationPtr &sink) {
    banner("test8a - partial handle");
    Facility log("test8", sink);
    SProxy m1 = *log[DEBUG].dup() <<"1.1";
    log[DEBUG] <<"2.1";
    log[DEBUG] <<" 2.2\n";
    *m1 <<" 1.2\n";
}

// handle for partial messages
void test8b(const DestinationPtr &sink) {
    banner("test8b - partial handle");
    Facility log("test8b", sink);

    Stream m1 = log[DEBUG] <<"1.1";
    log[DEBUG] <<"2.1";
    log[DEBUG] <<" 2.2\n";
    m1 <<" 1.2\n";
}

// handle for partial messages
void test8c(const DestinationPtr &sink) {
    banner("test8c - partial handle");
    Facility log("test8c", sink);

    Stream m1(log[DEBUG] <<"1.1");
    log[DEBUG] <<"2.1";
    log[DEBUG] <<" 2.2\n";
    m1 <<" 1.2\n";
}

// partial messages with enable/disable
void test9(const DestinationPtr &sink)
{
    banner("test9 - partial enable");
    Facility log("test9", sink);
    log[DEBUG].disable();
    log[DEBUG] <<"SHOULD NOT";
    log[DEBUG] <<" BE PRINTED\n";

    // Parts inserted while disable are buffered until enabled, so the vertical
    // bars should appear in pairs
    log[DEBUG] <<"part1";
    std::cerr <<"[This text should be first]\n";
    log[DEBUG].enable();
    log[DEBUG] <<" part2";
    std::cerr <<" |";
    log[DEBUG].disable();
    log[DEBUG] <<" part3";
    std::cerr <<"|";
    log[DEBUG].enable();
    log[DEBUG] <<" part4";
    std::cerr <<" |";
    log[DEBUG].disable();
    log[DEBUG] <<" part5";
    std::cerr <<"|";
    log[DEBUG].enable();
    log[DEBUG] <<" done\n";
}

//// indentation
//void test10()
//{
//    banner("test10 - indentation");
//    MessageFacility log("test10");
//
//    log[DEBUG] <<"level 0\n";
//    {
//        MessageIndenter indent1(log[DEBUG]);
//        log[DEBUG] <<"level 1\n";
//        log[INFO]  <<"level 1\n";
//        {
//            MessageIndenter indent2(log[DEBUG]);
//            log[DEBUG] <<"level 2\n";
//            MessageFacility alternate("test_X");
//            alternate[INFO] <<"level 2\n";
//        }
//        log[DEBUG] <<"level 1\n";
//    }
//    log[DEBUG] <<"level 0\n";
//}
//
//// indentation canceling
//void test11()
//{
//    banner("test11 - indent cancel");
//    MessageFacility log("test11");
//    log[DEBUG] <<"level 1\n";
//    {
//        MessageIndenter indent(log[DEBUG]);
//        log[DEBUG] <<"level 2\n";
//        indent.cancel() <<"level 1\n";
//        log[DEBUG] <<"level 1\n";
//    }
//    log[DEBUG] <<"level 1\n";
//}

// A stupid little sink that surrounds each message with "--->" and "<---"
class MySink: public StreamSink {
protected:
    explicit MySink(std::ostream &o): StreamSink(o) {}
public:
    typedef Sawyer::SharedPointer<MySink> Ptr;
    static Ptr instance(std::ostream &o) {
        return Ptr(new MySink(o));
    }
    virtual std::string maybePrefix(const Mesg &mesg, const MesgProps &props) /*override*/ {
        std::string retval = UnformattedSink::maybePrefix(mesg, props);
        if (!retval.empty())
            retval += " --->";
        return retval;
    }
    virtual std::string maybeBody(const Mesg &mesg, const MesgProps &props) /*override*/ {
        std::string retval = UnformattedSink::maybeBody(mesg, props);
        if (mesg.isComplete())
            retval += "<---";
        return retval;
    }
};

// alternative sink
void test12(const DestinationPtr &sink) 
{
    banner("test12 - user-defined sink");
    Facility log("test12", MySink::instance(std::cerr));
    log[INFO] <<"message 1\nmessage 2\n";
}

//// temporary enable
//void test13()
//{
//    banner("test13 - temporary enable");
//    MessageFacility log("test13");
//
//    log[DEBUG] <<"message 1\n";
//    {
//        TemporaryEnable te(log[DEBUG], false); // disable
//        log[DEBUG] <<"THIS MESSAGE SHOULD NOT APPEAR (1)\n";
//    }
//    log[DEBUG] <<"message 2\n";
//
//    log[DEBUG].disable();
//    {
//        log[DEBUG] <<"THIS MESSAGE SHOULD NOT APPEAR (2)\n";
//        TemporaryEnable te(log[DEBUG]);
//        log[DEBUG] <<"message 3\n";
//        te.cancel();
//        log[DEBUG] <<"THIS MESSAGE SHOULD NOT APPEAR (3)\n";
//        log[DEBUG].enable();
//        log[DEBUG] <<"message 4\n";
//    }
//    log[DEBUG] <<"message 5\n"; // because of the cancel followed by enable
//}

// Test that two distinct destinations to the same place coordinate with each other.
// Note that this doesn't work with C++ ostreams because there's no portable way to get file descriptors from them.
void test14(const DestinationPtr &sink1) {
    banner("test14 - coordinating destinations");
    DestinationPtr sink2 = FileSink::instance(stderr);
    Stream merr = Stream("sink1", INFO, sink1);
    merr.interruptionString("...{sink1}");
    Stream mout = Stream("sink2", INFO, sink2);
    mout.interruptionString("...{sink2}");

    merr <<"1";
    mout <<"a";
    merr <<"2";
    mout <<"b";
    merr <<"3";
    mout <<"c";
    merr <<"\n";
    mout <<"\n";
}

// Test that rate limiting works
void test15(const DestinationPtr &sink1) {
    banner("test15 - rate limiting");
    FilterPtr limited = TimeFilter::instance(0.5);
    limited->addDestination(sink1);
    Stream merr = Stream("test15", INFO, limited);

    for (size_t i=0; i<5000000; ++i) {
        merr <<"i=" <<i <<"\n";
    }
}

// Demonstrate rate limiting per importance level
void test15b(const DestinationPtr &sink1) {
    banner("test15b - per importance rate limiting");
    Facility log("test15b", sink1);

    for (size_t i=DEBUG; i<=FATAL; ++i) {
        Importance imp = (Importance)i;
        FilterPtr limiter = TimeFilter::instance(1.0);
        limiter->addDestination(log[imp].destination());
        log[imp].destination(limiter);
    }

    for (size_t i=0; i<300000; ++i) {
        log[DEBUG] <<"i=" <<i <<"\tdebugging message\n";
        log[TRACE] <<"i=" <<i <<"\ttracing message\n";
        log[WHERE] <<"i=" <<i <<"\tmajor execution point\n";
        log[INFO]  <<"i=" <<i <<"\tinformational message\n";
        log[WARN]  <<"i=" <<i <<"\twarning message\n";
        log[ERROR] <<"i=" <<i <<"\terror message\n";
        log[FATAL] <<"i=" <<i <<"\tfatal error message\n";
    }
}

// Test that sequence limiting works
void test16(const DestinationPtr &sink1) {
    banner("test16 - sequence limiting");
    FilterPtr limited = SequenceFilter::instance(10, 2, 5); // every other message beginning with #10, five total
    limited->addDestination(sink1);
    Stream merr = Stream("test16", INFO, limited);

    for (size_t i=0; i<100; ++i) {
        merr <<"i=" <<i <<"\n";
    }
}

// Test message colors
void test17(const DestinationPtr &sink) {
    banner("test17 - colors");
    Facility log("test17", sink);

    UnformattedSinkPtr unf = sink.dynamicCast<UnformattedSink>();
    if (unf==NULL) {
        std::cerr <<"skipped: colors can only be tested with sinks that support them\n";
        return;
    }
    PrefixPtr prefix = unf->prefix();

    std::cerr <<"full color:\n";
    prefix->colorSet() = ColorSet::fullColor();
    log[DEBUG] <<"debugging message\n";
    log[TRACE] <<"tracing message\n";
    log[WHERE] <<"major execution point\n";
    log[INFO]  <<"informational message\n";
    log[WARN]  <<"warning message\n";
    log[ERROR] <<"error message\n";
    log[FATAL] <<"fatal error message\n";

    std::cerr <<"black and white:\n";
    prefix->colorSet() = ColorSet::blackAndWhite();
    log[DEBUG] <<"debugging message\n";
    log[TRACE] <<"tracing message\n";
    log[WHERE] <<"major execution point\n";
    log[INFO]  <<"informational message\n";
    log[WARN]  <<"warning message\n";
    log[ERROR] <<"error message\n";
    log[FATAL] <<"fatal error message\n";

    std::cerr <<"no color:\n";
    prefix->colorSet() = ColorSet();
    log[DEBUG] <<"debugging message\n";
    log[TRACE] <<"tracing message\n";
    log[WHERE] <<"major execution point\n";
    log[INFO]  <<"informational message\n";
    log[WARN]  <<"warning message\n";
    log[ERROR] <<"error message\n";
    log[FATAL] <<"fatal error message\n";

    std::cerr <<"custom colors:\n";
    prefix->colorSet() = ColorSet();
    for (size_t fg=COLOR_BLACK; fg<=COLOR_DEFAULT; ++fg) {
        for (size_t bg=COLOR_BLACK; bg<=COLOR_DEFAULT; ++bg) {
            for (int isBold=0; isBold<=1; ++isBold) {
                prefix->colorSet()[DEBUG] = ColorSpec((AnsiColor)fg, (AnsiColor)bg, isBold!=0);
                log[DEBUG] <<stringifyColor((AnsiColor)fg) <<" on " <<stringifyColor((AnsiColor)bg)
                           <<(isBold ? " bold" : " regular") <<"\n";
            }
        }
    }

    std::cerr <<"default colors:\n";
    unf->prefix(Prefix::instance());
    log[DEBUG] <<"debugging message\n";
    log[TRACE] <<"tracing message\n";
    log[WHERE] <<"major execution point\n";
    log[INFO]  <<"informational message\n";
    log[WARN]  <<"warning message\n";
    log[ERROR] <<"error message\n";
    log[FATAL] <<"fatal error message\n";
}

class MyPrefix: public Prefix {
protected:
    MyPrefix() {}
public:
    typedef Sawyer::SharedPointer<MyPrefix> Ptr;
    static Ptr instance() { return Ptr(new MyPrefix); }
    virtual std::string toString(const Mesg&, const MesgProps &props) const /*override*/ {
        return "MY PREFIX {" + stringifyImportance(props.importance.orElse(INFO)) + "} ";
    }
};

// Test custom prefix
void test18(const DestinationPtr &sink) {
    banner("test18 - custom prefix");
    Facility log("test18", sink);
    
    UnformattedSinkPtr unf = sink.dynamicCast<UnformattedSink>();
    if (unf==NULL) {
        std::cerr <<"skipped: colors can only be tested with sinks that support them\n";
        return;
    }

    // An easy way to turn off the program name, process ID, and time delta, which some people find annoying.
    std::cerr <<"turn off some prefix features:\n";
    unf->prefix()->showProgramName(false);
    unf->prefix()->showThreadId(false);
    unf->prefix()->showElapsedTime(false);
    log[DEBUG] <<"debugging message\n";
    log[TRACE] <<"tracing message\n";
    log[WHERE] <<"major execution point\n";
    log[INFO]  <<"informational message\n";
    log[WARN]  <<"warning message\n";
    log[ERROR] <<"error message\n";
    log[FATAL] <<"fatal error message\n";
    
    // Prefix that starts each message with "MY PREFIX {<importance>} "
    std::cerr <<"use our own prefix generator:\n";
    unf->prefix(MyPrefix::instance());
    log[DEBUG] <<"debugging message\n";
    log[TRACE] <<"tracing message\n";
    log[WHERE] <<"major execution point\n";
    log[INFO]  <<"informational message\n";
    log[WARN]  <<"warning message\n";
    log[ERROR] <<"error message\n";
    log[FATAL] <<"fatal error message\n";

    // Now go back to the default
    std::cerr <<"now back to the default:\n";
    unf->prefix(Prefix::instance());
    log[DEBUG] <<"debugging message\n";
    log[TRACE] <<"tracing message\n";
    log[WHERE] <<"major execution point\n";
    log[INFO]  <<"informational message\n";
    log[WARN]  <<"warning message\n";
    log[ERROR] <<"error message\n";
    log[FATAL] <<"fatal error message\n";
}

void test19(const DestinationPtr &sink) {
    banner("test19 - two destinations");
    DestinationPtr two = Multiplexer::instance()->to(sink, sink);
    Facility log("test19", two);
    log[INFO] <<"one message, two destinations\n";
}

// Printing an empty string creates no message.
void test20(const DestinationPtr &sink) {
    banner("test20 - empty message");
    Facility log("test20", sink);
    log[INFO] <<"\n";
    log[INFO] <<std::endl;
    log[INFO] <<"no messages before this one\n";
}

// Make sure std::endl works.
void test21(const DestinationPtr &sink) {
    banner("test21 - std::endl");
    Facility log("test21", sink);
    log[INFO] <<"message one" <<std::endl;
    log[INFO] <<"message two" <<std::endl;
    log[INFO] <<"message three\n";
}

// Messages consisting of only white space are treated as if they were empty
void test22(const DestinationPtr &sink) {
    banner("test22 - white space only");
    Facility log("test22", sink);
    log[INFO] <<" " <<"\n";
    log[INFO] <<"\t \r\f\n";
    log[INFO] <<"no messages before this one\n";
}

int main()
{

#if 0 /**/
    DestinationPtr merr = StreamSink::instance(std::cerr);
#elif 1
    DestinationPtr merr = FileSink::instance(stderr);
#elif 0
    DestinationPtr merr = FdSink::instance(2);
#else
    DestinationPtr merr = SyslogSink::instance("sawyer", LOG_PID, LOG_DAEMON);
#endif

#if 0 /* [Robb Matzke 2014-01-20] */
    test8c(merr);
    exit(0);
#endif

    test1(merr);
    test2(merr);
    test3(merr);
    test4(merr);
    test5(merr);
    test6(merr);
    test7(merr);
    test8a(merr);
    test8b(merr);
    test8c(merr);
    test9(merr);
//    test10();
//    test11();
    test12(merr);
//    test13();
    test14(merr);
    test15(merr);
    test15b(merr);
    test16(merr);
    test17(merr);
    test18(merr);
    test19(merr);
    test20(merr);
    test21(merr);
    test22(merr);
    return 0;
}

#include "Sawyer.h"
#include <iostream>
#include <string>

using namespace Sawyer;

// Makes a benner to separate the tests from each other
void banner(const std::string &name)
{
    static struct Prefix: MessagePrefix {
        std::string operator()(const MessageProps&, const std::string&, MessageImportance) /*override*/ {
            return "";
        }
    } prefix;

    static MessageStream m("", WHERE, prefix);

    size_t width = 70;
    if (name.size()+2 >= width) {
        m <<name <<"\n";
    } else {
        size_t ltsz = (width-name.size())/2;
        size_t rtsz = width - ltsz - name.size();
        m <<std::string(ltsz, '=') <<" " <<name <<" " <<std::string(rtsz, '=') <<"\n";
    }
}

// Some very basic stuff
void test1()
{
    // banner("test1 - basic"); --wait until we get some basic testing under our belt
    MessageFacility log("test1");
    log[DEBUG] <<"debugging message\n";
    log[TRACE] <<"tracing message\n";
    log[WHERE] <<"major execution point\n";
    log[INFO]  <<"informational message\n";
    log[WARN]  <<"warning message\n";
    log[ERROR] <<"error message\n";
    log[FATAL] <<"fatal error message\n";
}

// Can streams be disabled?
void test2()
{
    banner("test2 - disable");
    MessageFacility log("test2");
    log[DEBUG] <<"message 1\n";
    log[DEBUG].disable();
    log[DEBUG] <<"SHOULD NOT APPEAR\n";
    log[DEBUG].enable();
    log[DEBUG] <<"message 2\n";
}

// Does Boolean context work?
void test3()
{
    banner("test3 - boolean");
    MessageFacility log("test3");
    if (!log[DEBUG])
        log[ERROR] <<"log[DEBUG] should be enabled now\n";
    log[DEBUG].disable();
    if (log[DEBUG])
        log[ERROR] <<"log[DEBUG] should be disabled now\n";
}

// Does short circuit work?
void test4()
{
    banner("test4 - short circuit");
    MessageFacility log("test4");
    int i=0;
    SAWYER(log[DEBUG]) <<"i=" <<++i <<"\n";
    SAWYER(log[DEBUG]) <<"i=" <<++i <<"\n";
    log[DEBUG] and log[DEBUG] <<"i=" <<++i <<"\n";
    log[DEBUG] and log[DEBUG] <<"i=" <<++i <<"\n";

    log[DEBUG].disable();
    SAWYER(log[DEBUG]) <<"i=" <<++i <<"\n";
    SAWYER(log[DEBUG]) <<"i=" <<++i <<"\n";
    log[DEBUG] and log[DEBUG] <<"i=" <<++i <<"\n";
    log[DEBUG] and log[DEBUG] <<"i=" <<++i <<"\n";

    log[DEBUG].enable();
    log[DEBUG] <<"final value for i=" <<i <<"\n";
}

// Simple unbuffered partial messages
void test5()
{
    banner("test5 - single partial");
    MessageFacility log("test5");
    log[DEBUG] <<"five characters, one per second [";
    for (size_t i=0; i<5; ++i) {
        sleep(1);
        log[DEBUG] <<"#";
    }
    log[DEBUG] <<"]\n";
}

// partial message completion
void test6()
{
    banner("test6 - partial completion");
    {
        MessageFacility log("test6");
        log[DEBUG] <<"partial";
    }
    std::cerr <<"[This text should be on its own line]\n";
}

// interferring partial messages
void test7()
{
    banner("test7 - interference");
    MessageFacility log("test7");
    log[DEBUG] <<"d1";
    log[TRACE] <<"t1" <<pterm(" (interrupted)\n");
    log[WHERE] <<"w1";
    
    log[DEBUG] <<" d2";
    log[TRACE] <<" t2" <<pterm("...\n");
    log[WHERE] <<" w2";

    log[DEBUG] <<" d_end\n";
    log[TRACE] <<" t_end\n";
    log[WHERE] <<" w_end\n";
}

// handle for partial messages
void test8()
{
    banner("test8 - partial handle");
    MessageFacility log("test8");
    MessageStream m1(log[DEBUG] <<"part1");
    log[DEBUG] <<"part2";
    log[DEBUG] <<" done2\n";
    m1 <<" done1\n";
}

// partial messages with enable/disable
void test9()
{
    banner("test9 - partial enable");
    MessageFacility log("test9");
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

// indentation
void test10()
{
    banner("test10 - indentation");
    MessageFacility log("test10");

    log[DEBUG] <<"level 0\n";
    {
        MessageIndenter indent1(log[DEBUG]);
        log[DEBUG] <<"level 1\n";
        log[INFO]  <<"level 1\n";
        {
            MessageIndenter indent2(log[DEBUG]);
            log[DEBUG] <<"level 2\n";
            MessageFacility alternate("test_X");
            alternate[INFO] <<"level 2\n";
        }
        log[DEBUG] <<"level 1\n";
    }
    log[DEBUG] <<"level 0\n";
}

// indentation canceling
void test11()
{
    banner("test11 - indent cancel");
    MessageFacility log("test11");
    log[DEBUG] <<"level 1\n";
    {
        MessageIndenter indent(log[DEBUG]);
        log[DEBUG] <<"level 2\n";
        indent.cancel() <<"level 1\n";
        log[DEBUG] <<"level 1\n";
    }
    log[DEBUG] <<"level 1\n";
}

// alternative sink
void test12() 
{
    banner("test12 - user-defined sink");

    struct Prefix: MessagePrefix {
        std::string operator()(const std::string&, MessageImportance) {
            return "";
        }
    } prefix;

    struct Sink: MessageFileSink {
        Sink(): MessageFileSink(std::cerr) {}
        void output(const MessageProps &mesg, const std::string &full_line, size_t new_stuff) {
            MessageFileSink::output(mesg, full_line, new_stuff);
            MessageFileSink::output(mesg, full_line, new_stuff);
        }
    } sink;

    MessageFacility log("test12", prefix, sink);
    log[DEBUG] <<"This message should be emitted twice\n";
}

// temporary enable
void test13()
{
    banner("test13 - temporary enable");
    MessageFacility log("test13");

    log[DEBUG] <<"message 1\n";
    {
        TemporaryEnable te(log[DEBUG], false); // disable
        log[DEBUG] <<"THIS MESSAGE SHOULD NOT APPEAR (1)\n";
    }
    log[DEBUG] <<"message 2\n";

    log[DEBUG].disable();
    {
        log[DEBUG] <<"THIS MESSAGE SHOULD NOT APPEAR (2)\n";
        TemporaryEnable te(log[DEBUG]);
        log[DEBUG] <<"message 3\n";
        te.cancel();
        log[DEBUG] <<"THIS MESSAGE SHOULD NOT APPEAR (3)\n";
        log[DEBUG].enable();
        log[DEBUG] <<"message 4\n";
    }
    log[DEBUG] <<"message 5\n"; // because of the cancel followed by enable
}

int main()
{
    merr.use_color(isatty(2)); // turn on color if this is a terminal
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
    test11();
    test12();
    test13();
    return 0;
}

#if 0

// Create the debugging facilities for this software layer.  If you use the name "log", which can be a verb, then the logging
// statements in the source code will look somewhat like commands.  The name of the facility will be printed (by default) as
// part of the output, so you might as well use the same name as the program/namespace/class/function/whatever that's using
// this Facility for its output.
Sawyer::Facility log("Example1", prefixer, numbering_sink);


// Demonstrate basic usage.  The output might be something like this:
// +----------------------------------------------------------------------------------
// |Example1[DEBUG]: called basic1(x=3)
// |Example1[WARN ]: basic1(): not fully implemented yet
// +----------------------------------------------------------------------------------
int basic1(int x) 
{
    log[DEBUG] <<"called basic1(x=" <<x <<")\n";
    log[WARN] <<"basic1(): not fully implemented yet\n";
    return x+x;
}

// Demonstrates how to turn on debugging just within a single function regardless of whether it was turned on by the command
// line.  As soon as 'te' is destroyed the setting reverts to its original value.  This works when throwing exceptions also. A
// good practice is to indicate why this is here since we're overriding user preferences (it's also a good marker to remind you
// to remove it before committing).
// +----------------------------------------------------------------------------------
// |Example1[DEBUG]: temporarily enabled
// +----------------------------------------------------------------------------------
void basic2()
{
    log[DEBUG].disable();
    log[DEBUG] <<"SHOULD NOT BE PRINTED\n";

    {
        TemporaryEnable te(log[DEBUG]);
        log[DEBUG] <<"temporarily enabled\n";
    }
    
    log[DEBUG] <<"SHOULD NOT BE PRINTED\n";
    log[DEBUG].enable();
}

// Demonstrate implicit conversion to bool
// +----------------------------------------------------------------------------------
// |Example1[INFO ]: DEBUG is enabled
// |Example1[INFO ]: DEBUG is disabled
// +----------------------------------------------------------------------------------
void basic3()
{
    if (log[DEBUG]) {
        log[INFO] <<"DEBUG is enabled\n";
    } else {
        log[INFO] <<"DEBUG is disabled\n";
    }

    TemporaryEnable te(log[DEBUG], false);
    if (log[DEBUG]) {
        log[INFO] <<"DEBUG is enabled\n";
    } else {
        log[INFO] <<"DEBUG is disabled\n";
    }
}

// Demonstrate how to avoid expensive print statements when a stream is disabled
// +----------------------------------------------------------------------------------
// |Example1[INFO ]: this should be printed and incremented x=1
// |Example1[INFO ]: final x=1
// +----------------------------------------------------------------------------------
void basic4()
{
    int x = 0;
    log[DEBUG] and log[INFO] <<"this should be printed and incremented x=" <<++x <<"\n";
    TemporaryEnable te(log[DEBUG], false);
    log[DEBUG] and log[INFO] <<"SHOULD NOT BE PRINTED AND NOT INCREMENTED x=" <<++x <<"\n";
    log[INFO] <<"final x=" <<x <<(x!=1 ? " INCORRECT VALUE":"") <<"\n";
}

// Demonstrate the simplest form of partial messages, where each facility+importance is treated as its own distinct stream of
// messages (i.e., any given stream can have only one outstanding partial message).
// This should produce
// +----------------------------------------------------------------------------------
// |Example1[DEBUG]: part1 part2
// |Example1[DEBUG]: first part...
// |Example1[TRACE]: here I am
// |Example1[DEBUG]: first part second part
// +----------------------------------------------------------------------------------
void partial1()
{
    // no interruption between the parts, so they're on one line
    log[DEBUG] <<"part1";
    log[DEBUG] <<" part2\n";

    // the debug stream is interrupted by the info stream, so the debug message is printed twice with elipses
    log[DEBUG] <<"first part";
    log[INFO] <<"here I am\n";
    log[DEBUG] <<" second part\n";
}

// Demonstrate what can go wrong with the partial messages. Namely, since parts are accumulated on a per-stream basis its not
// possible to use the same stream for more than one part. We wanted a message that said "first part second part" but what we
// get is the messsage from basic1() interleaved with our own message.
// +----------------------------------------------------------------------------------
// |Example1[DEBUG]: first partcalled basic1(x=3)
// |Example1[DEBUG]:  second part
// +----------------------------------------------------------------------------------
void partial2()
{
    log[DEBUG] <<"first part";
    basic1(3);
    log[DEBUG] <<" second part\n";
}

// Demonstrate a better way to do partial messages by using Lumberjack::Message objects.  Each object is its own stream of
// linefeed-separated messages.
// +----------------------------------------------------------------------------------
// |Example1[DEBUG]: first part...
// |Example1[DEBUG]: called basic1(x=3)
// |Example1[DEBUG]: first part second part
// +----------------------------------------------------------------------------------
static void partial3()
{
    MessageStream m1(log[DEBUG]);
    m1 <<"first part";
    basic1(3);
    m1 <<" second part\n";
}

void partial3b()
{
    MessageStream m1(log[DEBUG] <<"first part");          // like above, but more compact
    basic1(3);
    m1 <<" second part\n";
}

void partial3c()
{
    MessageStream m1(log[DEBUG] <<"first part");
    basic1(3);
    m1 <<" second part";
    // destructor automatically terminates the message with a line-feed
}

// demonstrate multiple outstanding partial messages
// +----------------------------------------------------------------------------------
// |Example1[DEBUG]: m1 first part...
// |Example1[DEBUG]: m2 first part...
// |Example1[DEBUG]: complete message
// |Example1[DEBUG]: m1 first part finished
// |Example1[DEBUG]: m2 first part finished
// +----------------------------------------------------------------------------------
void partial4()
{
    MessageStream m1(log[DEBUG] <<"m1 first part");
    MessageStream m2(log[DEBUG] <<"m2 first part");
    log[DEBUG] <<"complete message\n";
    m1 <<" finished\n";
    m2 <<" finished\n";
}

// demonstrate that output is unbuffered for partial messages
// +----------------------------------------------------------------------------------
// |Example1[DEBUG]: counting: i=0 i=1 i=2...
// |Example1[INFO ]: about half way
// |Example1[DEBUG]: counting: i=0 i=1 i=2 i=3 i=4 finished
// +----------------------------------------------------------------------------------
void partial5()
{
    log[DEBUG] <<"counting:";
    for (size_t i=0; i<5; ++i) {
        log[DEBUG] <<" i=" <<i;
        sleep(1);
        if (2==i) {
            log[INFO] <<"about half way\n";
            sleep(1);
        }
    }
    log[DEBUG] <<" finished\n";
}

// Demonstrates how messages can be indented. Calling nested2 produces this output:
// +----------------------------------------------------------------------------------
// |Example1[DEBUG]: nested2 begin
// |Example1[DEBUG]:   nested1
// |Example1[DEBUG]: nested2 end
// +----------------------------------------------------------------------------------
void nested1()
{
    log[DEBUG] <<"nested1\n";
}
void nested2()
{
    log[DEBUG] <<"nested2 begin\n";
    {
        MessageIndenter indent(log[DEBUG]);        // indent two extra spaces until destructed
        nested1();
    }
    log[DEBUG] <<"nested2 end\n";
}

// The extra scope is a bit ugly, so here's another way
void nested2b()
{
    MessageIndenter indent(log[DEBUG] <<"nested2 begin\n");
    nested1();
    indent.cancel() <<"nested2 end\n";
}



// Example command-line parsing
int main(int argc, char *argv[])
{
#if 0
    // Tell ROSE that we want users to be able to enable/disable this logging facility from the command line, config files,
    // etc.  and override the facility name with an all-lowercase version (maybe this should be the default).
    ROSE::loggers.insert(log, "example1");
    for (int i=1; i<argc; ++i) {
        if (!strncmp(argv[i], "--log=", 6)) {
            ROSE::loggers.control(argv[i]+6);
        } else if (!strcmp(argv[i], "--log") && i+1<argc) {
            ROSE::loggers.control(argv[++i]);
        } else {
            // ignore the others in this example
        }
    }
#endif

    std::cerr <<"--- basic1 ---\n";
    basic1(3);
    std::cerr <<"--- basic2 ---\n";
    basic2();
    std::cerr <<"--- basic3 ---\n";
    basic3();
    std::cerr <<"--- basic4 ---\n";
    basic4();

    std::cerr <<"--- partial1 ---\n";
    partial1();
    std::cerr <<"--- partial2 ---\n";
    partial2();
    std::cerr <<"--- partial3 ---\n";
    partial3();
    std::cerr <<"--- partial3b ---\n";
    partial3b();
    std::cerr <<"--- partial3c ---\n";
    partial3c();
    std::cerr <<"--- partial4 ---\n";
    partial4();
    std::cerr <<"--- partial5 ---\n";
    partial5();

    std::cerr <<"--- nested1 ---\n";
    nested1();
    std::cerr <<"--- nested2 ---\n";
    nested2();
    std::cerr <<"--- nested2b ---\n";
    nested2b();
    return 0;
}


#endif

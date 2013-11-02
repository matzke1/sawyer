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
        std::string operator()(const MessageProps&, const std::string&, MessageImportance) /*override*/ {
            return "";
        }
    } prefix;

    struct Sink: MessageFileSink {
        Sink(): MessageFileSink(std::cerr) {}
        void output(const MessageProps &mesg, const std::string &full_line, size_t new_stuff) /*override*/ {
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

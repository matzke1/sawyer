#include "CommandLine.h"
#include <boost/algorithm/string/predicate.hpp>

using namespace Sawyer::CommandLine;

static void showCommandLine(const std::vector<std::string> &args) {
    std::cout <<"  command line:";
    BOOST_FOREACH (const std::string arg, args) {
        if (arg.empty()) {
            std::cout <<" \"\"";
        } else {
            std::cout <<" " <<arg;
        }
    }
    std::cout <<"\n";
}

static void mustParse(size_t nParsed, Parser &p, const std::vector<std::string> &args) {
    showCommandLine(args);
    ParserResult pr = p.parse(args);
    ASSERT_require2(pr.parsedArgs().size()==nParsed,
                    "must have parsed exactly " + boost::lexical_cast<std::string>(nParsed) +
                    " argument" + (1==nParsed?"":"s"));
    pr.apply();
}
static void mustParse(size_t nParsed, Parser &p) {
    std::vector<std::string> args;
    mustParse(nParsed, p, args);
}
static void mustParse(size_t nParsed, Parser &p, const std::string &a1) {
    std::vector<std::string> args;
    args.push_back(a1);
    mustParse(nParsed, p, args);
}
static void mustParse(size_t nParsed, Parser &p, const std::string &a1, const std::string &a2) {
    std::vector<std::string> args;
    args.push_back(a1);
    args.push_back(a2);
    mustParse(nParsed, p, args);
}
static void mustParse(size_t nParsed, Parser &p, const std::string &a1, const std::string &a2, const std::string &a3) {
    std::vector<std::string> args;
    args.push_back(a1);
    args.push_back(a2);
    args.push_back(a3);
    mustParse(nParsed, p, args);
}

static void mustNotParse(const std::string &errmesg, Parser &p, const std::vector<std::string> &args) {
    showCommandLine(args);
    try {
        p.parse(args);
        ASSERT_not_reachable("parser should have failed but didn't");
    } catch (const std::runtime_error &e) {
        if (!boost::contains(e.what(), errmesg)) {
            std::cerr <<"actual error message: " <<e.what() <<"\n"
                      <<"expected message: " <<errmesg <<"\n";
            ASSERT_not_reachable("parser failed with the wrong error message");
        }
        std::cout <<"    " <<e.what() <<"\n";
        return;
    }
    ASSERT_not_reachable("parser failed with wrong exception type");
}
static void mustNotParse(const std::string &errmesg, Parser &p, const std::string &a1) {
    std::vector<std::string> args;
    args.push_back(a1);
    mustNotParse(errmesg, p, args);
}
static void mustNotParse(const std::string &errmesg, Parser &p, const std::string &a1, const std::string &a2) {
    std::vector<std::string> args;
    args.push_back(a1);
    args.push_back(a2);
    mustNotParse(errmesg, p, args);
}
static void mustNotParse(const std::string &errmesg, Parser &p, const std::string &a1, const std::string &a2,
                         const std::string &a3) {
    std::vector<std::string> args;
    args.push_back(a1);
    args.push_back(a2);
    args.push_back(a3);
    mustNotParse(errmesg, p, args);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      The tests
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Most basic switch possible
static void test01() {
    std::cerr <<"test01: most basic switch possible\n";
    Parser p;
    p.with(Switch("sw"));
    mustParse(0, p);
    mustParse(1, p, "--sw");
    mustParse(2, p, "--sw", "--sw");
    mustParse(1, p, "--sw", "p1");
    mustNotParse("unrecognized switch", p, "--s", "p1");
    mustNotParse("unrecognized switch", p, "--sww", "p1");
    mustNotParse("unrecognized switch", p, "-sw");
}

// Short names
static void test02() {
    std::cerr <<"test02: short names\n";
    Parser p;
    p.with(Switch("", 's'));
    mustParse(1, p, "-s");
    mustParse(1, p, "-ss");
    mustParse(2, p, "-s", "-s");
    mustNotParse("unrecognized switch: -st", p, "-st");
}

// Long and short names are the same
static void test03() {
    std::cerr <<"test03: long and short names are the same\n";
    Parser p;
    p.with(Switch("s", 's'));
    mustParse(1, p, "--s");
    mustParse(1, p, "-s");
}

// Multiple long names
static void test04() {
    std::cerr <<"test04: multiple long names\n";
    Parser p;
    p.with(Switch("gray", 'g')
           .longName("grey")
           .shortName('b'));
    mustParse(1, p, "--gray");
    mustParse(1, p, "--grey");
    mustParse(1, p, "-g");
    mustParse(1, p, "-b");
    mustParse(1, p, "-gb");
    mustParse(1, p, "-bg");
    mustNotParse("unrecognized switch", p, "--gris");
}

// Reset long prefixes
static void test05() {
    std::cerr <<"test05: reset long prefixes\n";
    Parser p;
    p.with(Switch("log", 'l')
           .resetLongPrefixes("-rose:"));
    mustParse(1, p, "-rose:log");
    mustParse(1, p, "-l");
    mustNotParse("unrecognized switch", p, "--log");
    mustNotParse("unrecognized switch", p, "--rose:log");
    mustNotParse("unrecognized switch", p, "-rose:");
}

// Additional long prefixes
static void test06() {
    std::cerr <<"test06: additional long prefixes\n";
    Parser p;
    p.with(Switch("log", 'l')
           .longPrefix("-rose:"));
    mustParse(1, p, "-rose:log");
    mustParse(1, p, "--log");
    mustNotParse("unrecognized switch", p, "--rose:log");
    mustNotParse("unrecognized switch", p, "-log");
}

// Saving intrinsic values using a parser
static void test07a() {
    std::cerr <<"test07a: saving intrinsic values using a parser\n";
    bool b;
    int i;
    double d;
    Parser p;
    p.with(Switch("bool")
           .intrinsicValue("true", booleanParser(b)));
    p.with(Switch("int")
           .intrinsicValue("1", integerParser(i)));
    p.with(Switch("dbl")
           .intrinsicValue("3.14", realNumberParser(d)));

    d = i = b = 0;
    mustParse(1, p, "--bool");
    ASSERT_require(b==true);
    ASSERT_require(i==0);
    ASSERT_require(d==0);

    d = i = b = 0;
    mustParse(1, p, "--int");
    ASSERT_require(b==false);
    ASSERT_require(i==1);
    ASSERT_require(d==0);

    d = i = b = 0;
    mustParse(1, p, "--dbl");
    ASSERT_require(b==false);
    ASSERT_require(i==0);
    ASSERT_require(d!=0);

    d = i = b = 0;
    mustParse(3, p, "--int", "--bool", "--dbl");
    ASSERT_require(b==true);
    ASSERT_require(i==1);
    ASSERT_require(d!=0);
}

// Saving intrinsic values without a parser
static void test07b() {
    std::cerr <<"test07b: saving intrinsic values without a parser\n";
    bool b;
    int i;
    double d;
    std::string s;
    Parser p;
    p.with(Switch("bool")
           .intrinsicValue(true, b));
    p.with(Switch("int")
           .intrinsicValue(1, i));
    p.with(Switch("dbl")
           .intrinsicValue(3.14, d));
    p.with(Switch("str")
           .intrinsicValue("hello", s));

    d = i = b = 0;
    mustParse(1, p, "--bool");
    ASSERT_require(b==true);
    ASSERT_require(i==0);
    ASSERT_require(d==0);

    d = i = b = 0;
    mustParse(1, p, "--int");
    ASSERT_require(b==false);
    ASSERT_require(i==1);
    ASSERT_require(d==0);

    d = i = b = 0;
    mustParse(1, p, "--dbl");
    ASSERT_require(b==false);
    ASSERT_require(i==0);
    ASSERT_require(d!=0);

    s = "";
    mustParse(1, p, "--str");
    ASSERT_require(s=="hello");

    d = i = b = 0;
    mustParse(3, p, "--int", "--bool", "--dbl");
    ASSERT_require(b==true);
    ASSERT_require(i==1);
    ASSERT_require(d!=0);
}

// Required argument
static void test08() {
    std::cerr <<"test08: required argument\n";
    Parser p;
    p.with(Switch("sw", 's')
           .argument("arg1"));
    mustParse(2, p, "--sw", "aaa", "bbb");
    mustParse(2, p, "-s", "ccc", "ddd");
    mustParse(1, p, "--sw=eee", "fff");
    mustParse(1, p, "-sggg", "hhh");
    mustNotParse("required argument for --sw is missing", p, "--sw");
    mustNotParse("required argument for -s is missing", p, "-s");
}

// Required argument that is saved
static void test09() {
    std::cerr <<"test09: required argument that is saved\n";
    std::string arg;
    Parser p;
    p.with(Switch("any", 'a')
           .argument("arg1", anyParser(arg)));

    mustParse(1, p, "--any=aaa", "bbb");
    ASSERT_require(arg=="aaa");

    mustParse(2, p, "--any", "ccc", "ddd");
    ASSERT_require(arg=="ccc");

    mustParse(1, p, "-aeee", "fff");
    ASSERT_require(arg=="eee");

    mustParse(2, p, "-a", "ggg", "hhh");
    ASSERT_require(arg=="ggg");

    mustParse(1, p, "-aaa", "bbb");
    ASSERT_require(arg=="aa");
}

// Integer parser
static void test10() {
    std::cerr <<"test10: integer parser\n";
    int si;
    short ss;
    unsigned int ui;
    unsigned short us;
    Parser p;
    p.with(Switch("si", 'a')
           .argument("arg", integerParser(si)));
    p.with(Switch("ui", 'b')
           .argument("arg", integerParser(ui)));
    p.with(Switch("ss", 'c')
           .argument("arg", integerParser(ss)));
    p.with(Switch("us", 'd')
           .argument("arg", integerParser(us)));

    // signed int
    mustParse(1, p, "--si=123", "0");
    ASSERT_require(si==123);

    mustParse(2, p, "--si", "456", "0");
    ASSERT_require(si==456);

    mustParse(1, p, "--si=-123", "0");
    ASSERT_require(si==-123);

    mustParse(2, p, "--si", "-456", "0");
    ASSERT_require(si==-456);

    mustParse(1, p, "-a123", "0");
    ASSERT_require(si==123);
    
    mustParse(2, p, "-a", "456", "0");
    ASSERT_require(si==456);

    mustParse(1, p, "-a-123", "0");
    ASSERT_require(si==-123);

    mustParse(2, p, "-a", "-456", "0");
    ASSERT_require(si==-456);

    mustNotParse("is greater than 2147483647", p, "--si=5000000000", "0");
    mustNotParse("is less than -2147483648", p, "--si=-5000000000", "0");

    // unsigned int
    mustParse(1, p, "--ui=123", "0");
    ASSERT_require(ui==123);

    mustParse(2, p, "--ui", "456", "0");
    ASSERT_require(ui==456);

    mustParse(1, p, "-b123", "0");
    ASSERT_require(ui==123);
    
    mustParse(2, p, "-b", "456", "0");
    ASSERT_require(ui==456);

    mustNotParse("is less than 0", p, "--ui=-123", "0");
    mustNotParse("is less than 0", p, "--ui", "-456", "0");

    mustNotParse("is less than 0", p, "-b-123", "0");
    mustNotParse("is less than 0", p, "-b", "-456", "0");

    mustNotParse("is greater than 4294967295", p, "--ui=5000000000", "0");
    mustNotParse("is less than 0", p, "--ui=-5000000000", "0");

    // signed short
    mustParse(1, p, "--ss=123", "0");
    ASSERT_require(ss==123);

    mustParse(2, p, "--ss", "456", "0");
    ASSERT_require(ss==456);

    mustParse(1, p, "--ss=-123", "0");
    ASSERT_require(ss==-123);

    mustParse(2, p, "--ss", "-456", "0");
    ASSERT_require(ss==-456);

    mustParse(1, p, "-c123", "0");
    ASSERT_require(ss==123);
    
    mustParse(2, p, "-c", "456", "0");
    ASSERT_require(ss==456);

    mustParse(1, p, "-c-123", "0");
    ASSERT_require(ss==-123);

    mustParse(2, p, "-c", "-456", "0");
    ASSERT_require(ss==-456);

    mustNotParse("is greater than 32767", p, "--ss=5000000000", "0");
    mustNotParse("is less than -32768", p, "--ss=-5000000000", "0");

    // unsigned short
    mustParse(1, p, "--us=123", "0");
    ASSERT_require(us==123);

    mustParse(2, p, "--us", "456", "0");
    ASSERT_require(us==456);

    mustParse(1, p, "-d123", "0");
    ASSERT_require(us==123);
    
    mustParse(2, p, "-d", "456", "0");
    ASSERT_require(us==456);

    mustNotParse("is less than 0", p, "--us=-123", "0");
    mustNotParse("is less than 0", p, "--us", "-456", "0");

    mustNotParse("is less than 0", p, "-d-123", "0");
    mustNotParse("is less than 0", p, "-d", "-456", "0");

    mustNotParse("is greater than 65535", p, "--us=5000000000", "0");
    mustNotParse("is less than 0", p, "--us=-5000000000", "0");

    // syntax errors
    mustNotParse("integer expected", p, "--si=x");
    mustNotParse("integer expected", p, "--si",  "x");
    mustNotParse("unexpectedly followed by", p, "--si=123x");
    mustNotParse("unrecognized switch: -a123x", p, "-a123x");
    mustNotParse("unexpectedly followed by", p, "-a", "123x");
}

// non-negative integer parser
static void test11() {
    std::cerr <<"test11: non-negative integer parser\n";
    int si;
    short ss;
    unsigned int ui;
    unsigned short us;
    Parser p;
    p.with(Switch("si", 'a')
           .argument("arg", nonNegativeIntegerParser(si)));
    p.with(Switch("ui", 'b')
           .argument("arg", nonNegativeIntegerParser(ui)));
    p.with(Switch("ss", 'c')
           .argument("arg", nonNegativeIntegerParser(ss)));
    p.with(Switch("us", 'd')
           .argument("arg", nonNegativeIntegerParser(us)));

    // signed int
    mustParse(1, p, "--si=123", "0");
    ASSERT_require(si==123);

    mustParse(2, p, "--si", "456", "0");
    ASSERT_require(si==456);

    mustNotParse("unsigned integer expected", p, "--si=-123", "0");
    mustNotParse("unsigned integer expected", p, "--si", "-456", "0");

    mustParse(1, p, "-a123", "0");
    ASSERT_require(si==123);
    
    mustParse(2, p, "-a", "456", "0");
    ASSERT_require(si==456);

    mustNotParse("unsigned integer expected", p, "-a-123", "0");
    mustNotParse("unsigned integer expected", p, "-a", "-456", "0");
    mustNotParse("is greater than 2147483647", p, "--si=5000000000", "0");
    mustNotParse("unsigned integer expected", p, "--si=-5000000000", "0");

    // unsigned int
    mustParse(1, p, "--ui=123", "0");
    ASSERT_require(ui==123);

    mustParse(2, p, "--ui", "456", "0");
    ASSERT_require(ui==456);

    mustParse(1, p, "-b123", "0");
    ASSERT_require(ui==123);
    
    mustParse(2, p, "-b", "456", "0");
    ASSERT_require(ui==456);

    mustNotParse("unsigned integer expected", p, "--ui=-123", "0");
    mustNotParse("unsigned integer expected", p, "--ui", "-456", "0");
    mustNotParse("unsigned integer expected", p, "-b-123", "0");
    mustNotParse("unsigned integer expected", p, "-b", "-456", "0");
    mustNotParse("is greater than 4294967295", p, "--ui=5000000000", "0");
    mustNotParse("unsigned integer expected", p, "--ui=-5000000000", "0");

    // signed short
    mustParse(1, p, "--ss=123", "0");
    ASSERT_require(ss==123);

    mustParse(2, p, "--ss", "456", "0");
    ASSERT_require(ss==456);

    mustNotParse("unsigned integer expected", p, "--ss=-123", "0");
    mustNotParse("unsigned integer expected", p, "--ss", "-456", "0");

    mustParse(1, p, "-c123", "0");
    ASSERT_require(ss==123);
    
    mustParse(2, p, "-c", "456", "0");
    ASSERT_require(ss==456);

    mustNotParse("unsigned integer expected", p, "-c-123", "0");
    mustNotParse("unsigned integer expected", p, "-c", "-456", "0");
    mustNotParse("is greater than 32767", p, "--ss=5000000000", "0");
    mustNotParse("unsigned integer expected", p, "--ss=-5000000000", "0");

    // unsigned short
    mustParse(1, p, "--us=123", "0");
    ASSERT_require(us==123);

    mustParse(2, p, "--us", "456", "0");
    ASSERT_require(us==456);

    mustParse(1, p, "-d123", "0");
    ASSERT_require(us==123);
    
    mustParse(2, p, "-d", "456", "0");
    ASSERT_require(us==456);

    mustNotParse("unsigned integer expected", p, "--us=-123", "0");
    mustNotParse("unsigned integer expected", p, "--us", "-456", "0");
    mustNotParse("unsigned integer expected", p, "-d-123", "0");
    mustNotParse("unsigned integer expected", p, "-d", "-456", "0");

    mustNotParse("is greater than 65535", p, "--us=5000000000", "0");
    mustNotParse("unsigned integer expected", p, "--us=-5000000000", "0");

    // syntax errors
    mustNotParse("unsigned integer expected", p, "--si=x");
    mustNotParse("unsigned integer expected", p, "--si",  "x");
    mustNotParse("unexpectedly followed by", p, "--si=123x");
    mustNotParse("unrecognized switch: -a123x", p, "-a123x");
    mustNotParse("unexpectedly followed by", p, "-a", "123x");
}

// Boolean parser
static void test12() {
    std::cerr <<"test12: Boolean parser\n";
    bool b1=false, b2=false;
    Parser p;
    p.with(Switch("first", 'f').argument("boolean", booleanParser(b1)));
    p.with(Switch("second", 'r').argument("boolean", booleanParser(b2)));

    mustParse(1, p, "--first=yes", "no");
    ASSERT_require(b1==true);
    mustParse(1, p, "--first=no", "yes");
    ASSERT_require(b1==false);
    mustParse(1, p, "--first=y", "n");
    ASSERT_require(b1==true);
    mustParse(1, p, "--first=n", "y");
    ASSERT_require(b1==false);
    mustParse(1, p, "--first=true", "false");
    ASSERT_require(b1==true);
    mustParse(1, p, "--first=false", "true");
    ASSERT_require(b1==false);
    mustParse(1, p, "--first=t", "f");
    ASSERT_require(b1==true);
    mustParse(1, p, "--first=f", "t");
    ASSERT_require(b1==false);
    mustParse(1, p, "--first=on", "off");
    ASSERT_require(b1==true);
    mustParse(1, p, "--first=off", "on");
    ASSERT_require(b1==false);
    mustParse(1, p, "--first=1", "0");
    ASSERT_require(b1==true);
    mustParse(1, p, "--first=0", "1");
    ASSERT_require(b1==false);

    mustParse(1, p, "-ftrt");
    ASSERT_require(b1==true);
    ASSERT_require(b2==true);
    mustParse(1, p, "-ffrf");
    ASSERT_require(b1==false);
    ASSERT_require(b2==false);
    mustParse(1, p, "-ftrue");
    ASSERT_require(b1==true);
    mustParse(1, p, "-ftruery");
    ASSERT_require(b1==true);
    ASSERT_require(b2==true);
    mustNotParse("required argument for -r is missing", p, "-ftru");
}

// String set parser
static void test13() {
    std::cerr <<"test13: string set parser\n";
    std::string s;
    Parser p;
    p.with(Switch("bkg", 'd')
           .argument("color",
                     stringSetParser(s)
                     ->with("red")
                     ->with("reddish")
                     ->with("ish")));
    mustParse(1, p, "--bkg=reddish");
    ASSERT_require(s=="reddish");

    mustParse(1, p, "--bkg=red");
    ASSERT_require(s=="red");

    mustParse(2, p, "-dred", "-dish");
    ASSERT_require(s=="ish");

    mustParse(1, p, "-dreddish");
    ASSERT_require(s=="reddish");                       // i.e., not "-d red -d ish"

    mustNotParse("specific word expected", p, "-dread");
    mustNotParse("specific word expected", p, "--bkg=blue");
}

// Enum parser
enum TestColor { RED, REDDISH, ISH };
static void test14() {
    std::cerr <<"test14: enum parser\n";
    TestColor s;
    Parser p;
    p.with(Switch("bkg", 'd')
           .argument("color",
                     enumParser(s)
                     ->with("red", RED)
                     ->with("reddish", REDDISH)
                     ->with("ish", ISH)));
    mustParse(1, p, "--bkg=reddish");
    ASSERT_require(s==REDDISH);

    mustParse(1, p, "--bkg=red");
    ASSERT_require(s==RED);

    mustParse(2, p, "-dred", "-dish");
    ASSERT_require(s==ISH);

    mustParse(1, p, "-dreddish");
    ASSERT_require(s==REDDISH);                         // i.e., not "-d red -d ish"

    mustNotParse("specific word expected", p, "-dread");
    mustNotParse("specific word expected", p, "--bkg=blue");
}

// List parser
static void test15() {
    std::cerr <<"test15: list parser\n";
    int v1, v2;
    std::string s1, s2;
    Parser p;
    p.with(Switch("ints", 'I')
           .argument("integers",
                     listParser(integerParser(v1))->nextMember(integerParser(v2))));
    p.with(Switch("strs", 'S')
           .argument("strings",
                     listParser(anyParser(s1), "-")->nextMember(anyParser(s2))));

    mustParse(1, p, "--ints=1,2", "3");
    ASSERT_require(v1==1);
    ASSERT_require(v2==2);

    mustParse(1, p, "-I10;20,30:40", "50");
    ASSERT_require(v1==10);
    ASSERT_require(v2==40);

    mustParse(2, p, "-I", "1", "2");
    ASSERT_require(v1==1);

    mustParse(1, p, "--strs=aa,a-b,cc");
    ASSERT_require(s1=="aa,a");
    ASSERT_require(s2=="cc");
}

// Optional argument
static void test16() {
    std::cerr <<"test16: optional argument\n";
    int width = 0;
    Parser p;
    p.with(Switch("width", 'w')
           .argument("n", nonNegativeIntegerParser(width), "80"));

    mustParse(1, p, "--width", "aaa");
    ASSERT_require(width==80);

    mustParse(2, p, "--width", "90", "aaa");
    ASSERT_require(width==90);

    mustParse(2, p, "--width", "100", "101");
    ASSERT_require(width==100);

    mustParse(1, p, "--width=10", "20");
    ASSERT_require(width==10);

    mustParse(1, p, "-w", "aaa");
    ASSERT_require(width==80);

    mustParse(1, p, "-w20", "21");
    ASSERT_require(width==20);

    mustParse(1, p, "-w30w40", "41");
    ASSERT_require(width==40);

    width = 0;
    mustNotParse("unexpected empty-string argument after --width default argument", p, "--width=");
    ASSERT_require(width==0);

    width = 0;
    mustNotParse("unrecognized switch: -w80x", p, "-w80x");
    ASSERT_require(width==0);
}

// Value separators
static void test17() {
    std::cerr <<"test17: value separators\n";
    int height = 0;
    Parser p;
    p.with(Switch("height", '1')
           .argument("n", integerParser(height))
           .valueSeparator("->"));

    mustParse(1, p, "--height->50", "0");
    ASSERT_require(height==50);

    mustParse(2, p, "--height", "60", "0");
    ASSERT_require(height==60);

    mustParse(1, p, "--height->-50");
    ASSERT_require(height==-50);

    mustParse(1, p, "-170");
    ASSERT_require(height==70);

    mustParse(2, p, "-1", "-1", "100");
    ASSERT_require(height==-1);
}

// Multiple arguments
static void test18() {
    std::cerr <<"test18: multiple arguments\n";
    int v1=0, v2=0;
    Parser p;
    p.with(Switch("swap", 'S')
           .argument("first", integerParser(v1))
           .argument("second", integerParser(v2)));

    mustParse(3, p, "--swap", "1", "2");
    ASSERT_require(v1==1);
    ASSERT_require(v2==2);

    mustParse(2, p, "--swap=3", "4", "5");
    ASSERT_require(v1==3);
    ASSERT_require(v2==4);

    mustNotParse("unexpectedly followed by", p, "--swap=6,7", "8");
    mustNotParse("second: integer expected", p, "--swap", "1", "aaa");
}

// Capturing values for all occurrences
static void test19() {
    std::cerr <<"test19: capturing values for all occurrences\n";
    std::vector<int> v1;
    Parser p;
    p.with(Switch("first")
           .argument("n", integerParser(v1))
           .whichValue(SAVE_FIRST));
    p.with(Switch("last")
           .argument("n", integerParser(v1))
           .whichValue(SAVE_LAST));
    p.with(Switch("one")
           .argument("n", integerParser(v1))
           .whichValue(SAVE_ONE));
    p.with(Switch("all")
           .argument("n", integerParser(v1))
           .whichValue(SAVE_ALL));
    p.with(Switch("none")
           .argument("n", integerParser(v1))
           .whichValue(SAVE_NONE));
    p.with(Switch("sum")
           .argument("n", integerParser(v1))
           .whichValue(SAVE_AUGMENTED)
           .valueAugmenter(sum<int>()));

    v1.clear();
    mustParse(2, p, "--first=1", "--first=2");
    ASSERT_require(v1.size()==1);
    ASSERT_require(v1[0]==1);

    v1.clear();
    mustParse(2, p, "--last=3", "--last=4");
    ASSERT_require(v1.size()==1);
    ASSERT_require(v1[0]==4);

    v1.clear();
    mustParse(2, p, "--all=5", "--all=6");
    ASSERT_require(v1.size()==2);
    ASSERT_require(v1[0]==5);
    ASSERT_require(v1[1]==6);

    v1.clear();
    mustParse(2, p, "--sum=7", "--sum=8");
    ASSERT_require(v1.size()==1);
    ASSERT_require(v1[0]==15);

    mustNotParse("cannot appear multiple times", p, "--one=1", "--one=2");
    mustNotParse("--none is illegal here", p, "--none=1");
}

// Empty program arguments
static void test20() {
    std::cerr <<"test20: empty program arguments\n";
    std::string s;
    int i = 0;
    std::vector<std::string> v;
    Parser p;
    p.with(Switch("str", 's').argument("username", anyParser(s)));
    p.with(Switch("int", 'i').argument("width", integerParser(i)));
    p.with(Switch("vec", 'v').argument("vector", listParser(anyParser(v))));

    mustParse(0, p, "");

    mustParse(1, p, "--str=", "aaa");
    ASSERT_require(s=="");

    mustParse(2, p, "--str", "", "aaa");
    ASSERT_require(s=="");

    mustParse(2, p, "--str", "--", "aaa");
    ASSERT_require(s=="--");

    mustParse(2, p, "-s", "", "aaa");
    ASSERT_require(s=="");

    mustParse(2, p, "-s", "--", "aaa");
    ASSERT_require(s=="--");

    v.clear();
    mustParse(1, p, "--vec=a,,b", "aaa");
    ASSERT_require(3==v.size());
    ASSERT_require(v[0]=="a");
    ASSERT_require(v[1]=="");
    ASSERT_require(v[2]=="b");

    mustNotParse("required argument for --int is missing; for width: integer expected", p, "--int", "", "123");
    mustNotParse("required argument for --int is missing; for width: integer expected", p, "--int=", "123");
}

// Using boost::lexical_cast via anyParser
static void test21() {
    std::cerr <<"test21: using boost::lexical cast via anyParser\n";
    Parser p;
    short i = 0;
    p.with(Switch("int", 'i').argument("arg1", anyParser(i)));

    mustParse(1, p, "--int=123", "aaa");
    ASSERT_require(i==123);

    mustNotParse("bad lexical cast", p, "--int=123x", "aaa");
}

// Saving user-defined intrinsic values without a parser
class UserDef1 {
    int i;
    std::string s;
public:
    UserDef1(): i(0) {}
    UserDef1(int i, const std::string &s): i(i), s(s){}
    bool operator==(const UserDef1 &other) const {
        return i==other.i && s==other.s;
    }
    friend std::ostream& operator<<(std::ostream &o, const UserDef1 &u) {
        o <<"{" <<u.i <<"," <<u.s <<"}";
        return o;
    }
};

static void test22() {
    std::cerr <<"test22: saving user-defined intrinsic values without a parser\n";
    Parser p;
    UserDef1 v1, v2;
    p.with(Switch("v1")
           .intrinsicValue(UserDef1(1, "one"), v1));
    p.with(Switch("v2")
           .intrinsicValue(UserDef1(2, "two"), v2));

    ASSERT_require(v1==UserDef1());
    ASSERT_require(v2==UserDef1());

    mustParse(1, p, "--v1", "aaa");
    ASSERT_require(v1==UserDef1(1, "one"));
    ASSERT_require(v2==UserDef1());

    mustParse(2, p, "--v1", "--v2");
    ASSERT_require(v1==UserDef1(1, "one"));
    ASSERT_require(v2==UserDef1(2, "two"));
}

int main(int argc, char *argv[]) {
    test01();
    test02();
    test03();
    test04();
    test05();
    test06();
    test07a();
    test07b();
    test08();
    test09();
    test10();
    test11();
    test12();
    test13();
    test14();
    test15();
    test16();
    test17();
    test18();
    test19();
    test20();
    test21();
    test22();
    std::cout <<"All tests passed\n";
    return 0;
}

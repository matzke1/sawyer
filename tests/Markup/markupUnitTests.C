#include <Sawyer/Markup.h>
#include <Sawyer/MarkupPod.h>

#include <iostream>
#include <Sawyer/Assert.h>
#include <sstream>

using namespace Sawyer;
using namespace Sawyer::Markup;

class MultiLine {
    std::string s;
public:
    explicit MultiLine(const std::string &s): s(s) {}
    void print(std::ostream &out) const {
        if (s=="") {
            out <<"[]";
        } else {
            bool hasInternalLineFeed = false;
            for (size_t i=0; i<s.size()-1 && !hasInternalLineFeed; ++i)
                hasInternalLineFeed = '\n' == s[i];
            bool hasFinalLineFeed = '\n' == s[s.size()-1];

            if (!hasInternalLineFeed) {
                out <<"[" <<s <<"]" <<(hasFinalLineFeed ? "" : " (no final linefeed)");
            } else {
                std::string termination = "]\n       [";
                out <<"\n       [";
                for (size_t i=0; i<s.size(); ++i) {
                    if ('\n'==s[i]) {
                        if (i < s.size()-1)
                            out <<"]\n       [";
                    } else if ('\t'==s[i]) {
                        out <<"\\t";
                    } else {
                        out <<s[i];
                    }
                }
                out <<"]" <<(hasFinalLineFeed?"":" (no final linefeed)");
            }
        }
    }
};

std::ostream& operator<<(std::ostream &out, const MultiLine &m) {
    m.print(out);
    return out;
}

static void parse(const std::string &input, const std::string &expect) {
    try {
        Parser parser;
        ParserResult parsed = parser.parse(input);
        std::ostringstream ss;
        parsed.print(ss);
        if (ss.str() != expect) {
            std::cerr <<"parser failure: parsed string doesn't match expected result:\n";
            std::cerr <<"  parsed string   = " <<MultiLine(input) <<"\n";
            std::cerr <<"  expected result = " <<MultiLine(expect) << "\n";
            std::cerr <<"  but got result  = " <<MultiLine(ss.str()) <<"\n";
            ASSERT_not_reachable("test failed");
        }
    } catch (const std::runtime_error &e) {
        std::cerr <<"parser failure: problem parsing string:\n";
        std::cerr <<"  parsed string   = " <<MultiLine(input) <<"\n";
        std::cerr <<"  expected result = " <<MultiLine(expect) <<"\n";
        std::cerr <<"  but got error   = " <<MultiLine(e.what()) <<"\n";
        ASSERT_not_reachable("test failed");
    }
}

static void dontParse(const std::string &input, const std::string &error) {
    try {
        Parser parser;
        ParserResult parsed = parser.parse(input);
        std::cerr <<"parser failure: parsed string that should have been an error:\n";
        std::cerr <<"  parsed string    = " <<MultiLine(input) <<"\n";
        std::cerr <<"  expected failure = " <<MultiLine(error) <<"\n";
        std::ostringstream ss;
        parsed.print(ss);
        std::cerr <<"  but got result   = " <<MultiLine(ss.str()) <<"\n";
        ASSERT_not_reachable("test failed");
    } catch (const std::runtime_error &e) {
        if (0!=error.compare(e.what())) {
            std::cerr <<"parser failure: wrong error message:\n";
            std::cerr <<"  parsed string    = " <<MultiLine(input) <<"\n";
            std::cerr <<"  expected failure = " <<MultiLine(error) <<"\n";
            std::cerr <<"  but got error    = " <<MultiLine(e.what()) <<"\n";
            ASSERT_not_reachable("test failed");
        }
    }
}


// Simple things that should parse
static void test_basic() {
    parse("Basic test",
          "DATA{Basic test}");

    parse("@section(Synopsis)(Hello world)",
          "@section{DATA{Synopsis}}{DATA{Hello world}}");

    parse("@section Synopsis(Hello world)",
          "@section{DATA{Synopsis}}{DATA{Hello world}}");

    parse("@section Synopsis Hello other",
          "@section{DATA{Synopsis}}{DATA{Hello}}DATA{ other}");

    parse("@section foo 2 3",
          "@section{DATA{foo}}{DATA{2}}DATA{ 3}");

    parse("@section<hello><world>",
          "@section{DATA{hello}}{DATA{world}}");

    parse("@section[hello](world)",
          "@section{DATA{hello}}{DATA{world}}");

    parse("@section hello world. And more",
          "@section{DATA{hello}}{DATA{world}}DATA{. And more}");

    parse("@not_a_tag",
          "DATA{@not_a_tag}");

    parse("@sectionational foo",
          "DATA{@sectionational foo}");

    parse("@section<a<b>> c",
          "@section{DATA{a<b>}}{DATA{c}}");

    parse("@comment[@em<@prose<>>]",                   // @em<@prose<>> would normally be an error
          "@comment{DATA{@em<@prose<>>}}");
}

// Things that are errors
static void test_errors() {
    dontParse("@section",
              "expected section argument 1 at end of input");

    //         ---------------V
    dontParse("@prose[@section 1]",
              "expected section argument 2 at byte 17");

    //         ----------V
    dontParse("@em[@prose[hi]]",
              "'prose' is not allowed inside a paragraph at byte 10");

    //         ---------------V
    dontParse("@section[@prose(hello)][world]",
              "'prose' is not allowed inside a paragraph at byte 15");
}

static void pod(const std::string &input, const std::string &expected) {
    ParserResult markup;
    try {
        markup = Parser().parse(input);
    } catch (const std::runtime_error &e) {
        std::cerr <<"parser failure: problems parsing string:\n";
        std::cerr <<"  parsed string = " <<MultiLine(input) <<"\n";
        std::cerr <<"  error is      = " <<MultiLine(e.what()) <<"\n";
        ASSERT_not_reachable("test failed");
    }

    PodFormatter::Ptr formatter = PodFormatter::instance();
    formatter->title("test");
    formatter->version("alpha", "2014-06-16");
    std::ostringstream ss;
    markup.emit(ss, formatter);

    if (ss.str() != expected) {
        std::cerr <<"POD formatter produced unexpected results:\n";
        std::cerr <<"  parsed string   = " <<MultiLine(input) <<"\n";
        std::ostringstream result;
        markup.print(result);
        std::cerr <<"  parser result   = " <<MultiLine(result.str()) <<"\n";
        std::cerr <<"  expected output = " <<MultiLine(expected) <<"\n";
        std::cerr <<"  but got output  = " <<MultiLine(ss.str()) <<"\n";
        ASSERT_not_reachable("test failed");
    }
}

// Test POD output
static void test_pod() {
    // Empty documents
    pod("",
        "=pod\n\n\n=cut\n");

    pod("@comment{hello world}",
        "=pod\n\n\n=cut\n");

    // Only prose
    pod("  Hello world",
        "=pod\n\nHello world\n\n=cut\n");

    pod("  Hello world\n",
        "=pod\n\nHello world\n\n=cut\n");

    pod("\n  Hello world\n",
        "=pod\n\n\nHello world\n\n=cut\n");

    pod("Hello\n  World\n",
        "=pod\n\nHello\nWorld\n\n=cut\n");

    // Only non-prose
    pod("@nonprose(Hello world)",
        "=pod\n"
        "\n"
        "\n"
        "=over\n"
        "\n"
        "\tHello world\n"
        "\n"
        "=back\n"
        "\n"
        "\n"
        "\n"
        "=cut\n");
}


int main() {
    test_basic();
    test_errors();
    test_pod();
}

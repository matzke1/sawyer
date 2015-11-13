// Searches for tokens in source code.

#include <Sawyer/Clexer.h>
#include <Sawyer/CommandLine.h>
#include <Sawyer/Message.h>
#include <Sawyer/StaticBuffer.h>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <vector>

using namespace Sawyer::Message::Common;
using namespace Sawyer::Language;

static Sawyer::Message::Facility mlog;

std::vector<std::string>
parseCommandLine(int argc, char *argv[]) {
    using namespace Sawyer::CommandLine;
    Parser parser;
    parser.errorStream(mlog[FATAL]);
    parser.purpose("search for tokens in source code");
    parser.doc("Synopsis",
               "@prop{programName} @v{needle} @v{haystack}...");
    parser.doc("Description",
               "Similar to Unix @man{grep}(1) in that it finds the proverbial needle in the haystack. The first argument is "
               "the \"needle\" for which to search and the remaining arguments are the names of source files that serve "
               "as the \"haystack\".  Unlike grep, this command searches using lexical analysis: it converts the first "
               "argument into a list of tokens and then tries to find matching tokens in the haystack. As such, white "
               "space and comments are ignored, and needles are not found as substrings of string literals (of course, "
               "the needle can contain a string literal which will only match an identical string literal in the haystack.");

    parser.with(Switch("help", 'h')
                .action(showHelpAndExit(0))
                .doc("Show this documentation."));

    std::vector<std::string> args = parser.parse(argc, argv).apply().unreachedArgs();
    if (args.empty()) {
        mlog[FATAL] <<"incorrect usage; see --help\n";
        exit(1);
    }
    return args;
}

static std::vector<std::string>
toTokens(const std::string &name, const std::string &content) {
    Clexer::TokenStream tokens(name, Sawyer::Container::StaticBuffer<size_t, char>::instance(content.c_str(), content.size()));
    std::vector<std::string> retval;
    while (tokens[0].type() != Clexer::TOK_EOF) {
        retval.push_back(tokens.lexeme(tokens[0]));
        tokens.consume();
    }
    return retval;
}

int
main(int argc, char *argv[]) {
    Sawyer::initializeLibrary();
    mlog = Sawyer::Message::Facility("tool");
    Sawyer::Message::mfacilities.insertAndAdjust(mlog);

    std::vector<std::string> args = parseCommandLine(argc, argv);
    ASSERT_forbid(args.empty());

    std::vector<std::string> needle = toTokens("needle", args[0]);
    args.erase(args.begin());

    bool anyFound = false;
    BOOST_FOREACH (const std::string &fileName, args) {
        if (!boost::filesystem::exists(fileName)) {
            mlog[ERROR] <<fileName <<": no such file\n";
            continue;
        }
        if (boost::filesystem::file_size(fileName) == 0)
            continue;

        Clexer::TokenStream haystack(fileName);
        while (haystack[0].type() != Clexer::TOK_EOF) {
            bool doesMatch = true;
            for (size_t i=0; doesMatch && i<needle.size(); ++i)
                doesMatch = haystack.matches(haystack[i], needle[i].c_str());
            if (doesMatch) {
                haystack.emit(fileName, haystack[0], "found here");
                anyFound = true;
            }
            haystack.consume();
        }
    }
    
    return anyFound ? 0 : 1;
}

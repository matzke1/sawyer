#include <Sawyer/Clexer.h>
#include <Sawyer/CommandLine.h>
#include <Sawyer/DistinctList.h>
#include <Sawyer/Graph.h>
#include <Sawyer/GraphTraversal.h>
#include <Sawyer/Message.h>
#include <Sawyer/Set.h>

#include <boost/regex.hpp>

using namespace Sawyer::Message::Common;
using namespace Sawyer::Language;

static Sawyer::Message::Facility mlog;
std::vector<std::string> includeSearchDirs;
typedef Sawyer::Container::Map<std::string, std::string> IncludeMap;
typedef Sawyer::Container::Graph<std::string, Sawyer::Nothing, std::string> IncludeGraph;

static Sawyer::Container::DistinctList<std::string>
parseCommandLine(int argc, char *argv[]) {
    using namespace Sawyer::CommandLine;
    Parser p;
    p.purpose("search for tokens in source code");
    p.doc("Synopsis",
               "@prop{programName} [@v{switches}] @v{files}...");
    p.errorStream(mlog[FATAL]);

    p.with(Switch("help", 'h')
           .action(showHelpAndExit(0))
           .doc("Show this documentation."));

    p.with(Switch("include", 'I')
           .argument("directory", listParser(anyParser(includeSearchDirs), ":"))
           .explosiveLists(true)
           .whichValue(SAVE_ALL)
           .doc("Directories in which to search for include files."));

    Sawyer::Container::DistinctList<std::string> retval;
    BOOST_FOREACH (const std::string &arg, p.parse(argc, argv).apply().unreachedArgs())
        retval.pushBack(arg);
    return retval;
}

static std::string
findIncludeFile(const std::string &includeName, IncludeMap &includeMap) {
    if (includeName.empty() || includeName[0] == '/')
        return includeName;

    std::string found;
    if (includeMap.getOptional(includeName).assignTo(found))
        return found;

    BOOST_FOREACH (const std::string &dirName, includeSearchDirs) {
        boost::filesystem::path check = boost::filesystem::path(dirName) / includeName;
        if (boost::filesystem::exists(check)) {
            includeMap.insert(includeName, check.native());
            return check.native();
        }
    }

    includeMap.insert(includeName, includeName);
    return includeName;
}

int
main(int argc, char *argv[]) {
    Sawyer::initializeLibrary();
    mlog = Sawyer::Message::Facility("tool");
    Sawyer::Message::mfacilities.insertAndAdjust(mlog);

    Sawyer::Container::DistinctList<std::string> topFiles = parseCommandLine(argc, argv);
    Sawyer::Container::DistinctList<std::string> workList = topFiles;
    boost::regex includeRe("#\\s*include\\s*[\"<](.*?)[\">].*");
    IncludeMap includeMap;
    IncludeGraph includeGraph;
    Sawyer::Container::Set<std::string> missingFiles;

    while (!workList.isEmpty()) {
        // Did we already process this source file?
        std::string sourceFile = workList.popFront();
        IncludeGraph::VertexIterator sourceVertex = includeGraph.findVertexValue(sourceFile);
        if (sourceVertex != includeGraph.vertices().end() && sourceVertex->nOutEdges() > 0)
            continue;

        // Open the source file as a C/C++ token stream
        if (!boost::filesystem::exists(sourceFile)) {
            missingFiles.insert(sourceFile);
            continue;
        }
        Clexer::TokenStream tokens(sourceFile);
        tokens.skipPreprocessorTokens(false);

        // Process each include directive
        for (/*void*/; tokens[0].type() != Clexer::TOK_EOF; tokens.consume()) {
            std::string directive = tokens.lexeme(tokens[0]);
            boost::smatch matches;
            if (tokens[0].type() != Clexer::TOK_CPP || !boost::regex_match(directive, matches, includeRe))
                continue;
            std::string includeName = matches.str(1);
            std::string includeFile = findIncludeFile(includeName, includeMap);

            includeGraph.insertEdgeWithVertices(sourceFile, includeFile);
            workList.pushBack(includeFile);
        }
    }

    BOOST_FOREACH (const std::string &name, missingFiles.values())
        mlog[WARN] <<"cannot find file #include <" <<name <<">\n";

    // For each top-level file, list all things which it includes directly or indirectly.
    BOOST_FOREACH (const std::string &sourceFile, topFiles.items()) {
        std::cout <<sourceFile <<":\n";
        IncludeGraph::VertexIterator sourceVertex = includeGraph.findVertexValue(sourceFile);
        ASSERT_require(includeGraph.isValidVertex(sourceVertex));
        typedef Sawyer::Container::Algorithm::DepthFirstForwardVertexTraversal<IncludeGraph> Traversal;
        Sawyer::Container::Set<std::string> includeFiles;
        for (Traversal t(includeGraph, sourceVertex); t; ++t)
            includeFiles.insert(t->value());
        BOOST_FOREACH (const std::string &includeFile, includeFiles.values())
            std::cout <<"  " <<includeFile <<"\n";
    }
}

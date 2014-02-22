#include "CommandLine.h"

enum WhenColor { NEVER, AUTO, ALWAYS };

int main(int argc, char *argv[]) {

    using namespace Sawyer::CommandLine;
    
    SwitchGroup ss;

#if 1 /*DEBUGGING [Robb Matzke 2014-02-17]*/
    ss.insert(Switch("test", 't'));



#else

    // Minimal format for a switch: takes no arguments and stores true if the switch occurred on the command line at least
    // once, and has no documentation.
    ss.insert(Switch("verbose", 'v'));

    // A switch may have more than one name.
    ss.insert(Switch("gray", 'g')
              .longName("grey")
              .shortName('b'));

    // Most switches also have documentation. Documentation for the body of the switch using a simple markup language.
    // The rest of these examples don't use documentation just to keep things short and simple.
    ss.insert(Switch("all", 'a')
              .doc("Do not ignore entries starting with '.'"));
    
    // A switch can cause some action to immediately occur each time it is processed.  The showVersion() is an action
    // provided by Sawyer, but user actions are also allowed.
    ss.insert(Switch("version", 'V')
              .action(showVersion("1.2.3")));

    // A switch can have more than one action. They're processed in the order they're declared.  The
    // showHelp() and exitProgram() actions are predefined.  Note that the short name is two characters, which means that both
    // "-h" and "-?" are valid spellings (assuming the prefix is "-" when parsing).  The alternative is a user-defined action
    // that does two or more things, but that's more verbose (one goal of this API is that switch descriptions are short).
    ss.insert(Switch("help", 'h')
              .shortName('?')
              .action(showHelp(std::cout))
              .action(exitProgram(0)));

    // An argumentless switch can can be given a different value than true/false when it occurs.  For instance, this "--wide"
    // will set its value to the unsigned int 130 if it occurs on the command-line.
    ss.insert(Switch("wide")
              .defaultValue("130", unsignedIntegerParser()));

    // A switch can take an argument interpreted as an arbitrary string.  The argument can be separated from the
    // switch in various ways: as a single program argument: --committer=alice or -Calice; as two program
    // arguments: --committer alice or -C alice.  Controlled by settings for the parser, switch set, or switch.
    ValueParser::Ptr xp = anyParser();
    SwitchArgument xa("name", xp);
    ss.insert(Switch("committer", 'C')
              .argument(xa));

    // A switch can have more than one argument, as in
    //    --swap thing1 thing2
    // which is different than a switch that takes one argument that can be parsed as two values:
    //    --swap thing1,thing2   (we'll show this below)
    // its also possible to combine two or more different syntaxes for the same switch (also shown below).
    ss.insert(Switch("swap")
              .argument("first-item")
              .argument("second-item"));

    // A switch may require that its argument be parsable as an integer, unsigned integer, real number, etc.
    ss.insert(Switch("threshold")                       // no short names
              .argument("threshold", realNumberParser()));
    
    // A later switch occurrence may override an earlier occurrence of the same switch on the command line.
    ss.insert(Switch("editor", 'E')
              .argument("editor_command")
              .saveLast());                             // this is the default

    // A switch may occur more than once on the command line with all values saved
    ss.insert(Switch("author", 'A')
              .argument("name")
              .saveAll());                              // save all values when the switch repeats

    // A switch may be prohibited from occurring multiple times on the command line.
    ss.insert(Switch("password", 'P')
              .saveOne());                              // different than saveFirst (which ignores subsequent occurrences)

    // A switch may have multiple values separated by the specified regular expression (default is
    // "[,:;[:space:]][[:space:]]*|[[:space:]]+" (i.e., a comma, colon, or semicolon followed by optional white
    // space, or at least one white space.
    ss.insert(Switch("incdir", 'I')
              .doc("List of directories to search.")
              .argument("directories", listParser(anyParser())) // each switch value is a list of strings
              .saveAll());                               // and the switch can appear multiple times

    // A switch can be one of an enumerated list.  The elements of the list are strings, but they must be
    // parsable as if they actually appeared on the program command line.
    ss.insert(Switch("untracked-files", 'u')
              .argument("mode",
                        stringSetParser()->with("no")->with("normal")->with("all")));
    
    // A 2d coordinate can be treated as a pair of real numbers as in "--origin=5.0,4.5"
    ss.insert(Switch("origin")
              .argument("coordinate-pair",
                        listParser(realNumberParser(), ",")  // list of real numbers separated by exactly "," characters
                        ->exactly(2)));                 // and containing exactly two elements

    // If two switches are a Boolean pair (--pager/--no-pager, --fomit-frame-pointer/--fno-omit-fram-pointer,
    // --verbose/--quiet) you can do this:
    ss.insert(Switch("fomit-frame-pointer")
              .key("fomit-frame-pointer")               // this is the default for this switch
              .defaultValue("yes", booleanParser()));   // this is the default for this switch
    ss.insert(Switch("fno-omit-frame-pointer")
              .key("fomit-frame-pointer")               // use the same key as the "yes" version
              .defaultValue("no", booleanParser()));    // but cause a "no" to be registered

    // Two switches may be mutually exclusive
    // FIXME[Robb Matzke 2014-02-13]

    // A switch argument may be optional. The next item on the command-line must look like a switch.  E.g.,
    // "--color" is the same as "--color=always" if the next program argument looks like a switch.
    ss.insert(Switch("use-color")
              .argument("when",                         // argument name
                        (enumParser<WhenColor>()        // WhenColor must be declared at global scope
                         ->with("never", NEVER)         // possible values
                         ->with("auto", AUTO)
                         ->with("always", ALWAYS)),
                        "always"));                     // default value
    
    // User can supply his own parser.
    typedef boost::shared_ptr<class MyParser> MyParserPtr;
    struct MyParser: ValueParser {
        static MyParserPtr instance() { return MyParserPtr(new MyParser); } // or use a global myParser() factory function
        boost::any operator()(const char *s, const char **rest) {
            if (0==strncmp(s, ":module:", 8) && isalnum(s[8])) {
                *rest = s + strlen(s);
                return std::string(s);
            }
            return boost::any();                        // nothing to parse
        }
    };

    ss.insert(Switch("module", 'M')
              .argument("module-name", MyParser::instance()));

    // User can supply his own action.
    // FIXME[Robb Matzke 2014-02-14]

    // One switch can be an alias for another.  The other switch need not be defined in the same SwitchGroup as
    // long as the other switch's definition is available when parsing.
    // FIXME[Robb Matzke 2014-02-13]

#endif

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Parsing
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // construct a parser, add switch groups, and apply it to program arguments.
    ParserResult cmdline = Parser().with(ss)
                           // .skipUnknownSwitches()
                           // .skipNonSwitches()
                           .parse(argc, argv);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Query results
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<std::string> args;

    if (cmdline.have("test")) {
        std::cout <<"parsed test: string value=\"" <<cmdline.parsed("test", 0).string() <<"\"\n";
        std::cout <<"value as floating point: " <<cmdline.parsed("test", 0).asDouble() <<"\n";
        std::cout <<"value as string: " <<cmdline.parsed("test", 0).asString() <<"\n";
    }
    
    args = cmdline.skippedArgs();
    if (!args.empty()) {
        std::cout <<"these non-switch program arguments were skipped:";
        for (size_t i=0; i<args.size(); ++i)
            std::cout <<" \"" <<args[i] <<"\"";
        std::cout <<"\n";
    }

    args = cmdline.remainingArgs();
    if (!args.empty()) {
        std::cout <<"these program arguments still remain unparsed:";
        for (size_t i=0; i<args.size(); ++i)
            std::cout <<" \"" <<args[i] <<"\"";
        std::cout <<"\n";
    }

    args = cmdline.unparsedArgs();
    if (!args.empty()) {
        std::cout <<"these are all the arguments remaining:";
        for (size_t i=0; i<args.size(); ++i)
            std::cout <<" \"" <<args[i] <<"\"";
        std::cout <<"\n";
    }

    return 0;
}



//    // FIXME[Robb Matzke 2014-02-13] show some things we can do with the results
//
//
//                   .longPrefix("--", "-")               // prefixes for introducing long-name switches (this is the default)
//                   .shortPrefix("-")                    // prefixes for introducing single-letter switches (this is default)
//                   .longValueSeparator("=", " ")        // what separates a switch name from its value (this is the default)
//                   .shortMayNestle(true)                // "-ab" is like "-a -b" (this is the default)
//                   .terminator("--")                    // stop processing when we hit "--" (this is the default)
//                   .include("@");                       // "@foobar" will insert switches from file "foobar"
//
//
//
//    SwitchGroup ss = SwitchGroup()

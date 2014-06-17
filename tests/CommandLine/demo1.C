#include <sawyer/CommandLine.h>
#include <iostream>

using namespace Sawyer::CommandLine;
    
enum WhenColor { NEVER, AUTO, ALWAYS };

void showRawManPage(const ParserResult &parserResult) {
    std::cout <<parserResult.parser().manDocumentation();
    exit(0);
}

void showRawPod(const ParserResult &parserResult) {
    std::cout <<parserResult.parser().podDocumentation();
    exit(0);
}

void showRawMarkup(const ParserResult &parserResult) {
    std::cout <<parserResult.parser().documentationMarkup();
    exit(0);
}

int main(int argc, char *argv[]) {

    // The command-line parsing library API enables a message chaining paradigm because each property-setting method returns a
    // reference to the object whose property was set.  You can either use this paradigm or ignore it; the examples here are a
    // mixture: we use chaining when declaring each individual switch, but we use the more traditional C++ method calls for
    // adding switches to switch groups.
    //
    // Methods that set or retrieve a property (property accessors) are nouns or adjectives; methods that perform a more
    // complex action are verbs.  Property accessors are overloaded so that the no-argument version returns the property's
    // current value and the others modify the property.  The ordering of property modifiers is generally not important since
    // the each modify only one property; the ordering of the verb methods probably is significant even in relation to the
    // property modifiers.


    // A SwitchGroup is used to hold zero or more related switches. A program like "git", which has the synopsis "git
    // [SWITCHES] COMMAND [ARGS]" probably has one group of top-level switches and a group of command-specific switches for
    // each command.
    SwitchGroup ss;

    // Minimal format for a switch: takes no arguments and stores true if the switch occurred on the command line at least
    // once.
    ss.insert(Switch("verbose", 'v'));

    // A switch may have more than one name.
    ss.insert(Switch("gray", 'g')
              .longName("grey")
              .shortName('b'));

    // Most switches also have documentation using a simple markup language.  The rest of these examples don't use
    // documentation in order to keep things short and simple.
    ss.insert(Switch("all", 'a')
              .doc("Do not ignore entries starting with '.'"));

    // Switches can override or augment the set of prefix strings.  This switch will be accepted as "-rose:log" instead
    // of the normal prefixes. (Using this is discouraged since it can be jarring for users on systems that don't normally use
    // that prefix. E.g., POSIX uses "+", Windows uses "/").
    ss.insert(Switch("log")
              .resetLongPrefixes("-rose:"));
    
    // A switch can register some action to be called after the command-line is parsed.  The showVersion() and showHelp() are
    // two actions provided by the library.  Actions are called when a parser result is applied (ParserResult::apply), which
    // means that parsing itself is free of side effects and allows users to ask whether a command-line is valid without
    // actually causing any configuration changes.
    ss.insert(Switch("version", 'V')
              .action(showVersion("1.2.3")));
    ss.insert(Switch("help", 'h')
              .shortName('?')
              .action(showHelp()));

    // Here's a user defined action the shortest way possible.  User's can also use the boost shared pointer paradigm used
    // throughout Sawyer itself, but this way is a little shorter and more STL-like. The functor should expect one argument, a
    // pointer to the parser which is calling it.
    ss.insert(Switch("raw-man")
              .action(userAction(showRawManPage))
              .doc("Prints the raw nroff man page to standard output for debugging."));

    ss.insert(Switch("raw-pod")
              .action(userAction(showRawPod))
              .doc("Prints the raw Perl POD source to standard output for debugging."));

    ss.insert(Switch("raw-markup")
              .action(userAction(showRawMarkup))
              .doc("Prints the raw markup to standard output for debugging."));

    // An argumentless switch can can be given a different value than true when it occurs.  For instance, this switch will
    // parse the mathematical integer "130" (can be any C/C++ format) and store it as an int as if the string "130" appeared on
    // the command-line.
    ss.insert(Switch("wide")
              .intrinsicValue("130", integerParser()));

    // Some parsers (like the numeric parsers) can also store their value as some other C++ type if you give that type name as
    // a template argument.  This example is like the "wide" switch above, but stores the result as an unsigned short.  If the
    // parsed value doesn't fit in an unsigned short (e.g., it was negative or larger than 32,767) you'll get an error to that
    // effect (but see parser skipping capabilities below).
    ss.insert(Switch("shortwide")
              .intrinsicValue("130", integerParser<unsigned short>()));

    // You can also tell the parser to store its value somewhere by giving an argument to the parser (most, but not all parsers
    // can do this).  If you give a storage locaton then the template parameter is not necessary (it's inferred from the
    // storage location's type).  This is often the way that boolean switches are used:
    unsigned short savedWide = 0;                       // the default value if the switch doesn't occur
    ss.insert(Switch("savedwide")
              .intrinsicValue("130", integerParser(savedWide)));

    // A switch can take an argument which can be separated from the switch in various ways: as a single program argument:
    // --committer=alice or -Calice; as two program arguments: --committer alice or -C alice (controlled by settings for the
    // parser, switch group, or switch).  The argument name, "name" in this case, is only used in error messages and
    // documentation and can be pretty much anything you want (but probably something that succinctly describes the argument).
    ss.insert(Switch("committer", 'C')
              .argument("name"));

    // A switch can control what separates the switch name from the value.  This switch accepts the usual "=" and " " (which
    // means the switch value is in the next program argument) but also accepts ":=".  It usually doesn't matter what order the
    // settings occur since they're all just properties that are being adjusted.
    ss.insert(Switch("verifier")
              .valueSeparator(":=")
              .argument("name"));

    // A switch can have more than one argument, as in
    //    --swap thing1 thing2
    // which is different than a switch that takes one argument that can be parsed as two individual values:
    //    --swap thing1,thing2   (we'll show this below)
    // its also possible to combine two or more different syntaxes for the same switch (also shown below).
    ss.insert(Switch("swap")
              .argument("first-item")
              .argument("second-item"));

    // A switch may require that its argument be parsable as an integer, unsigned integer, real number, etc. When a parser is
    // specified it not only allows some rudimentary error checking, but it also converts and stores the value as any C++ type
    // (in addition to std::string). It also allows nestling of single-letter switches with values, as in this example, which
    // allows "-t3.14v" -- same as "-t 3.14 -v" (nestling can be disabled with a parser property if you don't like it).  The
    // parser for the "argument" property works the same as what we saw above for the "intrinsicValue" property--it can specify
    // a storage location and type, just a storage type, or use a default storage type.
    ss.insert(Switch("threshold", 't')
              .argument("threshold", realNumberParser()));

    // As with intrinsicValue, a parsed switch value can be stored in a program variable and you get all the same range
    // checking, etc.  This example shows that a parser that accepts any mathematical integer can store values as unsigned (or
    // vice versa using nonNegativeIntegerParser and a signed type), and that if you enter a number that's negative or too
    // large you'll get an error.
    unsigned short testRangeExceptions;
    ss.insert(Switch("unsigned-short")
              .argument("value", integerParser(testRangeExceptions)));

    // If a switch occurs more than once on the command line, all occurrances will be parsed but only the last value is
    // preserved in the parser results (that's usually what one wants for most switches). This behavior can be changed so that
    // only the first value is saved, or it's an error for the switch to occur more than once (or ever), or all values are
    // saved.  This example saves all the values.
    ss.insert(Switch("editor", 'E')
              .argument("editor_command")
              .whichValue(SAVE_ALL));                   // the default is SAVE_LAST

    // Parsed values can also be appended to the end of STL vectors by providing a vector L-value as the parser storage
    // location.  This doesn't actually store every value parsed into the vector, only those values that are part of the final
    // ParserResult when the ParserResult::apply() method is invoked.
    std::vector<double> multipliers;
    ss.insert(Switch("multiplier")
              .argument("multiplicand", realNumberParser(multipliers))
              .whichValue(SAVE_ALL));

    // Another switch can use the same storage vector.  This time we accept a list of multipliers in a single switch
    // occurrence, like "--multipliers=1,2,3"
    ss.insert(Switch("multipliers")
              .argument("multiplicand-list", listParser(realNumberParser(multipliers))));


    // It's also possible that subsequent occurrences of a switch modify the value stored by the previous occurrence. A classic
    // example is a debug switch where each occurrence of the switch increments the debug level, storing only a single integer
    // rather than a list of integers.  One does that by registering a value augmenter and setting the whichProperty to
    // SAVE_AUGMENTED.  The augmenter needs to be aware of the data type being saved.  Users can create the own
    // augmenters which are called with a list of all previous values and the new value(s), and return the value(s) to be
    // saved.
    short debugLevel = 0;
    ss.insert(Switch("debug", 'd')
              .intrinsicValue("1", integerParser(debugLevel))
              .valueAugmenter(sum<short>())               // provided by the library to sum arguments
              .whichValue(SAVE_AUGMENTED));               // ensures the augmenter is called
    
    // A switch may be prohibited from occurring multiple times on the command line.
    ss.insert(Switch("password", 'P')
              .whichValue(SAVE_ONE));                   // different than SAVE_FIRST which ignores subsequent occurrences

    // A switch can even be prohibited from occuring at all on the command line (it still appears in the documentation if it
    // isn't also hidden).//FIXME[Robb Matzke 2014-02-24]: provide an error message as an argument
    ss.insert(Switch("author", 'A')
              .argument("name")
              .whichValue(SAVE_NONE));

    // A switch may have multiple values separated by the specified regular expression (default is a comma, colon, or semicolon
    // followed by optional white space).  If the listParser() is used, which returns a ListParser::ValueList container, the
    // container can be optionally exploded so "-I foo:bar" is (almost) indistinguishable from "-I foo -I bar" (almost, because
    // the information about where each value came from on the command line is inaccurate).
    ss.insert(Switch("incdir", 'I')
              .doc("List of directories to search.")
              .argument("directories", listParser(anyParser())) // each switch value is a list of strings
              .explosiveLists(true)
              .whichValue(SAVE_ALL));                   // and the switch can appear multiple times

    // A switch can be one of an enumerated list.
    ss.insert(Switch("untracked-files", 'u')
              .argument("no|normal|all",
                        stringSetParser()->with("no")->with("normal")->with("all")));

    // It is possible for the same switch to have multiple syntaxes. They're tried in the order they're declared. Here's a
    // switch that accepts an integer or the string "auto".
#if 1    // FIXME[Robb Matzke 2014-02-24]: make this easier and error messages better
    ss.insert(Switch("width", 'w')
              .argument("nchars", stringSetParser()->with("auto")));
    ss.insert(Switch("width", 'w')
              .argument("nchars", integerParser<int>()));
#else
    ss.insert(Switch("width", 'w')
              .argument("nchars",
                        .alternativeParsers(stringSetParser()->with("auto"),
                                            integerParser())));
#endif
    
    
    // A 2d coordinate can be treated as a pair of real numbers as in "--origin=5.0,4.5"
    ss.insert(Switch("origin")
              .argument("coordinate-pair",
                        listParser(realNumberParser<float>(), ",")  // list of real numbers separated by exactly "," characters
                        ->exactly(2)));                 // and containing exactly two elements

    // A list can even have different types for the elements.  Here's a switch that takes a list containing an integer and a
    // string.  The string is optional because this list can have 1 or 2 items:
    ss.insert(Switch("increment")
              .argument("delta,when",
                        listParser(integerParser<int>())
                        ->nextMember(stringSetParser()->with("pre")->with("post"))
                        ->limit(1, 2)));                // allow only one or two items in the list

    // If two switches are a Boolean pair (--pager/--no-pager, --fomit-frame-pointer/--fno-omit-fram-pointer,
    // --verbose/--quiet) you can give the switches two different intrinsic values and tell them to store their value in the
    // same place (by virtue of both using the same storage key).  The Boolean parser accepts a variety of strings, and if it's
    // not flexible enough you can use the stringSetParser.
    // FIXME[Robb Matzke 2014-02-28]: could we make this easier?
    ss.insert(Switch("fomit-frame-pointer")
              .key("fomit-frame-pointer")               // this is the default for this switch
              .intrinsicValue("yes", booleanParser<bool>())); // this is the default for this switch
    ss.insert(Switch("fno-omit-frame-pointer")
              .key("fomit-frame-pointer")               // use the same key as the "yes" version
              .intrinsicValue("no", booleanParser<bool>()));  // but cause a "no" to be registered

    // Two switches can be mutually exclusive using the storeOne() attribute in conjuction with a single key.  The various
    // storage settings (one, first, last, all, etc.) that we saw above are applied on a per-key basis rather than a per-switch
    // basis.  This also causes an error if any single switch occurs more than once ("--quick --quick"); if you want more
    // complex logic you can do that by either writing a value augmenter that fails if the previous and current values are
    // different, or you can simply look at the ParseResult after parsing is finished and throw your own error.
    // FIXME[Robb Matzke 2014-02-28]: error mesage could be better since it now only mentions the offending switch
    ss.insert(Switch("go-fast", 'F')
              .longName("quick")
              .longName("rabbit")
              .key("speed")
              .intrinsicValue("75", integerParser<int>())
              .whichValue(SAVE_ONE));
    ss.insert(Switch("go-slow", 'S')
              .longName("lazy")
              .longName("turtle")
              .key("speed")
              .intrinsicValue("15", integerParser<int>())
              .whichValue(SAVE_ONE));

    // A switch argument may be optional. E.g., "--color" is the same as "--color=always" if the next program argument doesn't
    // look like a valid argument for the switch.
    // FIXME[Robb Matzke 2014-02-24]: the error message could be better if you misspell the argument and the switch argument is
    // in the next program argument (--use-color no) because the "no" then looks like a positional program argument rather than
    // a syntactically incorrect switch argument.
    ss.insert(Switch("use-color")
              .argument("when",                         // argument name
                        (enumParser<WhenColor>()        // WhenColor must be declared at global scope
                         ->with("never", NEVER)         // possible values
                         ->with("auto", AUTO)
                         ->with("always", ALWAYS)),
                        "always"));                     // default value
    
    // User can supply his own parser.  Here's a parser that requires an argument to start with the string ":module:".  Of
    // course, this silly example can probably be delayed until after parsing, but when the syntax is for an optional switch
    // argument you sometimes need to do it during parsing.  The interface shown has an API like strtod() et al. There's also
    // an interface for parsing values that span multiple program arguments.
    typedef Sawyer::SharedPointer<class MyParser> MyParserPtr;
    class MyParser: public ValueParser {
    public:
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

    // When one switch is a prefix of the other, longer prefixes are matched first.
    ss.insert(Switch("pre").argument("arg1"));
    ss.insert(Switch("prefix").argument("arg1"));

    // One switch can be an alias for another.  The other switch need not be defined in the same SwitchGroup as
    // long as the other switch's definition is available when parsing.
    // FIXME[Robb Matzke 2014-02-13]


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Parsing
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // construct a parser, add switch groups, and apply it to program arguments.  Parsers also have a lot of documentation
    // properties which are demonstrated elsewhere.
    ParserResult cmdline = Parser()

                           // as many switch groups as you like (but at least one)
                           .with(ss)

                           // You can cause the parser to skip over some things that would otherwise be errors.
                           // .skipUnknownSwitches()
                           // .skipNonSwitches()

                           // You can change what pattern causes a file to be read and expanded into the command-line. This
                           // example changes it from "@foo" to "--file=foo".
                           .resetInclusionPrefixes("--file=")

                           // Here's where the parsing actually happens and a ParserResult is returned. Notice that unlike
                           // property settings, this method is a verb.  There are a number of overloaded parse methods.
                           .parse(argc, argv)

                           // Finally, if you're not just parsing a command line to get error messages or to remove switches
                           // that the parser recognizes, you'll want to call apply() so that parsed switch values are written
                           // into program variables (if that feature is used).
                           .apply();


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Query results
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    std::vector<std::string> args;

    // Values saved for various switches
    std::cout <<"savedWide = " <<savedWide <<"\n";
    std::cout <<"testRangeExceptions = " <<testRangeExceptions <<"\n";
    std::cout <<"multipers = {";
    for (size_t i=0; i<multipliers.size(); ++i)
        std::cout <<" " <<multipliers[i];
    std::cout <<" }\n";
    std::cout <<"debugLevel = " <<debugLevel <<"\n";

    // Did we see an occurrence of the "--test" switch?  If so, get its value a few different ways.   The ParserResult also
    // stores quite a bit of information about where the value came from: command-line location, command-line string, switch
    // key, switch preferred name, matched switch string, switch location, etc.
    if (cmdline.have("test")) {
        std::cout <<"parsed test: string value=\"" <<cmdline.parsed("test", 0).string() <<"\"\n"; // command-line text
        std::cout <<"value as floating point: " <<cmdline.parsed("test", 0).asDouble() <<"\n";
        std::cout <<"value as string: " <<cmdline.parsed("test", 0).asString() <<"\n"; // the double cast to a string
    }

    // Some queries that return parts of a command line...
    args = cmdline.allArgs();
    std::cout <<"these are all the program arguments:";
    for (size_t i=0; i<args.size(); ++i)
        std::cout <<" \"" <<args[i] <<"\"";
    std::cout <<"\n";
    
    args = cmdline.parsedArgs();
    std::cout <<"these program arguments were parsed:";
    for (size_t i=0; i<args.size(); ++i)
        std::cout <<" \"" <<args[i] <<"\"";
    std::cout <<"\n";
    
    args = cmdline.skippedArgs();
    std::cout <<"these program arguments were skipped over:";
    for (size_t i=0; i<args.size(); ++i)
        std::cout <<" \"" <<args[i] <<"\"";
    std::cout <<"\n";

    args = cmdline.unreachedArgs();
    std::cout <<"these program arguments were never reached:";
    for (size_t i=0; i<args.size(); ++i)
        std::cout <<" \"" <<args[i] <<"\"";
    std::cout <<"\n";

    // This query is how you would use the parser to remove the switches we recognize so that the rest of the command line can
    // be passed to some other parser.
    args = cmdline.unparsedArgs();
    std::cout <<"these are all the arguments not parsed:";
    for (size_t i=0; i<args.size(); ++i)
        std::cout <<" \"" <<args[i] <<"\"";
    std::cout <<"\n";

    // This example is a good demonstration of how chaining can be used to perform a rather complex action in a single
    // statement.  It removes "--width=N" or ("--width N" as two args) from the command line if N is a valid short int, and
    // sets "width" to N.  And it does it all without a single template parameter.
    short width = 0;
    args = Parser()
           .with(Switch("width")
                 .argument("n", integerParser(width)))
           .skipUnknownSwitches()
           .skipNonSwitches()
           .parse(argc, argv)
           .apply()
           .unparsedArgs(true);
    std::cout <<"after removal of --width (width=" <<width <<"):";
    for (size_t i=0; i<args.size(); ++i)
        std::cout <<" \"" <<args[i] <<"\"";
    std::cout <<"\n";

    return 0;
}

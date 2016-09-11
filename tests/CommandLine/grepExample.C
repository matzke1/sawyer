#include <Sawyer/CommandLine.h>
using namespace Sawyer::CommandLine;

static const std::string VERSION_STRING = "GNU grep 2.6.3";
enum Matcher { MATCHER_EXTENDED, MATCHER_STRINGS, MATCHER_BASIC, MATCHER_PERL };
enum ColorWhen { COLOR_NEVER, COLOR_ALWAYS, COLOR_AUTO };
enum BinaryFile { BIN_TEXT, BIN_BINARY, BIN_SKIP };
enum Action { ACTION_READ, ACTION_SKIP, ACTION_RECURSE };

struct Options {
    Matcher matcher;
    std::string pattern;
    std::string patternFile;
    bool ignoreCase, invert, wordRegexp, lineRegexp, count;
    ColorWhen colorWhen;
    bool filesWithoutMatch, filesWithMatches, onlyMatching, quiet, noMessages;
    int maxCount;
    bool byteOffset, withFileName;
    std::string label;
    bool lineNumbers, initialTab, unixByteOffsets, nullAfterName;
    int afterContext, beforeContext, contextLines;
    BinaryFile binaryFile;
    Action deviceAction, directoryAction;
    std::string excludeGlob, excludeFromFile, includeGlob;
    bool lineBuffered, useMmap, openAsBinary, nulTerminatedLines;
    Options()
        : matcher(MATCHER_BASIC), ignoreCase(false), invert(false), wordRegexp(false), lineRegexp(false), count(false),
          colorWhen(COLOR_AUTO), filesWithoutMatch(false), filesWithMatches(false), onlyMatching(false), quiet(false),
          noMessages(false), maxCount(-1), byteOffset(false), withFileName(false), lineNumbers(false), initialTab(false),
          unixByteOffsets(false), nullAfterName(false), afterContext(-1), beforeContext(-1), contextLines(-1),
          binaryFile(BIN_BINARY), deviceAction(ACTION_READ), directoryAction(ACTION_READ), lineBuffered(false),
          useMmap(false), openAsBinary(false), nulTerminatedLines(false)
        {}
};
    

int main(int argc, char *argv[]) {

    // Command-line parsing will "push" parsed values into members of this struct when ParserResult::apply is called.
    // Alternatively, we could have used a "pull" paradigm in which we query the ParserResult to obtain the values. In
    // fact, nothing prevents us from doing both, except if we use a pull paradigm we would probably want to set some
    // of the Switch::key properties so related switches (like -E, -F, -G, -P) all store their result in the same place
    // within the ParserResult.
    Options opt;

    // We split the switches into individual groups only to make the documentation better.  The name supplied to the
    // switch group constructor is the subsection under which the documentation for those switches appears.
    SwitchGroup generic("Generic Program Information");
    generic.switchOrder(INSERTION_ORDER);
    generic.doc("This paragraph is attached to a switch group and appears before the switches that this "
                "group contains.  It may be as many paragraphs as you like and may contain markup like for the "
                "individual switches.\n\n"
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec a diam lectus. Sed sit amet ipsum "
                "mauris. Maecenas congue ligula ac quam viverra nec consectetur ante hendrerit. Donec et mollis dolor. "
                "Praesent et diam eget libero egestas mattis sit amet vitae augue. Nam tincidunt congue enim, ut porta "
                "lorem lacinia consectetur. Donec ut libero sed arcu vehicula ultricies a non tortor. Lorem ipsum "
                "dolor sit amet, consectetur adipiscing elit. Aenean ut gravida lorem. Ut turpis felis, pulvinar a "
                "semper sed, adipiscing id dolor. Pellentesque auctor nisi id magna consequat sagittis. Curabitur "
                "dapibus enim sit amet elit pharetra tincidunt feugiat nisl imperdiet. Ut convallis libero in urna "
                "ultrices accumsan. Donec sed odio eros. Donec viverra mi quis quam pulvinar at malesuada arcu rhoncus. "
                "Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. In rutrum "
                "accumsan ultricies. Mauris vitae nisi at sem facilisis semper ac in est.");
    generic.insert(Switch("help")
                   .action(showHelpAndExit(0))
                   .doc("Print a usage message briefly summarizing these command-line options and the bug-reporting "
                        "address, then exit."));
    generic.insert(Switch("version", 'V')
                   .action(showVersion(VERSION_STRING))
                   .doc("Print the version number of @prop{programName} to the standard output stream. This version "
                        "number should be included in all bug reports (see below)."));
    generic.insert(Switch("log")
                   .action(configureDiagnostics("log", Sawyer::Message::mfacilities))
                   .argument("config")
                   .whichValue(SAVE_ALL)
                   .doc("Controls Sawyer diagnostics.  See Sawyer::Message::Facilities::control for details."));

    SwitchGroup matcher("Matcher Selection");
    matcher.switchOrder(INSERTION_ORDER);
    matcher.insert(Switch("extended-regexp", 'E')
                   .intrinsicValue(MATCHER_EXTENDED, opt.matcher)
                   .doc("Interpret @v{pattern} as an extended regular expression (ERE, see below). (@s{E} is specified "
                        "by POSIX.)"));
    matcher.insert(Switch("fixed-strings", 'F')
                   .intrinsicValue(MATCHER_STRINGS, opt.matcher)
                   .doc("Interpret @v{pattern} as a list of fixed strings, separated by newlines, any of which is to "
                        "be matched. (@s{F} is specified by POSIX.)"));
    matcher.insert(Switch("basic-regexp", 'G')
                   .intrinsicValue(MATCHER_BASIC, opt.matcher)
                   .doc("Interpret @v{pattern} as a basic regular expression (BRE, see below).  This is the default."));
    matcher.insert(Switch("perl-regexp", 'P')
                   .intrinsicValue(MATCHER_PERL, opt.matcher)
                   .doc("Interpret @v{pattern} as a Perl regular expression.  This is highly experimental and "
                        "@prop{programName} @s{P} may warn of unimplemented features."));

    SwitchGroup control("Matching Control");
    control.switchOrder(INSERTION_ORDER);
    control.insert(Switch("regexp", 'e')
                   .argument("pattern", anyParser(opt.pattern))
                   .doc("Use @v{pattern} as the pattern.  This can be used to specify multiple search patterns, or to "
                        "protect a pattern beginning with a hyphen (\"-\"). (@s{e} is specified by POSIX.)"));
    control.insert(Switch("file", 'f')
                   .argument("file", anyParser(opt.patternFile))
                   .doc("Obtain patterns from @v{file}, one per line.  The empty file contains zero patterns, and "
                        "therefore matches nothing. (@s{f} is specified by POSIX.)"));
    control.insert(Switch("ignore-case", 'i')
                   .intrinsicValue(true, opt.ignoreCase)
                   .doc("Ignore case distinctions in both the @v{pattern} and the input files. (@s{i} is specified "
                        "by POSIX.)"));
    control.insert(Switch("invert-match", 'v')
                   .intrinsicValue(true, opt.invert)
                   .doc("Invert the sense of matching, to select non-matching lines. (@s{v} is specified by POSIX.)"));
    control.insert(Switch("word-regexp", 'w')
                   .intrinsicValue(true, opt.wordRegexp)
                   .doc("Select only those lines containing matches that form whole words.  The test is that the "
                        "matching substring must either be at the beginning of the line, or preceded by a non-word "
                        "constituent character.  Similarly, it must be either at the end of the line or followed by "
                        "a non-word constituent character.  Word-constituent characters are letters, digits, and "
                        "the underscore."));
    control.insert(Switch("line-regexp", 'x')
                   .intrinsicValue(true, opt.lineRegexp)
                   .doc("Select only those matches that exactly match the whole line. (@s{x} is specified by POSIX.)"));
    control.insert(Switch("", 'y')
                   .whichValue(SAVE_NONE)
                   .doc("Obsolete synonym for @s{i}."));

    SwitchGroup output("General Output Control");
    output.nameSpace("output");
    output.switchOrder(INSERTION_ORDER);
    output.insert(Switch("count", 'c')
                  .intrinsicValue(true, opt.count)
                  .doc("Suppress normal output; instead print a count of matching lines for each input file.  With "
                       "the @s{v}, @s{invert-match} option (see below), count non-matching lines. (@s{c} is specified "
                       "by POSIX.)"));
    output.insert(Switch("color")
                  .longName("colour")
                  .argument("when",
                            enumParser(opt.colorWhen)
                            ->with("never", COLOR_NEVER)
                            ->with("always", COLOR_ALWAYS)
                            ->with("auto", COLOR_AUTO),
                            "auto")
                  .doc("Surround the matched (non-empty) strings, matching lines, context lines, file names, line "
                       "numbers, byte offsets, and separators (for fields and groups of context lines) with escape "
                       "sequences to display them in color on the terminal.  The colors are defined by the "
                       "environment variable \"GREP_COLORS\".  The deprecated environment variable \"GREP_COLOR\" "
                       "is still supported, but its setting does not have priority. @v{when} is \"never\", \"always\", "
                       "or \"auto\"."));
    output.insert(Switch("files-without-match", 'L')
                  .intrinsicValue(true, opt.filesWithoutMatch)
                  .doc("Suppress normal output; instead print the name of each input file from which no output "
                       "would normally have been printed.  The scanning will stop on the first match."));
    output.insert(Switch("files-with-matches", 'l')
                  .intrinsicValue(true, opt.filesWithMatches)
                  .doc("Suppress normal output; instead print the name of each input file from which output would "
                       "normally have been printed.  The scanning will stop on the first match. (@s{l} is specified "
                       "by POSIX.)"));
    output.insert(Switch("max-count", 'm')
                  .argument("num", nonNegativeIntegerParser(opt.maxCount))
                  .doc("Stop reading a file after @v{num} matching lines.  If the input is standard input from a "
                       "regular file, and @v{num} matching lines are output, @prop{programName} ensures that the "
                       "standard input is positioned to just after the last matching line before exiting, "
                       "regardless of the presence of trailing context lines.  This enables a calling process "
                       "to resume a search.  When @prop{programName} stops after @v{num} matching lines, it "
                       "outputs any trailing context lines.  When the @s{c} or @s{count} options is also used, "
                       "@prop{programName} does not output a count greater than @v{num}.  When the @s{v} or "
                       "@s{invert-match} option is also used, @prop{programName} stops after outputting @v{num} "
                       "non-matching lines."));
    output.insert(Switch("only-matching", 'o')
                  .intrinsicValue(true, opt.onlyMatching)
                  .doc("Print only the matched (non-empty) parts of a matching line, with each such part on a "
                       "separate output line."));
    output.insert(Switch("quiet", 'q')
                  .longName("silent")
                  .intrinsicValue(true, opt.quiet)
                  .doc("Quiet; do not write anything to standard output.  Exit immediately with zero status "
                       "if any match is found, even if an error was detected.  Also see the @s{s} or "
                       "@s{no-messages} option. (@s{q} is specified by POSIX.)"));
    output.insert(Switch("no-messages", 's')
                  .intrinsicValue(true, opt.noMessages)
                  .doc("Suppress error messages about nonexistent or unreadable files.  Portability note: "
                       "unlike GNU grep, 7th Edition Unix grep did not conform to POSIX, because it lacked "
                       "@s{q} and its @s{s} option behaved like GNU grep's @s{q} option.  USG-style grep "
                       "also lacked @s{q} but its @s{s} option behaved like GNU grep.  Portable shell "
                       "scripts should avoid both @s{q} and @s{s} and should redirect standard and error "
                       "output to /dev/null instead.  (@s{s} is specified by POSIX.)"));

    SwitchGroup prefix("Output Line Prefix Control");
    prefix.switchOrder(INSERTION_ORDER);
    prefix.insert(Switch("byte-offset", 'b')
                  .intrinsicValue(true, opt.byteOffset)
                  .doc("Print the 0-based byte offset within the input file before each line of output. "
                       "If @s{o} (@s{only-matching}) is specified, print the offset of the matching part "
                       "itself."));
    prefix.insert(Switch("with-filename", 'H')
                  .intrinsicValue(true, opt.withFileName)
                  .doc("Print the file name for each match.  This is the default when there is more than one "
                       "file to search."));
    prefix.insert(Switch("no-filename", 'h')
                  .intrinsicValue(false, opt.withFileName)
                  .doc("Suppress the prefixing of file names on output.  This is the default when there is only "
                       "one file (or only standard input) to search."));
    prefix.insert(Switch("label")
                  .argument("label", anyParser(opt.label))
                  .doc("Display input actually coming from standard input as input coming from file @v{label}. "
                       "This is especially useful when implementing tools like zgep, e.g., \"gzip -cd foo.gz | "
                       "grep --label=foo -H something\".  See also the @s{H} option."));
    prefix.insert(Switch("line-number", 'n')
                  .intrinsicValue(true, opt.lineNumbers)
                  .doc("Prefix each line of output with the 1-based line number within its input file. (@s{n} "
                       "is specified by POSIX.)"));
    prefix.insert(Switch("initial-tab", 'T')
                  .intrinsicValue(true, opt.initialTab)
                  .doc("Make sure that the first character of actual line content lies on a tab stop, so that "
                       "the alignment of tabs looks normal.  This is useful with options that prefix their "
                       "output to the actual content: @s{H}, @s{n}, and @s{b}.  In order to improve the "
                       "probability that lines from a single file will all start at the same column, this "
                       "also causes the line number and byte offset (if present) to be printed in a minimum "
                       "size field width."));
    prefix.insert(Switch("unix-byte-offsets", 'u')
                  .intrinsicValue(true, opt.unixByteOffsets)
                  .doc("Report Unix-style byte offsets.  This switch causes @prop{programName} to report byte "
                       "offsets as if the file were a Unix-style text file, i.e., with CR characters stripped "
                       "off.  This will produce results identical to running @prop{programName} on a Unix "
                       "machine.  This option has no effect unless @s{b} option is also used;  it has no "
                       "effect on platforms other than MS-DOS and MS-Windows."));
    prefix.insert(Switch("null", 'Z')
                  .intrinsicValue(true, opt.nullAfterName)
                  .doc("Output a zero byte (the ASCII NUL character) instead of the character that normally "
                       "follows a file name.  For example, \"grep -lZ\" outputs a zero byte after each file "
                       "name instead of the usual newline.  This option makes the output unambiguous, even in "
                       "the presence of file name containing unusual characters like newlines.  This option "
                       "can be used with commands like \"find -print0\", \"perl -0\", \"sort -z\", and \"xargs "
                       "-0\" to process arbitrary file names, even those that contain newline characters."));

    SwitchGroup context("Context Line Control");
    context.switchOrder(INSERTION_ORDER);
    context.insert(Switch("after-context", 'A')
                   .argument("num", nonNegativeIntegerParser(opt.afterContext))
                   .doc("Print @v{num} lines of trailing context after matching lines.  Places a line containing "
                        "a group separator (\"--\") between contiguous groups of matches.  With the @s{o} or "
                        "@s{only-matching} option, this has no effect and a warning is given."));
    context.insert(Switch("before-context", 'B')
                   .argument("num", nonNegativeIntegerParser(opt.beforeContext))
                   .doc("Print @v{num} lines of leading context before matching lines.  Places a line containing "
                        "a group separator (\"--\") between contigous groups of matches.  With the @s{o} or "
                        "@s{only-matching} option, this has no effect and a warning is given."));
    context.insert(Switch("context", 'C')               // FIXME[Robb Matzke 2014-03-15]: allow "-NUM"
                   .argument("num", nonNegativeIntegerParser(opt.contextLines))
                   .doc("Print @v{num} lines of output context.  Places a line containing a group separator "
                        "(\"--\") between contigous groups of matches.  With the @s{o} or @s{only-matching} "
                        "option, this has no effect and a warning is given."));

    SwitchGroup selection("File and Directory Selection");
    selection.switchOrder(INSERTION_ORDER);
    selection.insert(Switch("text", 'a')
                     .intrinsicValue(BIN_TEXT, opt.binaryFile)
                     .doc("Process a binary file as if it were text; this is the equivalent to the "
                          "\"@s{binary-files}=text\" option."));
    selection.insert(Switch("binary-files")
                     .argument("type",
                               enumParser(opt.binaryFile)
                               ->with("binary", BIN_BINARY)
                               ->with("without-match", BIN_SKIP)
                               ->with("text", BIN_TEXT))
                     .doc("If the first few bytes of a file indicate that the file contains binary data, assume "
                          "that the file is of type @v{type}.  By default, @v{type} is \"binary\", and "
                          "@prop{programName} normally outputs either a one-line message saying that a binary "
                          "file matches, or no message if there is no match.  If @v{type} is \"without-match"
                          "@prop{programName} assumes that a binary file does not match; this is equivalent to "
                          "the @s{I} option.  If @v{type} is \"text\", @prop{programName} processes a binary "
                          "file as if it were text; ths is equivalent to the @s{a} option.  @em Warning: "
                          "\"@prop{programName} @s{binary-files}=text\" might output binary garbage, which can "
                          "have nasty side effects if the output is a terminal and if the terminal driver "
                          "interprets some of it as commands."));
    selection.insert(Switch("devices", 'D')
                     .argument("action",
                               enumParser(opt.deviceAction)
                               ->with("read", ACTION_READ)
                               ->with("skip", ACTION_SKIP))
                     .doc("If an input file is a device, FIFO, or socket, use @v{action} to process it.  By "
                          "default, @v{action} is \"read\", which means that devices are read just as if they "
                          "were ordinary files.  If @v{action} is \"skip\", devices are silently skipped."));
    selection.insert(Switch("directories", 'd')
                     .argument("action",
                               enumParser(opt.directoryAction)
                               ->with("read", ACTION_READ)
                               ->with("skip", ACTION_SKIP)
                               ->with("recurse", ACTION_RECURSE))
                     .doc("If an input file is a directory, use @v{action} to process it.  By default, "
                          "@v{action} is \"read\", which means that directories are read just as if they were "
                          "ordinary files.  If @v{action} is \"skip\", directories are silently skipped. If "
                          "@v{action} is \"recurse\", @prop{programName} reads all files under each directory, "
                          "recursively; this is equivalent to the @s{r} option."));
    selection.insert(Switch("exclude")
                     .argument("glob", anyParser(opt.excludeGlob))
                     .doc("Skip files whose base name matches @v{glob} (using wildcard matching). A file-name "
                          "glob can use \"*\", \"?\", and \"[...]\" as wildcards, and \"\\\" to quote a wildcard "
                          "or backslash character literally."));
    selection.insert(Switch("exclude-from")
                     .argument("file", anyParser(opt.excludeFromFile))
                     .doc("Skip files whose base name matches any of the file-name globs read from @v{file} "
                          "(using wildcard matching as described under @s{exclude})."));
    selection.insert(Switch("", 'I')
                     .intrinsicValue(BIN_SKIP, opt.binaryFile)
                     .doc("Process a binary file as if it did not contain matching data; this is equivalent "
                          "to the \"@s{binary-files}=without-match\" option."));
    selection.insert(Switch("include")
                     .argument("glob", anyParser(opt.includeGlob))
                     .doc("Search only files whose base names match @v{glob} (using wildcard matching as "
                          "described under @s{exclude})."));
    selection.insert(Switch("recursive", 'R')
                     .shortName('r')
                     .intrinsicValue(ACTION_RECURSE, opt.directoryAction)
                     .doc("Read all files under each directory, recursively; this is equivalent to the "
                          "\"@s{d} recurse\" option."));

    SwitchGroup misc("Other Options");
    misc.switchOrder(INSERTION_ORDER);
    misc.insert(Switch("line-buffered")
                .intrinsicValue(true, opt.lineBuffered)
                .doc("Use line buffering on output.  This can cause a performance penalty."));
    misc.insert(Switch("mmap")
                .intrinsicValue(true, opt.useMmap)
                .doc("If possible, use the @man{mmap}(2) system call to read input, instead of the default "
                     "@man{read}(2) system call.  In some situations, @s{mmap} yields better performance.  "
                     "However, @s{mmap} can cause undefined behavior (including core dumps) if an input "
                     "file shrinks while @prop{programName} is operating, or if an I/O error occurs."));
    misc.insert(Switch("binary", 'U')
                .intrinsicValue(true, opt.openAsBinary)
                .doc("Treat the file(s) as binary.  By default, under MS-DOS and MS-Windows, "
                     "@prop{programName} guesses the file type by looking at the contents of the first 32kB "
                     "read from the file.  If @prop{programName} decides the file is a text file, it strips "
                     "the CR characters from the original file contents (to make regular expressions with "
                     "\"^\" and \"$\" work correctly).  Specifying @s{U} overrules this guesswork, causing all "
                     "files to be read and passed to the matching mechanism verbatim; if the file is a text file "
                     "with CR/LF pairs at the end of each line, this will cause some regular expressions to "
                     "fail.  This option has no effect on platforms other than MS-DOS and MS-Windows."));
    misc.insert(Switch("null-data", 'Z')
                .intrinsicValue(true, opt.nulTerminatedLines)
                .doc("Treat the input as a set of lines, each terminated by a zero byte (the ASCII NUL character) "
                     "instead of a newline.  Like the @s{Z} or @s{null} option, this option can be used with "
                     "commands like \"sort -z\" to process arbitrary file names."));


    // Build the parser
    Parser parser;
    parser
        .with(generic)
        .with(matcher)
        .with(control)
        .with(output)
        .with(prefix)
        .with(context)
        .with(selection)
        .with(misc);

    // Add some top-level documentation.
    parser
        .programName("demoGrep")                        // override the real command name
        .purpose("print lines matching a pattern")
        .version(VERSION_STRING)
        .doc("Synopsis",
             "@em{@prop{programName}} [@v{options}] @v{pattern} [@v{file}...]\n\n"
             "@em{@prop{programName}} [@v{options}] [@s{e} @v{pattern} | @s{f} @v{file}] [@v{file}...]")
        .doc("Description",
             "@prop{programName} searches the named input @v{file}s (or standard input if no files are named, or if "
             "a single hyphen-minus (\"-\") is given as the file name) for lines containing a match to the given "
             "@v{pattern}.  By default, @prop{programName} prints the matching lines."
             "\n\n"
             "In addition, three variant programs egrep, fgrep, and rgrep are available.  egrep is the same as "
             "\"@prop{programName} @s{E}\"; fgrep is the same as \"@prop{programName} @s{F}\"; rgrep is the same as "
             "\"@prop{programName} @s{r}\".  Direct invocation as either \"egrep\" or \"fgrep\" is deprecated, but "
             "is provided to allow historical applications that rely on them to run unmodified.")
        .doc("Regular Expressions",
             "A regular expression is a pattern that describes a set of strings.  Regular expressions are "
             "constructed analogously to arithmetic expressions, by using various operators to combine smaller "
             "expressions."
             "\n\n"
             "@prop{programName} understands three different versions of regular expression syntax: \"basic,\" "
             "\"extended\" and \"perl.\"  In @prop{programName}, there is no difference in available functionality "
             "between basic and extended syntaxes.  In other implementations, basic regular expressions are less "
             "powerful.  The following description applies to extended regular expressions; differences for basic "
             "regular expressions are summarized afterwards.  Perl regular expressions give additional functionality, "
             "and are documented in @man{pcresyntax}(3) and @man{pcrepattern}(3), but may not be available on every "
             "system."
             "\n\n"
             "The fundamental building blocks are the regular expressions that match a single character.  Most "
             "characters, including all letters and digits, are regular expressions that  match  themselves.   Any "
             "meta-character with special meaning may be quoted by preceding it with a backslash."
             "\n\n"
             "The period \".\" matches any single character.");


    // Parse the command-line
    ParserResult cmdline = parser.parse(argc, argv);

    // Apply the parser results, causing values to be saved and actions to be executed.
    cmdline.apply();

}

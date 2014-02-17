/** Command-line switch parsing. */
#ifndef Sawyer_CommandLine_H
#define Sawyer_CommandLine_H

#include <boost/any.hpp>
#include <boost/cstdint.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace Sawyer {
namespace CommandLine {

const std::string STR_NONE(" %-NONE^}"); // unusual value for default string arguments when empty string is valid input

/*******************************************************************************************************************************
 *                                      Program argument cursor
 *******************************************************************************************************************************/

struct Location {
    size_t idx;                                                 // index into a vector of strings
    size_t offset;                                              // character offset within a string
    Location(): idx(0), offset(0) {}
    Location(size_t idx, size_t offset): idx(idx), offset(offset) {}
    bool operator==(const Location &other) const { return idx==other.idx && offset==other.offset; }
};

std::ostream& operator<<(std::ostream&, const Location&);
extern const Location NOWHERE;

class Cursor {
    const std::vector<std::string> &strings_;                   // program arguments
    Location loc_;
public:
    Cursor(const std::vector<std::string> &strings): strings_(strings) {}

    // Current position of the cursor.
    const Location& location() const { return loc_; }
    Cursor& location(const Location &loc) { loc_ = loc; return *this; }

    // True if the cursor points to an argument and is at the beginning of that argument.
    bool atArgBegin() const { return loc_.idx<strings_.size() && 0==loc_.offset; }

    // True if the cursor is after the last argument or after the last character of the current argument.
    bool atArgEnd() const { return loc_.idx>=strings_.size() || loc_.offset==strings_[loc_.idx].size(); }

    // True when the cursor is after all arguments
    bool atEnd() const { return loc_.idx>=strings_.size(); }

    // Return the entire current program argument regardless of where the cursor is in that argument.
    const std::string& arg() const { return strings_[loc_.idx]; }

    // Return the part of an argument at and beyond the cursor.  If the cursor is at the end of an argument then this will be
    // the empty string.
    std::string rest() const { return strings_[loc_.idx].substr(loc_.offset); }

    // Advance the cursor N characters within the current argument, but never going beyond the end to the next argument.
    void consumeChars(size_t nchars) { loc_.offset = std::min(strings_[loc_.idx].size(), loc_.offset+nchars); }

    // Advance the cursor to the beginning of the next argument. Permissible to advance past the last argument.
    void consumeArgs(size_t nargs) {
        loc_.idx = std::min(strings_.size(), loc_.idx+nargs);
        loc_.offset = 0;
    }
    void consumeArg() { consumeArgs(1); }
};

class ExcursionSaver {
    Cursor &cursor_;
    Location loc_;
    bool canceled_;
public:
    ExcursionSaver(Cursor &cursor): cursor_(cursor), loc_(cursor.location()), canceled_(false) {} // implicit
    ~ExcursionSaver() { if (!canceled_) cursor_.location(loc_); }
    void cancel() { canceled_ = true; }
};

/*******************************************************************************************************************************
 *                                      Switch Argument Parsers
 *******************************************************************************************************************************/

typedef boost::shared_ptr<class ValueParser>            ValueParserPtr;
typedef boost::shared_ptr<class AnyParser>              AnyParserPtr;
typedef boost::shared_ptr<class IntegerParser>          IntegerParserPtr;
typedef boost::shared_ptr<class UnsignedIntegerParser>  UnsignedIntegerParserPtr;
typedef boost::shared_ptr<class RealNumberParser>       RealNumberParserPtr;
typedef boost::shared_ptr<class BooleanParser>          BooleanParserPtr;
typedef boost::shared_ptr<class StringSetParser>        StringSetParserPtr;
typedef boost::shared_ptr<class ListParser>             ListParserPtr;

class NotMatched {};

/** Base class for all switch argument parsers. */
class ValueParser: public boost::enable_shared_from_this<ValueParser> {
public:
    virtual ~ValueParser() {}

    /** Parse the entire string and return a value.  Throw at NotMatched exception if the string cannot be parsed. */
    boost::any matchAll(const std::string&);

    /** Parse a value from the beginning of the specified string. */
    boost::any match(const char *input, const char **rest);

    virtual boost::any operator()(const char *input, const char **rest) = 0;
};

/** Parses any argument as plain text. Returns std::string. */
class AnyParser: public ValueParser {
public:
    virtual boost::any operator()(const char *input, const char **rest) /*override*/;
};

/** Parses an integer.  The integer may be positive, negative, or zero in decimal, octal, or hexadecimal format using the C/C++
 *  syntax.  The syntax is that which is recognized by the strtoll() function. Returns boost::int64_t */
class IntegerParser: public ValueParser {
public:
    virtual boost::any operator()(const char *input, const char **rest) /*override*/;
};

/** Parses a non-negative integer.  The integer may be positive or zero in decimal, octal, or hexadecimal format using the
 *  C/C++ syntax.  The syntax is that which is recognized by the strtoull function. Returns boost::uint64_t. */
class UnsignedIntegerParser: public ValueParser {
public:
    virtual boost::any operator()(const char *input, const char **rest) /*override*/;
};

/** Parses a floating point constant. The syntax is that which is recognized by the strtod() function. Returns double. */
class RealNumberParser: public ValueParser {
public:
    virtual boost::any operator()(const char *input, const char **rest) /*override*/;
};

/** Parses a boolean value.  True values are: 1, t, true, y, yes, on; false values are 0, f, false, n, no, off. Comparisons are
 * case insensitive. */
class BooleanParser: public ValueParser {
public:
    virtual boost::any operator()(const char *input, const char **rest) /*override*/;
};

/** Parses any one of a set of strings.  The return value is of type std::string. Returns std::string. */
class StringSetParser: public ValueParser {
    std::vector<std::string> strings_;
public:
    StringSetParserPtr with(const std::string &s) { return with(&s, &s+1); }
    StringSetParserPtr with(const std::vector<std::string> sv) { return with(sv.begin(), sv.end()); }

    template<class InputIterator>
    StringSetParserPtr with(InputIterator begin, InputIterator end) {
        strings_.insert(strings_.end(), begin, end);
        return boost::dynamic_pointer_cast<StringSetParser>(shared_from_this());
    }

    virtual boost::any operator()(const char *input, const char **rest) /*override*/;
};

/** Parses an enumerated constant. The template parameter is the enum type. Returns T. */
template<typename T>
class EnumParser: public ValueParser {
    StringSetParserPtr strParser_;
    std::map<std::string, T> members_;
public:
    typedef boost::shared_ptr<EnumParser> Ptr;

    EnumParser(): strParser_(StringSetParserPtr(new StringSetParser)) {}

    Ptr with(const std::string &name, T value) {
        strParser_->with(name);
        members_[name] = value;
        return boost::dynamic_pointer_cast<EnumParser>(shared_from_this());
    }

    virtual boost::any operator()(const char *input, const char **rest) /*override*/ {
        std::string str = boost::any_cast<std::string>((*strParser_)(input, rest));
        return members_[str];
    }
};

/** Parses a list of values. */
class ListParser: public ValueParser {
    typedef std::pair<ValueParserPtr, std::string> ParserSep;
    std::vector<ParserSep> elements_;
    size_t minLength_, maxLength_;                      // limits on the number of values permitted
public:
    ListParser(const ValueParserPtr &firstElmtType, const std::string &separatorRe=",\\s*")
        : minLength_(1), maxLength_((size_t)-1) {
        elements_.push_back(ParserSep(firstElmtType, separatorRe));
    }
    ListParserPtr nextMember(const ValueParserPtr &elmtType, const std::string &separatorRe=",\\s*") {
        elements_.push_back(ParserSep(elmtType, separatorRe));
        return boost::dynamic_pointer_cast<ListParser>(shared_from_this());
    }
    ListParserPtr limit(size_t minLength, size_t maxLength);
    ListParserPtr limit(size_t maxLength) { return limit(std::min(minLength_, maxLength), maxLength); }
    ListParserPtr exactly(size_t length) { return limit(length, length); }

    virtual boost::any operator()(const char *input, const char **rest) /*override*/;
};
    

// Factories
AnyParserPtr anyParser();
IntegerParserPtr integerParser();
UnsignedIntegerParserPtr unsignedIntegerParser();
RealNumberParserPtr realNumberParser();
BooleanParserPtr booleanParser();
StringSetParserPtr stringSetParser();
ListParserPtr listParser(const ValueParserPtr&, const std::string &sepRe=",\\s*");

// Note: enum T must be defined at global scope
template<typename T>
typename EnumParser<T>::Ptr enumParser() {
    return boost::shared_ptr<EnumParser<T> >(new EnumParser<T>);
}

/*******************************************************************************************************************************
 *                                      Switch Argument Descriptors
 *******************************************************************************************************************************/

// Describes one argument of a command-line switch. Some switches may have more than one argument ("--swap a b"), or an
// argument that is a list ("--swap a,b"), or appear more than once to create a list ("--swap a --swap b"); these are handled
// at the Switch level, not the Argument level.  An argument parses one single entity (although the "a,b" in "--swap a,b" could
// be handled as a single entity that is a list).  Users don't usually use this directly, but rather the argument() factory.
class SwitchArgument {
    std::string name_;
    ValueParserPtr parser_;
    boost::any defaultValue_;
public:
    SwitchArgument() {}

    explicit SwitchArgument(const std::string &name, const ValueParserPtr &parser = anyParser())
        : name_(name), parser_(parser) {}
    SwitchArgument(const std::string &name, const ValueParserPtr &parser, const std::string &defaultValueStr)
        : name_(name), parser_(parser), defaultValue_(parser->matchAll(defaultValueStr)) {}

    bool isRequired() const {
        return defaultValue_.empty();
    }

    boost::any defaultValue() const {
        return defaultValue_;
    }

    const ValueParserPtr& parser() const { return parser_; }
};

/*******************************************************************************************************************************
 *                                      Switch Value Augmentor
 *******************************************************************************************************************************/

typedef boost::shared_ptr<class ValueAugmenter> ValueAugmenterPtr;

class ValueAugmenter {
public:
    virtual ~ValueAugmenter() {}
    virtual void operator()() = 0;                           // FIXME[Robb Matzke 2014-02-14]
};

template<typename T>
class Increment: public ValueAugmenter {
    T delta_;
public:
    Increment(T delta): delta_(delta) {}
    virtual void operator()();
};

template<typename T>
boost::shared_ptr<Increment<T> >
incrementValue(T delta=1) {
    return boost::shared_ptr<Increment<T> >(new Increment<T>(delta));
}


/*******************************************************************************************************************************
 *                                      Switch Immediate Actions
 *******************************************************************************************************************************/

typedef boost::shared_ptr<class SwitchAction>           SwitchActionPtr;
typedef boost::shared_ptr<class ExitProgram>            ExitProgramPtr;
typedef boost::shared_ptr<class ShowVersion>            ShowVersionPtr;
typedef boost::shared_ptr<class ShowHelp>               ShowHelpPtr;

class SwitchAction {
public:
    virtual ~SwitchAction() {}
    virtual void operator()() = 0;                      // FIXME[Robb Matzke 2014-02-14]
};

class ExitProgram: public SwitchAction {
    int exitStatus_;
public:
    explicit ExitProgram(int exitStatus): exitStatus_(exitStatus) {}
    virtual void operator()() /*override*/;
};

class ShowVersion: public SwitchAction {
    std::string versionString_;
public:
    explicit ShowVersion(const std::string &versionString): versionString_(versionString) {}
    virtual void operator()() /*overload*/;
};

class ShowHelp: public SwitchAction {
    std::ostream &stream_;
public:
    explicit ShowHelp(std::ostream &stream): stream_(stream) {}
    virtual void operator()() /*override*/;
};

ExitProgramPtr exitProgram(int exitStatus);
ShowVersionPtr showVersion(const std::string &versionString);
ShowHelpPtr showHelp(std::ostream &o=std::cerr);

/*******************************************************************************************************************************
 *                                      Parsed value
 *******************************************************************************************************************************/

class Switch;

// Lowest level of info stored in the ParserResult.
class ParsedValue {
    boost::any value_;
    Location valueLocation_;                            // where this value came from (virtually) on the command-line
    std::string valueString_;                           // string representation of the value if it came from the command-line
    const Switch *switch_;                              // switch that parsed this value
    Location switchLocation_;                           // where is the actual switch name in switchString_?
    std::string switchString_;                          // prefix and switch name
    size_t keyOrdinal_, switchOrdinal_;                 // location of this value w.r.t. other values for same key and switch
    
public:
    ParsedValue(const boost::any value, const Location &loc, const std::string &str)
        : value_(value), valueLocation_(loc), valueString_(str),
          switch_(NULL), keyOrdinal_(0), switchOrdinal_(0) {}

    // Update the switch information for the parsed value.
    ParsedValue& switchInfo(const Switch *sw, const Location &loc, const std::string &str) {
        switch_ = sw; switchLocation_ = loc; switchString_ = str;
        return *this;
    }

    // Update ordinal info
    void ordinalInfo(size_t keyOrdinal, size_t switchOrdinal) {
        keyOrdinal_ = keyOrdinal; switchOrdinal_ = switchOrdinal;
    }

    // Location in the command line where the value appeared.  The location will be valid even for values created for
    // optional switch arguments that didn't actually occur on the command line.
    size_t valueIndex() const { return valueLocation_.idx; }
    size_t valueBegin() const { return valueLocation_.offset; }

    // The string from the command line that was parsed to get a value if this value came from the command line. Empty
    // string for default values.
    const std::string &string() const { return valueString_; }

    // The parsed value
    const boost::any& value() const { return value_; }

    // More convenient forms to get the value that don't need a surrounding boost::any_cast
    int valueAsInt() const;
    unsigned valueAsUnsigned() const;
    long valueAsLong() const;
    unsigned long valueAsUnsignedLong() const;
    boost::int64_t valueAsInt64() const;
    boost::uint64_t valueAsUnsigned64() const;
    double valueAsDouble() const;
    bool valueAsBool() const;
    std::string valueAsString() const;                  // possibly different than string()
    template<typename T> T valueAs() const;

    // The location of the switch to which this value belongs.  E.g., if the first program argument was
    // "--file=/dev/null" then these return (0, 2, 6).  Anything before the switch is the prefix.  For single-character
    // switches that were nestled together, the switch name is rewritten. E.g., if the program argument was "-abc" and
    // we're asking about switch "b" the switchString() will be "-b" and switchBegin() is 1.
    size_t switchIndex() const { return switchLocation_.idx; }
    size_t switchBegin() const { return switchLocation_.offset; }

    // The string for the switch that caused this value to be parsed.  The string described by switchIndex() et. al.
    const std::string& switchString() const { return switchString_; }

    // The description of the switch that parsed this value.
    const Switch* descriptor() const { return switch_; }

    // Ordinal of this value among all values for the same key
    size_t keyOrdinal() const { return keyOrdinal_; }

    // Ordinal of this value among all values for the same switch
    size_t switchOrdinal() const { return switchOrdinal_; }

    // Print some debug info
    void print(std::ostream&) const;
};

std::ostream& operator<<(std::ostream&, const ParsedValue&);

typedef std::vector<ParsedValue> ParsedValues;

/*******************************************************************************************************************************
 *                                      Switch Descriptors
 *******************************************************************************************************************************/

// Used internally to pass around switch properties
struct ParsingProperties {
    std::vector<std::string> longPrefixes;              // Prefixes for long command-line switches
    bool inheritLongPrefixes;                           // True if this also inherits longPrefixes from a higher layer
    std::vector<std::string> shortPrefixes;
    bool inheritShortPrefixes;
    std::vector<std::string> valueSeparators;           // What separates a long switch from its value.
    bool inheritValueSeparators;
    ParsingProperties()
        : inheritLongPrefixes(true), inheritShortPrefixes(true), inheritValueSeparators(true) {}
    ParsingProperties inherit(const ParsingProperties &base) const;
};

/** Describes one command-line switch.  A switch is something that typically starts with a hyphen on Unix or a slash on
 * Windows. A Switch is not used for non-switch program arguments that typically occur after the switches. */
class Switch {
public:
    /** Describes how to handle switches that occur multiple times. */
    enum Save {
        SAVE_NONE,                                      /**< Switch is disabled. Any occurrence will be an error. */
        SAVE_ONE,                                       /**< Switch cannot appear more than once. */
        SAVE_LAST,                                      /**< Use only the last occurrence and ignore all previous. */
        SAVE_FIRST,                                     /**< Use only the first occurrence and ignore all previous. */
        SAVE_ALL,                                       /**< Save all values as a vector. */
        SAVE_AUGMENT,                                   /**< Save the first value, or modify previously saved value. */
    };

private:
    std::vector<std::string> longNames_;                // long name of switch, or empty string
    std::string shortNames_;                            // optional short names for this switch
    std::string key_;                                   // unique key, usually the long name or the first short name
    ParsingProperties properties_;                      // properties valid at multiple levels of the hierarchy
    std::string synopsis_;                              // user-defined synopsis or empty string
    std::string documentation_;                         // main documentation for the switch
    std::string documentationKey_;                      // for sorting documentation
    bool hidden_;                                       // hide documentation?
    std::vector<SwitchArgument> arguments_;             // arguments with optional default values
    std::vector<SwitchActionPtr> actions_;              // what happens as soon as the switch is parsed
    Save save_;                                         // which switch values should be saved
    ValueAugmenterPtr augmenter_;                       // used if save_==SAVE_AUGMENT
    boost::any defaultValue_;                           // default when no arguments are present
public:
    /** The @shortName should be an empty string or a single character. Neither name should include switch prefixes (e.g.,
     * "-"). */
    explicit Switch(const std::string &longName, const std::string &shortNames="")
        : hidden_(false), defaultValue_(true) {
        init(longName, shortNames);
    }

    /** Long name of the switch.  With no arguments, the primary long name is returned (the one from the construtor).
     * @{ */
    Switch& longName(const std::string &name);
    const std::string& longName() const { return longNames_.front(); }
    const std::vector<std::string>& longNames() const { return longNames_; }
    /** @} */

    /** Return a string containing all the short names in no particular order. */
    const std::string& shortNames() const { return shortNames_; }

    /** Key used when parsing this switch. The default is to use the long switch name if present, or else the first single
     *  letter name.  The switch prefix (e.g., "-") should not be part of the key.
     * @{ */
    Switch& key(const std::string &s) { key_ = s; return *this; }
    const std::string &key() const { return key_; }
    /** @} */

    /** Abstract summary of the switch syntax.  The synopsis is normally generated automatically from other information
     *  specified for the switch, but the user may provide a synopsis to override the generated one.  A synopsis should
     *  be a comma-separated list of alternative switch sytax specifications using markup to specify things such as the
     *  switch prefix and switch/value separator.  Using markup will cause the synopsis to look correct regardless of
     *  the operating system or output media.  See [[markup]] for details.  For example:
     *
     * @code
     *  @{shortprefix}C @V{commit}, @{longprefix}reuse-message@{valsep}@V{commit}
     * @endcode
     *
     *  The reason a user may want to override the synopsis is to be able to document two related switches at once.  For
     *  instance, the git-rev-parse(1) man page has the following documentation for these three switches:
     *
     * @code
     *  --branches[=PATTERN], --tags[=PATTERN], --remotes[=PATTERN]
     *      Show all branches, tags, or remote-tracking branches, respecitively (i.e., refs
     *      found in refs/heads, refs/tags, or refs/remotes, respectively).  If a pattern is
     *      given, only refs matching the given shell glob are shown.  If the pattern does
     *      not contain a globbing character (?, *, or [), it is turned into a prefix match
     *      by appending a slash followed by an asterisk.
     * @endcode
     *
     *  The previous example can be accomplished by documenting only one of the three switches (with a user-specified
     *  synopsis) and making the other switches hidden.
     * @{ */
    Switch& synopsis(const std::string &s) { synopsis_ = s; return *this; }
    std::string synopsis() const;
    /** @} */

    /** Detailed description.  This is the description of the switch in a simple markup language.  See [[markup]] for details.
     * @{ */
    Switch& doc(const std::string &s) { documentation_ = s; return *this; }
    const std::string& doc() const { return documentation_; }
    /** @} */

    /** Key to control order of documentation.  Normally, documentation for a group of switches is sorted according to the
     *  long name of the switch (or the short name when there is no long name).  Specifying a key causes the key string to be
     *  used instead of the switch names.
     *  @{ */
    Switch& docKey(const std::string &s) { documentationKey_ = s; return *this; }
    const std::string &docKey() const { return documentationKey_; }
    /** @} */
    
    /** Whether this switch appears in documentation. A hidden switch still participates when parsing command lines, but will
     *  not show up in documentation.  This is ofen used for a switch when that switch is documented as part of some other
     *  switch. */
    Switch& hidden(bool b) { hidden_ = b; return *this; }
    bool hidden() const { return hidden_; }

    /** Prefix strings for long names.  Prefix strings are usually inherited from the enclosing SwitchGroup, but this method
     *  overrides them for this switch.  Long switch prefixes are usually "--" and/or "-". The empty string is a valid prefix.
     * @{ */
    Switch& resetLongPrefixes(const std::string &s1=STR_NONE, const std::string &s2=STR_NONE,
                              const std::string &s3=STR_NONE, const std::string &s4=STR_NONE);
    Switch& longPrefix(const std::string &s1) { properties_.longPrefixes.push_back(s1); return *this; }
    const std::vector<std::string>& longPrefixes() const { return properties_.longPrefixes; }
    /** @} */

    /** Prefix strings for short names.  Prefix strings are usually inherited from the enclosing SwitchGroup, but this method
     *  overrides them for this switch.  The short switch prefixes is usually "-".  Up to four prefixes can be specified as
     *  arguments, or the prefixes can be passed as a vector.  The empty string is a valid prefix.
     * @{ */
    Switch& resetShortPrefixes(const std::string &s1=STR_NONE, const std::string &s2=STR_NONE,
                               const std::string &s3=STR_NONE, const std::string &s4=STR_NONE);
    Switch& shortPrefix(const std::string &s1) { properties_.shortPrefixes.push_back(s1); return *this; }
    const std::vector<std::string>& shortPrefixes() const { return properties_.shortPrefixes; }
    /** @} */

    /** Strings that separate a long switch from its value.  The string " " means the value must be in the next
     *  program argument.
     * @{ */
    Switch& resetValueSeparators(const std::string &s1=STR_NONE, const std::string &s2=STR_NONE,
                                 const std::string &s3=STR_NONE, const std::string &s4=STR_NONE);
    Switch& valueSeparator(const std::string &s1) { properties_.valueSeparators.push_back(s1); return *this; }
    const std::vector<std::string>& valueSeparators() const { return properties_.valueSeparators; }
    /** @} */

    /** Switch argument.  Switches usually have zero or one argument for each occurrence of the switch on the command line, but
     *  it is permissible for switches to have multiple arguments.  See also multiple(), which handles the situation where a
     *  switch occurs multiple times on the command line.
     *
     *  The @p name is used only in documentation.  If a @p defaultValue is supplied, then it indicates that the argument is
     *  optional and when the argument is missing this value should be used.  The default value must be parsable by the
     *  specified parser.
     *  @{ */
    Switch& argument(const std::string &name, const ValueParserPtr &p = anyParser());
    Switch& argument(const std::string &name, const ValueParserPtr&, const std::string &defaultValue);
    Switch& argument(const SwitchArgument &arg) { arguments_.push_back(arg); return *this; }
    size_t nArguments() const { return arguments_.size(); }
    size_t nRequiredArguments() const;
    const SwitchArgument& argument(size_t idx) const { return arguments_[idx]; }
    const std::vector<SwitchArgument>& arguments() const { return arguments_; }
    /** @} */

    /** The default value for a switch that has no arguments declared.  For instance, a "--verbose" switch might have a default
     *  value of "1" as an integer.  If no default value is specified for a switch with zero arguments declared, then the
     *  Boolean value true is used (the "true" string as parsed by BooleanParser).  On the other hand, if a switch has at least
     *  one argument declared then this setting is not consulted; arguments have their own mechanism for indicating the default
     *  value when the argument is optional (see argument()).
     *  @{ */
    Switch& defaultValue(const std::string &text, const ValueParserPtr &p = anyParser());
    boost::any defaultValue() const { return defaultValue_; }
    /** @} */

    /** Action to occur each time a switch is parsed.  All switches cause the parser result to be updated and usually this
     *  parser result is queried after the parser returns in order to decide what the program should do.  However, sometimes
     *  it's convenient to cause a switch to do something immediately when it's parsed, and that's the purpose of this property.
     *
     *  For instance, the "--help" switch can be bound to an action that causes the command-line parser to emit documentation,
     *  and a "--verbose" switch can be bound to an action that causes verbosity to immediately be increased (which is useful
     *  if argument parsing itself uses it).
     *  
     *  @{ */
    Switch& action(const SwitchActionPtr&);
    const std::vector<SwitchActionPtr>& actions() const { return actions_; }
    /** @} */

    /** Describe what to do if a switch occurs more than once.  The save() function is the general mechanism for adjusting this
     *  property, and the others are for convenience.
     * @{ */
    Switch& save(Save s) { save_ = s; return *this; }
    Save save() const { return save_; }
    Switch& saveNone() { return save(SAVE_NONE); }
    Switch& saveOne() { return save(SAVE_ONE); }
    Switch& saveLast() { return save(SAVE_LAST); }
    Switch& saveFirst() { return save(SAVE_FIRST); }
    Switch& saveAll() { return save(SAVE_ALL); }
    Switch& saveAugment(const ValueAugmenterPtr f) { save(SAVE_AUGMENT); return augmentValue(f); }
    /** @} */

    /** The functor to call when augmenting a previously saved value.
     *  @{ */
    Switch& augmentValue(const ValueAugmenterPtr &f) { augmenter_ = f; return *this; }
    ValueAugmenterPtr augmentValue() const { return augmenter_; }
    /** @} */

    // used internally
    const ParsingProperties& properties() const { return properties_; }

    // Constructs an exception describing that there is no separator between the switch name and its value.
    std::runtime_error noSeparator(const std::string &switchString, const Cursor&, const ParsingProperties&) const;

    // Constructs an exception describing that there is unexpected extra text after a switch argument.
    std::runtime_error extraTextAfterArgument(const std::string &switchString, const Cursor&) const;

    // Constructs an exception describing that we couldn't parse all the required arguments
    std::runtime_error notEnoughArguments(const std::string &switchString, const Cursor&, size_t nargs) const;

    // Determines if this switch can match against the specified program argument when considering only this switch's long
    // names.  If program argument starts with a valid long name prefix and then matches the switch name, this this function
    // returns true (the number of characters matched).  Switches that take no arguments must match to the end of the string,
    // but switches that have arguments (even if they're optional or wouldn't match the rest of the string) do not have to
    // match entirely as long as a value separator is found when they don't match entirely.
    size_t matchLongName(const std::string &arg, const ParsingProperties &props) const;

    // Parses switch arguments from the command-line arguments to obtain switch values.  Upon entry, the cursor should point to
    // the first character after the switch name; it will be updated to point to the beginning of the next program argument
    // after this switch and its arguments.  If anything goes wrong an exception is thrown (cursor and result might be
    // invalid).
    void matchArguments(const std::string &switchString, Cursor &cursor /*in,out*/, const ParsingProperties &props,
                        ParsedValues &result /*out*/) const;

    // Run the actions associated with this switch.
    void runActions() const;

private:
    void init(const std::string &longName, const std::string &shortNames);
};


/** A collection of related switches that are parsed in tandem. */
class SwitchGroup {
    std::vector<Switch> switches_;
    ParsingProperties properties_;
public:
    SwitchGroup& resetLongPrefixes(const std::string &s1=STR_NONE, const std::string &s2=STR_NONE,
                                   const std::string &s3=STR_NONE, const std::string &s4=STR_NONE);
    SwitchGroup& longPrefix(const std::string &s1) { properties_.longPrefixes.push_back(s1); return *this; }
    const std::vector<std::string>& longPrefixes() const { return properties_.longPrefixes; }

    SwitchGroup& resetShortPrefixes(const std::string &s1=STR_NONE, const std::string &s2=STR_NONE,
                                    const std::string &s3=STR_NONE, const std::string &s4=STR_NONE);
    SwitchGroup& shortPrefix(const std::string &s1) { properties_.shortPrefixes.push_back(s1); return *this; }
    const std::vector<std::string>& shortPrefixes() const { return properties_.shortPrefixes; }

    SwitchGroup& resetValueSeparators(const std::string &s1=STR_NONE, const std::string &s2=STR_NONE,
                                      const std::string &s3=STR_NONE, const std::string &s4=STR_NONE);
    SwitchGroup& valueSeparator(const std::string &s1) { properties_.valueSeparators.push_back(s1); return *this; }
    const std::vector<std::string>& valueSeparators() const { return properties_.valueSeparators; }
    /** @} */

    /** Return information about switches.
     *  @{ */
    size_t nSwitches() const { return switches_.size(); }
    const std::vector<Switch>& switches() const { return switches_; }
    const Switch& getByIndex(size_t idx) const { return switches_[idx]; }
    bool nameExists(const std::string &switchName);
    const Switch& getByName(const std::string &switchName);     // returns first match
    bool keyExists(const std::string &switchKey);
    const Switch& getByKey(const std::string &switchKey);       // returns first match
    /** @} */

    /** Insert a switch into the set. */
    SwitchGroup& insert(const Switch&);

    /** Remove a switch from the set. */
    SwitchGroup& removeByIndex(size_t idx);
    SwitchGroup& removeByName(const std::string &switchName);
    SwitchGroup& removeByKey(const std::string &switchKey);

    /** Make an alias between switch names. FIXME[Robb Matzke 2014-02-13]: is this the best way?
     *  @{ */
    SwitchGroup& alias(const std::string &newName, const std::string &existingName);
    /** @} */

    // used internally
    const ParsingProperties& properties() const { return properties_; }
};



class ParserResult {
public:
    // Returns the number of values parsed for the specified key.  Same as parsedKey("x").size().
    size_t nKey(const std::string &switchKey);

    // Returns values by key. Example the first value from switches that use the "verbose" key.
    //    bool b = cmdline.parsedKey("verbose")[0].valueAsBool();
    const ParsedValue& parsedKey(const std::string &switchKey, size_t idx);
    std::vector<ParsedValue> parsedKey(const std::string &switchKey);
    

    // Non-switch program arguments.  File inclusions are expanded.  Excludes termination switch. */
    std::vector<std::string> nonSwitches() const;

    // The following are for convenience.  You can get the same info from the functions above.

    // All command line switches and non-switches sent to the parser (after shell expansion for single-string parser). File
    // inclusion is expanded.  Includes termination switch if present.
    const std::vector<std::string>& commandLine() const;

    // Switches that were parsed in the order they were parsed, excluding non-switches.  Optionally include switches
    // that caused errors.  File inclusions are expanded.  Includes termination switch if present.
    std::vector<std::string> parsedSwitches(bool withErrors=false) const;

};


class Parser {
    std::vector<SwitchGroup> switchGroups_;
    ParsingProperties properties_;
    std::vector<std::string> terminationSwitches_;      // special switch to terminate parsing; default is "--"
    bool shortMayNestle_;                               // is "-ab" the same as "-a -b"?
    std::vector<std::string> inclusionPrefixes_;        // prefixes that mark command line file inclusion (e.g., "@")
    size_t maxInclusionDepth_;                          // max depth to prevent recursive inclusion
public:
    Parser(): shortMayNestle_(true), maxInclusionDepth_(10) { init(); }
    
    Parser& with(const SwitchGroup &sg) { switchGroups_.push_back(sg); return *this; }

    // default is "--" and "-"
    Parser& resetLongPrefixes(const std::string &s1=STR_NONE, const std::string &s2=STR_NONE,
                              const std::string &s3=STR_NONE, const std::string &s4=STR_NONE);
    Parser& longPrefix(const std::string &s1) { properties_.longPrefixes.push_back(s1); return *this; }
    const std::vector<std::string>& longPrefixes() const { return properties_.longPrefixes; }

    // default is "-"
    Parser& resetShortPrefixes(const std::string &s1=STR_NONE, const std::string &s2=STR_NONE,
                               const std::string &s3=STR_NONE, const std::string &s4=STR_NONE);
    Parser& shortPrefix(const std::string &s1) { properties_.shortPrefixes.push_back(s1); return *this; }
    const std::vector<std::string>& shortPrefixes() const { return properties_.shortPrefixes; }

    // default is "=" and " " (as in separate arguments)
    Parser& resetValueSeparators(const std::string &s1=STR_NONE, const std::string &s2=STR_NONE,
                                const std::string &s3=STR_NONE, const std::string &s4=STR_NONE);
    Parser& valueSeparator(const std::string &s1) { properties_.valueSeparators.push_back(s1); return *this; }
    const std::vector<std::string>& valueSeparators() const { return properties_.valueSeparators; }

    // default is "--"
    Parser& resetTerminationSwitches(const std::string &s1=STR_NONE, const std::string &s2=STR_NONE,
                                   const std::string &s3=STR_NONE, const std::string &s4=STR_NONE);
    Parser& terminationSwitch(const std::string &s1) { terminationSwitches_.push_back(s1); return *this; }
    const std::vector<std::string>& terminationSwitches() const { return terminationSwitches_; }

    Parser& shortMayNestle(bool b) { shortMayNestle_ = b; return *this; }
    bool shortMayNestle() const { return shortMayNestle(); }

    // whether command-line inclusion is allowed, and how many levels; default is "@" as in "@more-options"
    Parser& resetInclusionPrefixes(const std::string &s1=STR_NONE, const std::string &s2=STR_NONE,
                                   const std::string &s3=STR_NONE, const std::string &s4=STR_NONE);
    Parser& inclusionPrefix(const std::string &s1) { inclusionPrefixes_.push_back(s1); return *this; }
    const std::vector<std::string>& inclusionPrefixes() const { return inclusionPrefixes_; }

    Parser& maxInclusionDepth(size_t n) { maxInclusionDepth_ = n; return *this; }
    size_t maxInclusionDepth() const { return maxInclusionDepth_; }
        

    // Parsing starts with the second (index 1) element of argv since the first is the command name.  The argv array
    // does not require that argv[argc] be allocated.
    ParserResult parse(int argc, char *argv[]);

    // The vector should contain only command arguments, not the command name or a final empty string.
    ParserResult parse(const std::vector<std::string>&);

    template<typename Iterator>
    ParserResult parse(Iterator begin, Iterator end) {
        std::vector<std::string> args(begin, end);
        return parse(args);
    }

    // Parse one switch from the current position in the command line and return the switch descriptor.  If the cursor is at
    // the end of the command line then return null.  If the cursor is at a termination switch (e.g., "--") then consume the
    // terminator and return null.  If the switch name is valid but the arguments cannot be parsed, then throw an error.  If
    // the cursor is at what appears to be a switch but no matching switch declaration can be found, then throw an error.  The
    // cursor and parsedValues might be modified even when an error is thrown.
    const Switch* parseOneSwitch(Cursor&, ParsedValues &parsedValues /*out*/);

    // Returns true if the program argument at the cursor looks like it might be a switch.  Apparent switches are any program
    // argument that starts with a long or short prefix.
    bool apparentSwitch(const Cursor&) const;

private:
    void init();
};

} // namespace
} // namespace

#endif

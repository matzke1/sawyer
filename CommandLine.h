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

namespace Sawyer { // documented in Sawyer.h

/** Parses program command lines.
 *
 * @section defns Definitions
 *
 * <ul>
 *   <li>A <em>program command line</em> is the vector of strings passed to a program by the operating system or runtime.</li>
 *   <li>A <em>commmand line argument</em> is one element of the program command line vector.</li>
 *   <li>A <em>switch</em> is a named command line argument, usually introduced with a special character sequence followed
 *       by a name, such as "--color".</li>
 *   <li>A <em>switch argument</em> is an optional value specified on the program command line and associated with a switch,
 *       such as the word "red" in "--color=red" (or as two command line arguments, "--color", "red").</li>
 *   <li>A <em>switch value</em> is a switch argument that has been converted to a value within a program, such as the
 *       enumeration constant RED.</li>
 *   <li>A <em>non-switch</em> is a program argument that doesn't appear to be a switch. Another name for the same thing
 *       is <em>positional program argument</em>.</li>
 * </ul>
 *
 * @section parts The major parts of the API
 *
 * Program command-line parsing consists of the following major components:
 *
 * <ul>
 *   <li>Switch objects are used to define a switch and specify such things as the switch name and its arguments.</li>
 *   <li>SwitchGroup objects group related switches into collections.</li>
 *   <li>Parser objects match SwitchGroup objects agains a program command line to produce a ParserResult.</li>
 *   <li>ParserResult objects store all information about how a program command line was parsed by storing, among other things,
 *       a list of ParsedValue objects.</li>
 *   <li>ParsedValue objects store the details about each value parsed from a program command line, including such things as
 *       the switch with which the value is associated, and where the value came from on the program command line.</li>
 * <ul>
 *
 * @section desc Description
 *
 *  The library is used in three phases: first, the program is described in terms of switches and their arguments; then a
 *  parser is constructed and applied to the command line to obtain a result; and finally, the result is queried.
 *
 *  Some of our goals in designing this library were influenced by other libraries. We wanted to take the good ideas of others
 *  but avoid the same pitfalls.
 *
 * <ul>
 *   <li>Keep the switch declaration API as simple and terse as reasonably possible without the loss of self-documenting
 *       code.  Our approach is to use small, optional property-setting functions that can be chained together rather than
 *       excessive function overloading or magical strings.</li>
 *   <li>The switch declaration API should be able to describe all the situations that routinely occur in command-line parsing
 *       including things like optional arguments and multiple arguments, various ways to handle multiple occurrences of the
 *       same switch, short and long switch names, various switch prefixes and value separators, nestling of short switches and
 *       their arguments, standard switch actions, etc.  We accomplish this by using a consistent API of switch properties.</li>
 *   <li>Parsing of the command line should not only recognize the syntax of the command line, but also parse switch values
 *       into a form that is easy to use within the program.  E.g., a string like "1234" should become an "int" in the
 *       program.</li>
 *   <li>Provide an extension mechanism that is easy for beginning programmers to grasp. One way we accomplish this is by
 *       avoiding excessive use of templates for generic programming and instead rely on polymorphic classes and smart
 *       pointers, both of which are common in other object oriented languages.</li>
 *   <li>The library should provide an API that is suitable for both library writers and application programmers.  In other
 *       words, a library that expects to be configured from the command line should provide a description of its command-line
 *       in a way that allows the application to augment it with its own switches, even to the extent that the application can
 *       rename library switches if it desires.</li>
 *   <li>The parser results should have a rich query API since this is the point at which it primarily interfaces with the
 *       program.  A program should not have to work hard to use the parsing results.</li>
 */
namespace CommandLine {

extern const std::string STR_NONE;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Program argument cursor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Position within a command-line. */
struct Location {
    size_t idx;                                                 /**< Index into some vector of program argument strings. */
    size_t offset;                                              /**< Character offset within a program argument string. */
    Location(): idx(0), offset(0) {}
    Location(size_t idx, size_t offset): idx(idx), offset(offset) {}
    bool operator==(const Location &other) const { return idx==other.idx && offset==other.offset; }
    bool operator<(const Location &other) const { return idx<other.idx || (idx==other.idx && offset<other.offset); }
    bool operator<=(const Location &other) const { return *this<other || *this==other; }
};

std::ostream& operator<<(std::ostream&, const Location&);
extern const Location NOWHERE;

/** Input stream for command line arguments.
 *
 *  A cursor is a program command line and an associated position within the command line. */
class Cursor {
    std::vector<std::string> strings_;
    Location loc_;
public:
    /** Constructor. The cursor will reference the specified command line and be positioned at the first character of
     *  the first command line argument.  The specified vector should not be destroyed before the cursor. */
    Cursor(const std::vector<std::string> &strings): strings_(strings) {}
    Cursor(const std::string &string): strings_(1, string) {}

    /** Current position of the cursor.
     *  @{ */
    const Location& location() const { return loc_; }
    Cursor& location(const Location &loc);
    /** @} */

    /** Returns true if the cursor points to an argument and is at the beginning of that argument. */
    bool atArgBegin() const { return loc_.idx<strings_.size() && 0==loc_.offset; }

    /** Returns true when the cursor is after all arguments. */
    bool atEnd() const { return loc_.idx>=strings_.size(); }

    /** Return the entire current program argument regardless of where the cursor is in that argument.  A @p location can be
     *  specified to override the location that's inherent to this cursor without changing this cursor.  It is an error to call
     *  this when atEnd() would return true.
     * @{ */
    const std::string& arg() const { return strings_[loc_.idx]; }
    const std::string& arg(const Location &location) const { return strings_[location.idx]; }
    /** @} */

    /** Return the part of an argument at and beyond the cursor.  If the cursor is at the end of an argument then an empty
     *  string is returned.   A @p location can be specified to override the location that's inherent to this cursor without
     *  changing this cursor.  It is an error to call this when atEnd() would return true.
     * @{ */
    std::string rest() const { return strings_[loc_.idx].substr(loc_.offset); }
    std::string rest(const Location &location) const { return strings_[location.idx].substr(location.offset); }
    /** @} */

    /** Returns all characters within limits.  Returns all the characters between this cursor's current location and the
     *  specified location, which may be left or right of the cursor's location.  The two argument version uses the two
     *  specified locations rather than this cursor's current location.  The @p separator string is inserted between text that
     *  comes from two different program arguments.
     * @{ */
    std::string substr(const Location &limit, const std::string &separator=" ") { return substr(loc_, limit, separator); }
    std::string substr(const Location &limit1, const Location &limit2, const std::string &separator=" ");
    /** @} */

    /** Advance the cursor N characters.  The location is not advanced to the beginning of the next argument if it
     *  reaches the end of the current argument. */
    void consumeChars(size_t nchars);

    /** Advance the cursor to the beginning of the next argument. It is permissible to advance past the last argument, and to
     *  call this method when atEnd() already returns true.
     * @{ */
    void consumeArgs(size_t nargs) {
        loc_.idx = std::min(strings_.size(), loc_.idx+nargs);
        loc_.offset = 0;
    }
    void consumeArg() { consumeArgs(1); }
    /** @} */
};

/** Guards a cursor and restores it when destroyed.  If this object is destroyed without first calling its cancel() method then
 *  the associated cursor's location is reset to its location at the time this guard was constructed. */
class ExcursionGuard {
    Cursor &cursor_;
    Location loc_;
    bool canceled_;
public:
    ExcursionGuard(Cursor &cursor): cursor_(cursor), loc_(cursor.location()), canceled_(false) {} // implicit
    ~ExcursionGuard() { if (!canceled_) cursor_.location(loc_); }

    /** Cancel the excursion guard.  The associated cursor will not be reset to its initial location when this guard is
     *  destroyed. */
    void cancel() { canceled_ = true; }
};

/*******************************************************************************************************************************
 *                                      Switch Argument Parsers
 *******************************************************************************************************************************/

/** Base class parsing a value from input.
 *
 *  A ValueParser is a functor that attempts to recognize the next few characters of a command-line and to convert those
 *  characters to a value of some time. These are two separate but closely related operations.  Subclasses of ValueParser must
 *  implement one of the operator() methods: either the one that takes a Cursor reference argument, or the one that takes
 *  pointers to C strings a la strtod().  The cursor-based approach allows a match to span across several program arguments,
 *  and the C string pointer approach is a very simple interface.
 *
 *  If the parser is able to recognize the next characters of the input then it indicates so by advancing the cursor or
 *  updating the @p endptr argument; if no match occurs then the cursor is not changed and the @p endptr should point to the
 *  beginning of the string.
 *
 *  If the parser is able to convert the recognized characters into a value, then the value is returned as a boost::any,
 *  otherwise an std::runtime_error should be thrown.
 *
 *  For instance, a parser that recognizes non-negative decimal integers would match (consume) any positive number of
 *  consecutive digits. However, if that string represents a mathematical value which is too large to store in the return type,
 *  then the parser should throw an exception whose string describes the problem: "integer overflow", or "argument is too large
 *  to store in an 'int'".  The exception string should not include the string that was matched since that will be added by a
 *  higher layer.
 *
 *  Value parsers are always allocated on the heap and reference counted.  Each value parser defines a class factory method,
 *  "instance", to allocate a new object and return a pointer to it. The pointer types are named "Ptr" and are defined within
 *  the class.  For convenience, the parsers built into the library also have global factory functions which have the same name
 *  as the class but start with an initial lower-case letter.  For instance:
 *
 * @code
 *  class IntegerParser;                                // recognizes integers
 *  IntegerParser::Ptr ptr = IntegerParser::instance(); // construct a new object
 *  IntegerParser::Ptr ptr = integerParser();           // another constructor
 * @endcode
 *
 *  Users can create their own parsers (and are encouraged to do so) by following this same recipe. */
class ValueParser: public boost::enable_shared_from_this<ValueParser> {
protected:
    ValueParser() {}
public:
    typedef boost::shared_ptr<ValueParser> Ptr;
    virtual ~ValueParser() {}

    /** Parse the entire string and return a value.  The matching of the parser against the input is performed by calling
     *  match(), which may throw an exception if the input is matched but cannot be converted to a value (e.g., integer
     *  overflow).  If the parser did not match the entire string, then an std::runtime_error is thrown. */
    boost::any matchString(const std::string&) /*final*/;

    /** Parse a value from the beginning of the specified string.  If the parser does not recognize the input then it returns
     *  without updating the cursor, returning an empty boost::any.  If the parser recognizes the input but cannot convert it
     *  to a value (e.g., integer overflow) then the cursor should be updated to show the matching region before the matching
     *  operator throws an std::runtime_error exception. */
    boost::any match(Cursor&) /*final*/;

private:
    /** Parse next input characters. Each subclass should implement one of these. See documentation for class ValueParser.
     *  @{ */
    virtual boost::any operator()(Cursor&);
    virtual boost::any operator()(const char *input, const char **endptr);
    /** @} */
};

/** Parses any argument as plain text. Returns std::string. */
class AnyParser: public ValueParser {
protected:
    AnyParser() {}
public:
    typedef boost::shared_ptr<AnyParser> Ptr;
    static Ptr instance() { return Ptr(new AnyParser); } /**< Allocating constructor. */;
private:
    virtual boost::any operator()(Cursor&) /*override*/;
};

/** Parses an integer.  The integer may be positive, negative, or zero in decimal, octal, or hexadecimal format using the C/C++
 *  syntax.  The syntax is that which is recognized by the strtoll() function, plus it sucks up any trailing white space.
 *  Returns boost::int64_t */
class IntegerParser: public ValueParser {
protected:
    IntegerParser() {}
public:
    typedef boost::shared_ptr<IntegerParser> Ptr;
    static Ptr instance() { return Ptr(new IntegerParser); } /**< Allocating constructor. */
private:
    virtual boost::any operator()(const char *input, const char **rest) /*override*/;
};

/** Parses a non-negative integer.  The integer may be positive or zero in decimal, octal, or hexadecimal format using the
 *  C/C++ syntax.  The syntax is that which is recognized by the strtoull function, plus it sucks up any trailing white
 *  space. Returns boost::uint64_t. */
class UnsignedIntegerParser: public ValueParser {
protected:
    UnsignedIntegerParser() {}
public:
    typedef boost::shared_ptr<UnsignedIntegerParser> Ptr;
    static Ptr instance() { return Ptr(new UnsignedIntegerParser); } /**< Allocating constructor. */
private:
    virtual boost::any operator()(const char *input, const char **rest) /*override*/;
};

/** Parses a floating point constant. The syntax is that which is recognized by the strtod() function, plus it sucks up any
 *  trailing white space. Returns double. */
class RealNumberParser: public ValueParser {
protected:
    RealNumberParser() {}
public:
    typedef boost::shared_ptr<RealNumberParser> Ptr;
    static Ptr instance() { return Ptr(new RealNumberParser); } /**< Allocating constructor. */
private:
    virtual boost::any operator()(const char *input, const char **rest) /*override*/;
};

/** Parses a boolean value.  True values are: 1, t, true, y, yes, on; false values are 0, f, false, n, no, off. Comparisons are
 * case insensitive. The value may be preceded and/or followed by white space. */
class BooleanParser: public ValueParser {
protected:
    BooleanParser() {}
public:
    typedef boost::shared_ptr<BooleanParser> Ptr;
    static Ptr instance() { return Ptr(new BooleanParser); } /**< Allocating constructor. */
private:
    virtual boost::any operator()(const char *input, const char **rest) /*override*/;
};

/** Parses any one of a set of strings.  The return value is of type std::string. Returns std::string. */
class StringSetParser: public ValueParser {
    std::vector<std::string> strings_;
protected:
    StringSetParser() {}
public:
    typedef boost::shared_ptr<StringSetParser> Ptr;
    static Ptr instance() { return Ptr(new StringSetParser); } /**< Allocating constructor. */

    /** Adds string members.  Inserts an additional string to be recognized in the input.
     *  @{ */
    Ptr with(const std::string &s) { return with(&s, &s+1); }
    Ptr with(const std::vector<std::string> sv) { return with(sv.begin(), sv.end()); }
    template<class InputIterator>
    Ptr with(InputIterator begin, InputIterator end) {
        strings_.insert(strings_.end(), begin, end);
        return boost::dynamic_pointer_cast<StringSetParser>(shared_from_this());
    }
    /** @} */

private:
    virtual boost::any operator()(Cursor&) /*override*/;
};

/** Parses an enumerated constant. The template parameter is the enum type. Returns T. */
template<typename T>
class EnumParser: public ValueParser {
    StringSetParser::Ptr strParser_;
    std::map<std::string, T> members_;
protected:
    EnumParser(): strParser_(StringSetParser::instance()) {}
public:
    typedef boost::shared_ptr<EnumParser> Ptr;
    static Ptr instance() { return Ptr(new EnumParser); } /** Allocating constructor. */

    /** Adds enum members.  Inserts an additional enumeration constant and its string name. */
    Ptr with(const std::string &name, T value) {
        strParser_->with(name);
        members_[name] = value;
        return boost::dynamic_pointer_cast<EnumParser>(shared_from_this());
    }
private:
    virtual boost::any operator()(Cursor &cursor) /*override*/ {
        std::string str = boost::any_cast<std::string>(strParser_->match(cursor));
        return members_[str];
    }
};

/** Parses a list of values. The return value is an STL vector whose members are boost::any values returned by the value
 *  parsers.  A list parser may have one or more value parsers, the last of which is reused as often as necessary to parse the
 *  values of the list. */
class ListParser: public ValueParser {
    typedef std::pair<ValueParser::Ptr, std::string> ParserSep;
    std::vector<ParserSep> elements_;
    size_t minLength_, maxLength_;                      // limits on the number of values permitted
protected:
    ListParser(const ValueParser::Ptr &firstElmtType, const std::string &separatorRe)
        : minLength_(1), maxLength_((size_t)-1) {
        elements_.push_back(ParserSep(firstElmtType, separatorRe));
    }
public:
    typedef boost::shared_ptr<ListParser> Ptr;

    /** Allocating constructor.  The @p firstElmtType is the parser for the first value (and the remaining values also if no
     *  subsequent parser is specified), and the @p separatorRe is the regular expression describing how values of this type
     *  are separated from subsequent values. */
    static Ptr instance(const ValueParser::Ptr &firstElmtType, const std::string &separatorRe=",\\s*") {
        return Ptr(new ListParser(firstElmtType, separatorRe)); 
    }

    /** Specifies element type and separator. Adds another element type and separator to this parser.  The specified values
     *  are also used for all the following list members unless a subsequent type and separator are supplied.  I.e., the
     *  final element type and separator are repeated as necessary when parsing. */
    Ptr nextMember(const ValueParser::Ptr &elmtType, const std::string &separatorRe=",\\s*") {
        elements_.push_back(ParserSep(elmtType, separatorRe));
        return boost::dynamic_pointer_cast<ListParser>(shared_from_this());
    }

    /** Specify limits for the number of values parsed.  By default, a list parser parses zero or more values with no limit.
     *  The limit() method provides separate lower and upper bounds (the one argument version sets only the upper bound), and
     *  the exactly() method is a convenience to set the lower and upper bound to the same value.  @{ */
    Ptr limit(size_t minLength, size_t maxLength);
    Ptr limit(size_t maxLength) { return limit(std::min(minLength_, maxLength), maxLength); }
    Ptr exactly(size_t length) { return limit(length, length); }
    /** @} */
private:
    virtual boost::any operator()(Cursor&) /*override*/;
};
    

/** Factory function.  A factory function is a more terse way of calling the instance() static allocator.
 *  @{ */
AnyParser::Ptr anyParser();
IntegerParser::Ptr integerParser();
UnsignedIntegerParser::Ptr unsignedIntegerParser();
RealNumberParser::Ptr realNumberParser();
BooleanParser::Ptr booleanParser();
StringSetParser::Ptr stringSetParser();
ListParser::Ptr listParser(const ValueParser::Ptr&, const std::string &sepRe=",\\s*");

// Note: enum T must be defined at global scope
template<typename T>
typename EnumParser<T>::Ptr enumParser() {
    return EnumParser<T>::instance();
}
/** @} */


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Switch argument descriptors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Describes one argument of a command-line switch. Some switches may have more than one argument ("--swap a b"), or an
 *  argument that is a list ("--swap a,b"), or appear more than once to create a list ("--swap a --swap b"); these are
 *  described at the Switch level, not the SwitchArgument level.  An argument parses one single entity (although the "a,b" in
 *  "--swap a,b" could be handled as a single entity that is a list).  Users don't usually use this directly, but rather via
 *  the argument() factory. */
class SwitchArgument {
    std::string name_;                                  // argument name for synopsis
    ValueParser::Ptr parser_;                           // how to match and parse this argument
    boost::any defaultValue_;                           // default value if the argument is optional
    std::string defaultValueString_;                    // string from which the default value is parsed
public:
    SwitchArgument() {}

    /** Construct a new switch required argument. The @p name is used in documentation and error messages and need not be
     *  unique. */
    explicit SwitchArgument(const std::string &name, const ValueParser::Ptr &parser = anyParser())
        : name_(name), parser_(parser) {}

    /** Construct a new switch optional argument.  The @p name is used in documentation and error messages and need not be
     *  unique. The @p defaultValueString is immediately parsed via supplied parser and stored; an exception is thrown if it
     *  cannot be parsed. */
    SwitchArgument(const std::string &name, const ValueParser::Ptr &parser, const std::string &defaultValueString)
        : name_(name), parser_(parser), defaultValue_(parser->matchString(defaultValueString)),
          defaultValueString_(defaultValueString)  {}

    /** Returns true if this argument is required. */
    bool isRequired() const {
        return defaultValue_.empty();
    }

    /** Returns true if this argument is not required.  Any argument that is not a required argument will have a default value
     *  that is returned in its place when parsing the switch. */
    bool isOptional() const {
        return !isRequired();
    }

    /** Returns the name. The name is used for documentation and error messages. */
    const std::string &name() const { return name_; }

    /** The parsed default value.  Returns an empty boost::any if the argument is required, but since such values are also
     *  valid parsed default values, one should call isRequired() or isOptional() to make that determination. */
    const boost::any& defaultValue() const {
        return defaultValue_;
    }

    /** The default value string.  This is the string that was parsed to create the default value.  Returns an empty string if
     *  the argument is required, but since such values are also valid default values one should call isRequired() or
     *  isOptional() to make that determination. */
    const std::string& defaultValueString() const {
        return defaultValueString_;
    }

    /** Returns a pointer to the parser.  Parsers are reference counted and should not be explicitly destroyed. */
    const ValueParser::Ptr& parser() const { return parser_; }

    // FIXME[Robb Matzke 2014-02-21]: a way to store values directly into user-supplied areas rather than (or in addition to)
    // storing them in the ParserResults.
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Switch value agumenters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// FIXME[Robb Matzke 2014-02-21]: Finish this design

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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Switch immediate actions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// FIXME[Robb Matzke 2014-02-21]: Finish this design

class SwitchAction {
public:
    typedef boost::shared_ptr<SwitchAction> Ptr;
    virtual ~SwitchAction() {}
    void run() /*final*/ { (*this)(); }
private:
    virtual void operator()() = 0;                      // FIXME[Robb Matzke 2014-02-14]
};

class ExitProgram: public SwitchAction {
    int exitStatus_;
protected:
    explicit ExitProgram(int exitStatus): exitStatus_(exitStatus) {}
public:
    typedef boost::shared_ptr<ExitProgram> Ptr;
    static Ptr instance(int exitStatus) { return Ptr(new ExitProgram(exitStatus)); }
private:
    virtual void operator()() /*override*/;
};

class ShowVersion: public SwitchAction {
    std::string versionString_;
protected:
    explicit ShowVersion(const std::string &versionString): versionString_(versionString) {}
public:
    typedef boost::shared_ptr<ShowVersion> Ptr;
    static Ptr instance(const std::string &versionString) { return Ptr(new ShowVersion(versionString)); }
private:
    virtual void operator()() /*overload*/;
};

class ShowHelp: public SwitchAction {
    std::ostream &stream_;
protected:
    explicit ShowHelp(std::ostream &stream): stream_(stream) {}
public:
    typedef boost::shared_ptr<ShowHelp> Ptr;
    static Ptr instance(std::ostream &stream) { return Ptr(new ShowHelp(stream)); }
private:
    virtual void operator()() /*override*/;
};

ExitProgram::Ptr exitProgram(int exitStatus);
ShowVersion::Ptr showVersion(const std::string &versionString);
ShowHelp::Ptr showHelp(std::ostream &o=std::cerr);



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Parsed value
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Switch;

/** Information about a parsed switch value.
 *
 *  Each time a switch argument is parsed to create a value, whether it comes from the program command line or a default value
 *  string, a ParsedValue object is constructed to describe it.  These objects hold the value, the string from whence the value
 *  was parsed, and information about where the value came from and with which switch it is associated.  This class also
 *  provides a number of methods for conveniently and safely casting the value to other types. */
class ParsedValue {
    boost::any value_;
    Location valueLocation_;                            // where this value came from on the command-line; or NOWHERE if dflt
    std::string valueString_;                           // string representation of the value
    const Switch *switch_;                              // switch that parsed this value
    Location switchLocation_;                           // where is the actual switch name in switchString_?
    std::string switchString_;                          // prefix and switch name
    size_t keySequence_, switchSequence_;               // relation of this value w.r.t. other values for same key and switch

private:
    friend class Switch;
    friend class ParserResult;
    friend class Parser;

    ParsedValue(const boost::any value, const Location &loc, const std::string &str)
        : value_(value), valueLocation_(loc), valueString_(str),
          switch_(NULL), keySequence_(0), switchSequence_(0) {}

    // Update the switch information for the parsed value.
    ParsedValue& switchInfo(const Switch *sw, const Location &loc, const std::string &str) {
        switch_ = sw; switchLocation_ = loc; switchString_ = str;
        return *this;
    }

    // Update sequence info
    void sequenceInfo(size_t keySequence, size_t switchSequence) {
        keySequence_ = keySequence;
        switchSequence_ = switchSequence;
    }

public:
    /** Returns the parsed value.  Parsed values are represented by boost::any, which is capable of storing any type of parsed
     *  value including void.  This is the most basic access to the value; the class also provides a variety of casting
     *  accessors that are sometimes more convenient (their names start with "as"). */
    const boost::any& value() const { return value_; }

    /** Command-line location from whence this value came.  For values that are defaults which didn't come from the
     *  command-line, the constant NOWHERE is returned. */
    Location valueLocation() const { return valueLocation_; }

    /** String representation.  This is the value as it appeared on the command-line or as a default string. */
    const std::string &string() const { return valueString_; }

    /** Convenience cast.  This returns the value cast to the specified type.  Whereas boost::any_cast requires an exact type
     *  match for the cast to be successful, this method makes more of an effort to be successful.  It recognizes a variety of
     *  common types; less common types will need to be explicitly converted by hand.  In any case, the original string
     *  representation and parser are available if needed.
     * @{ */
    int asInt() const;
    unsigned asUnsigned() const;
    long asLong() const;
    unsigned long asUnsignedLong() const;
    boost::int64_t asInt64() const;
    boost::uint64_t asUnsigned64() const;
    double asDouble() const;
    float asFloat() const;
    bool asBool() const;
    std::string asString() const;
    /** @} */

    /** Convenient any_cast.  This is a slightly less verbose way to get the value and perform a boost::any_cast. */
    template<typename T> T as() const { return boost::any_cast<T>(value_); }

    /** The string for the switch that caused this value to be parsed.  This string includes the switch prefix and the switch
     *  name in order to allow programs to distinguish between the same switch occuring with two different prefixes (like the
     *  "-invert" vs "+invert" style which is sometimes used for Boolean-valued switches).
     *
     *  For nestled short switches, the string returned by this method doesn't necessarily appear anywhere in the program
     *  command line.  For instance, this method might return "-b" when the command line was "-ab", because "-a" is a different
     *  switch with presumably a different set of parsed values. */
    const std::string& switchString() const { return switchString_; }

    /** The command-line location of the switch to which this value belongs.  The return value indicates the start of the
     *  switch name after any leading prefix.  For nestled single-character switches, the location's command line argument
     *  index will be truthful, but the character offset will refer to the string returned by switchString(). */
    Location switchLocation() const { return switchLocation_; }

    // FIXME[Robb Matzke 2014-02-21]: better interface, perhaps reference counting or copying
    /** The switch that caused the value to be created.  Switches are referred to by pointers, which are only valid as long as
     *  the switch itself is allocated.  The pointer should be valid as long as the switch's SwitchGroup is still allocated and
     *  has not been modified since this SwitchValue was created.  This generally isn't a problem since switch groups are
     *  typically fully created before parsing takes place. */
    const Switch& createdBy() const { return *switch_; }

    /** How this value relates to others with the same key.  This method returns the sequence number for this value among all
     *  values created for the same key. */
    size_t keySequence() const { return keySequence_; }

    /** How this value relates to others created by the same switch.  This method returns the sequence number for this value
     *  among all values created for the same switch. */
    size_t switchSequence() const { return switchSequence_; }

    // Print some debug info
    void print(std::ostream&) const;

    // FIXME[Robb Matzke 2014-02-21]: A switch to receive the non-switch program arguments?
};

std::ostream& operator<<(std::ostream&, const ParsedValue&);

/** A vector of parsed values. */
typedef std::vector<ParsedValue> ParsedValues;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Switch descriptors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Used internally to pass around switch properties that are common among parsers, switch groups, and switches.
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

/** Describes one command-line switch.
 *
 *  A command line switch is something that typically starts with a hyphen on Unix or a slash on Windows, followed by a name,
 *  and zero or more values.  Switches refer to named command-line arguments but not usually positional command-line
 *  arguments.
 *
 *  The user normally interacts with the Switch class by instantiating a new switch object and setting properties for that
 *  object before inserting it into a SwitchGroup.  All property-setting methods return a reference to the same switch and are
 *  typically chained together. In fact, the switch is often immediately inserted into the SwitchGroup without even creating a
 *  permanent switch object:
 *
 * @code
 *  SwitchGroup outputSwitches;
 *  outputSwitches.insert(Switch("use-color", "C")
 *                        .argument("when",                         // argument name
 *                                  (enumParser<WhenColor>()        // WhenColor must be declared at global scope
 *                                   ->with("never",  NEVER)        // possible values
 *                                   ->with("auto",   AUTO)
 *                                   ->with("always", ALWAYS)),
 *                                  "always"));                     // the default value
 * @endcode
 */
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
    std::vector<SwitchAction::Ptr> actions_;            // what happens as soon as the switch is parsed
    Save save_;                                         // which switch values should be saved
    ValueAugmenterPtr augmenter_;                       // used if save_==SAVE_AUGMENT
    boost::any defaultValue_;                           // default when no arguments are present
    std::string defaultValueString_;                    // string version of defaultValue_ (before parsing)

public:
    /** Switch declaration constructor.  Every switch must have either a long or short name (or both).  Neither name should
     *  include prefix characters such as hyphens, and the short name should be one character (or empty if none).  The best
     *  practice is to provide a long name for every switch and short names only for the most commonly used switches.  The
     *  constructor should provide the canonical names when there are more than one (additional names can be specified with the
     *  longName() and shortName() methods) because the names specified in the constructor are used to create keys (although
     *  the key() method can override this).  Names of switches need not be unique.  If more than one switch is able to parse a
     *  command-line argument then the first declaration wins--this includes being able to parse its arguments.  This feature
     *  is sometimes used to declare two switches with the same name but which take different types of arguments. */
    explicit Switch(const std::string &longName, char shortName='\0')
        : hidden_(false), save_(SAVE_ALL), defaultValue_(true), defaultValueString_("true") {
        init(longName, shortName);
    }

    /** Long name of the switch. Either adds another name to the list of permissible names, or returns a vector of permissible
     * names. The primary name is the first name and is usually specified in the constructor.
     * @{ */
    Switch& longName(const std::string &name);
    const std::string& longName() const { return longNames_.front(); }
    const std::vector<std::string>& longNames() const { return longNames_; }
    /** @} */

    /** Short name of the switch. If an argument is specified it is added to the list of permissible short names. With no
     *  arguments, this method returns a string containing all the valid short names  in the order they were declared.
     * @{ */
    Switch& shortName(char c) { if (c) shortNames_ += std::string(1, c); return *this; }
    const std::string& shortNames() const { return shortNames_; }
    /** @} */

    /** Key used when parsing this switch.  When a switch value is parsed (or a default value used) to create a ParsedValue
     *  object, the object will be associated with the switch key.  This allows different switches to write to the same result
     *  locations and is useful for different keys that refer to the same concept, like "--verbose" and "--quiet". The default
     *  is to use the long switch name if specified in the constructor, or else the single letter name name specified in the
     *  constructor (one or the other must be present).  The switch prefix (e.g., "-") should generally not be part of the
     *  key.
     * @{ */
    Switch& key(const std::string &s) { key_ = s; return *this; }
    const std::string &key() const { return key_; }
    /** @} */

    // FIXME[Robb Matzke 2014-02-21]: not implemented yet
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

    // FIXME[Robb Matzke 2014-02-21]: markup needs to be documented
    /** Detailed description.  This is the description of the switch in a simple markup language.  See [[markup]] for details.
     * @{ */
    Switch& doc(const std::string &s) { documentation_ = s; return *this; }
    const std::string& doc() const { return documentation_; }
    /** @} */

    // FIXME[Robb Matzke 2014-02-21]: not implemented yet
    /** Key to control order of documentation.  Normally, documentation for a group of switches is sorted according to the
     *  long name of the switch (or the short name when there is no long name).  Specifying a key causes the key string to be
     *  used instead of the switch names.
     *  @{ */
    Switch& docKey(const std::string &s) { documentationKey_ = s; return *this; }
    const std::string &docKey() const { return documentationKey_; }
    /** @} */
    
    // FIXME[Robb Matzke 2014-02-21]: not implemented yet
    /** Whether this switch appears in documentation. A hidden switch still participates when parsing command lines, but will
     *  not show up in documentation.  This is ofen used for a switch when that switch is documented as part of some other
     *  switch. */
    Switch& hidden(bool b) { hidden_ = b; return *this; }
    bool hidden() const { return hidden_; }

    /** Prefix strings for long names.  A long name prefix is the characters that introduce a long switch, usually "--" and
     *  sometimes "-" or "+"). Prefixes are specified in the parser, the switch group, and the switch, each entity inheriting
     *  and augmenting the list from the earlier entity.  To prevent inheritance use the resetLongPrefixes() method, which also
     *  takes a list of new prefixes.  In any case, additional prefixes can be added with longPrefix(), which should be after
     *  resetLongPrefixes() if both are used.  The empty string is a valid prefix and is sometimes used in conjunction with "="
     *  as a value separator to describe switches of the form "bs=1024".
     *
     *  It is generally unwise to override prefixes without inheritance since the parser itself chooses the basic prefixes
     *  based on the conventions used by the operating system.  Erasing these defaults might get you a command line syntax
     *  that's jarring to uses.
     * @{ */
    Switch& resetLongPrefixes(const std::string &s1, const std::string &s2=STR_NONE, const std::string &s3=STR_NONE,
                              const std::string &s4=STR_NONE);
    Switch& longPrefix(const std::string &s1) { properties_.longPrefixes.push_back(s1); return *this; }
    const std::vector<std::string>& longPrefixes() const { return properties_.longPrefixes; }
    /** @} */

    /** Prefix strings for short names.  A short name prefix is the characters that introduce a short switch, usually
     *  "-". Prefixes are specified in the parser, the switch group, and the switch, each entity inheriting and augmenting the
     *  list from the earlier entity.  To prevent inheritance use the resetShortPrefixes() method, which also 
     *  takes a list of new prefixes.  In any case, additional prefixes can be added with shortPrefix(), which should be after
     *  resetShortPrefixes() if both are used.  The empty string is a valid short prefix to be able to parse tar-like switches
     *  like "xvf".
     *
     *  It is generally unwise to override prefixes without inheritance since the parser itself chooses the basic prefixes
     *  based on the conventions used by the operating system.  Erasing these defaults might get you a command line syntax
     *  that's jarring to uses.
     * @{ */
    Switch& resetShortPrefixes(const std::string &s1, const std::string &s2=STR_NONE, const std::string &s3=STR_NONE,
                               const std::string &s4=STR_NONE);
    Switch& shortPrefix(const std::string &s1) { properties_.shortPrefixes.push_back(s1); return *this; }
    const std::vector<std::string>& shortPrefixes() const { return properties_.shortPrefixes; }
    /** @} */

    /** Strings that separate a long switch from its value. A value separator is the string that separates a long switch name
     *  from its value and is usually "=" and/or " ".  The " " string has special meaning: it indicates that the value must be
     *  separated from the switch by being in the following command line argument.  Separators are specified in the parser, the
     *  switch group, and the switch, each entity inheriting and augmenting the list from the earlier entity.  To prevent
     *  inheritance use the resetValueSeparators() method, which also takes a list of new separators.  In any case, additional
     *  separators can be added with valueSeparator(), which should be after resetValueSeparators() if both are used.  The
     *  empty string is a valid separator although typically not used since it leads to things like "--authormatzke" as an
     *  alternative to "--author=matzke".
     *
     *  It is generally unwise to override the separators without inheritance since the parser itself chooses the basic
     *  separators based on the conventions used by the operating system.  Erasing these defaults might get you a command line
     *  syntax that's jarring to uses.
     * @{ */
    Switch& resetValueSeparators(const std::string &s1=STR_NONE, const std::string &s2=STR_NONE,
                                 const std::string &s3=STR_NONE, const std::string &s4=STR_NONE);
    Switch& valueSeparator(const std::string &s1) { properties_.valueSeparators.push_back(s1); return *this; }
    const std::vector<std::string>& valueSeparators() const { return properties_.valueSeparators; }
    /** @} */

    /** Switch argument.  A switch argument declares how text after the switch name is parsed to form a value.  If a default is
     *  specified then the argument will be optional and the parser will behave as if the default value string appeared on the
     *  command line at the point where the argument was expected.  This also means that the default value string must be
     *  parsable as if it were truly present.  The default parser, an instance of AnyParser, will allow the argument to be any
     *  non-empty string stored as an std::string.
     *
     *  Although switches usually have either no arguments or one argument, it is possible to declare switches that have
     *  multiple arguments.  This is different from a switch that occurs multiple times or from a switch that has one argument
     *  which is a list.  Each switch argument must be separated from the previous switch argument by occuring in a subsequent
     *  program command line argument.  E.g., "--swap a b" as three program arguments, or "--swap=a b" as two program
     *  arguments; but "--swap=a,b" is one switch argument that happens to look like a list, and "--swap=a --swap=b" is
     *  obviously two switches with one switch argument each.
     *
     *  The @p name is used in documentation and error messages and need not be unique.
     *  @{ */
    Switch& argument(const std::string &name, const ValueParser::Ptr &p = anyParser());
    Switch& argument(const std::string &name, const ValueParser::Ptr&, const std::string &defaultValue);
    Switch& argument(const SwitchArgument &arg) { arguments_.push_back(arg); return *this; }
    size_t nArguments() const { return arguments_.size(); }
    size_t nRequiredArguments() const;
    const SwitchArgument& argument(size_t idx) const { return arguments_[idx]; }
    const std::vector<SwitchArgument>& arguments() const { return arguments_; }
    /** @} */

    /** The default value for a switch that has no arguments declared.  A switch with no declared arguments is always parsed as
     *  if it had one argument, a value of "true" parsed and stored as type "bool" by the BooleanParser.  The defaultValue()
     *  can specify a different value and type.  For instance, "--laconic" and "--effusive" might be two switches that use the
     *  same key "verbosity" and have default values of "1" and "2" as integers (or even "laconic" and "effusive" as enums).
     *
     *  If a switch has at least one declared argument then this property is not consulted, even if that argument is optional
     *  and missing (in which case that argument's default is used).
     * @{ */
    Switch& defaultValue(const std::string &text, const ValueParser::Ptr &p = anyParser());
    boost::any defaultValue() const { return defaultValue_; }
    std::string defaultValueString() const { return defaultValueString_; }
    /** @} */

    /** Action to occur each time a switch is parsed.  All switch occurrences cause a ParserResult object to be modified in
     *  some way and eventually returned by the parser, which the user then queries.  However, sometimes it's necessary for a
     *  switch to also cause something to happen as soon as it's recognized, and that's the purpose of this property. The
     *  specified actions are invoked in the order they were declared after the switch and its arguments are parsed.
     *
     *  The library provides a few actions, all of which are derived from SwitchAction, and the user can provide additional
     *  actions.  For instance, two commonly used switches are:
     *
     * @code
     *  sg.insert(Switch("help", 'h')             // allow "--help" and "-h"
     *            .shortName('?')                 // also "-?"
     *            .action(showHelp(std::cout))    // show documentation
     *            .action(exitProgram(0)));       // and then exit
     *  sg.insert(Switch("version", 'V')          // allow "--version" and "-V"
     *            .action(showVersion("1.2.3"))); // emit "1.2.3"
     * @endcode
     * @{ */
    Switch& action(const SwitchAction::Ptr&);
    const std::vector<SwitchAction::Ptr>& actions() const { return actions_; }
    /** @} */

    // FIXME[Robb Matzke 2014-02-21]: not implemented yet
    /** Describes what to do if a switch occurs more than once.  Normally, if a switch occurs more than once then all of its
     * values are made available in the result. Setting this property to one of its other possibilities can make make it easier
     * to do things like using only the last value, causing an error, or avoiding occurrences of two switches that are mutually
     * exclusive.
     *
     * The save() method is the general mechanism for adjusting this property; the others are just conveniences.
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

    // FIXME[Robb Matzke 2014-02-21]: not implemented yet
    /** The functor to call when augmenting a previously saved value.
     *  @{ */
    Switch& augmentValue(const ValueAugmenterPtr &f) { augmenter_ = f; return *this; }
    ValueAugmenterPtr augmentValue() const { return augmenter_; }
    /** @} */

private:
    friend class Parser;

    void init(const std::string &longName, char shortName);

    const ParsingProperties& properties() const { return properties_; }

    // Constructs an exception describing that there is no separator between the switch name and its value.
    std::runtime_error noSeparator(const std::string &switchString, const Cursor&, const ParsingProperties&) const;

    // Constructs an exception describing that there is unexpected extra text after a switch argument.
    std::runtime_error extraTextAfterArgument(const std::string &switchString, const Cursor&) const;

    // Constructs an exception describing that we couldn't parse all the required arguments
    std::runtime_error notEnoughArguments(const std::string &switchString, const Cursor&, size_t nargs) const;

    // Constructs an exception describing an argument that is missing.
    std::runtime_error missingArgument(const std::string &switchString, const Cursor &cursor,
                                       const SwitchArgument &sa, const std::string &reason) const;

    // Determines if this switch can match against the specified program argument when considering only this switch's long
    // names.  If program argument starts with a valid long name prefix and then matches the switch name, this this function
    // returns true (the number of characters matched).  Switches that take no arguments must match to the end of the string,
    // but switches that have arguments (even if they're optional or wouldn't match the rest of the string) do not have to
    // match entirely as long as a value separator is found when they don't match entirely.
    size_t matchLongName(Cursor&/*in,out*/, const ParsingProperties &props) const;

    // Matches a short switch name.  Although the cursor need not be at the beginning of the program argument, this method
    // first matches a short name prefix at the beginning of the argument.  If the prefix ends at or before the cursor then the
    // prefix must be immediately followed by a single-letter switch name, otherwise the prefix may be followed by one or more
    // characters before a switch name is found exactly at the cursor.  In any case, if a name is found the cursor is advanced
    // to the character after the name.  The returned name is the prefix and the character even if they don't occur next to
    // each other in the program argument.
    size_t matchShortName(Cursor&/*in,out*/, const ParsingProperties &props, std::string &name) const;

    // Parse long switch arguments when the cursor is positioned immediately after the switch name.  This matches the
    // switch/value separator if necessary and subsequent switch arguments.
    void matchLongArguments(const std::string &switchString, Cursor &cursor /*in,out*/, const ParsingProperties &props,
                            ParsedValues &result /*out*/) const;

    // Parse short switch arguments when the cursor is positioned immediately after the switch name.
    void matchShortArguments(const std::string &switchString, Cursor &cursor /*in,out*/, const ParsingProperties &props,
                             ParsedValues &result /*out*/) const;

    // Parses switch arguments from the command-line arguments to obtain switch values.  Upon entry, the cursor should point to
    // the first character after the switch name; it will be updated to point to the first position after the last parsed
    // argument upon return.  If anything goes wrong an exception is thrown (cursor and result might be invalid).  Returns the
    // number of argument values parsed, not counting those created from default values.
    size_t matchArguments(const std::string &switchString, Cursor &cursor /*in,out*/, ParsedValues &result /*out*/) const;

    // Run the actions associated with this switch.
    void runActions() const;

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
    std::vector<std::string> argv_;                     // does not include main's argv[0]
    ParsedValues values_;
    Location stoppedAt_;                                // where parsing stopped

    // List of parsed values organized by key.  The integers in this map are indexes into the values_ vector.
    typedef std::map<std::string /*keyname*/, std::vector<size_t> > KeyIndex;
    KeyIndex keyIndex_;

    // List of parsed values organized by switch. The integers are indexes into the values_ vector.
    typedef std::map<const Switch*, std::vector<size_t> > SwitchIndex;
    SwitchIndex switchIndex_;

    // List of parsed values organized by their location on the command line.  The location is for the switch itself even if
    // the values are spread out across subsequent argv members. We do it this way because many of the values are defaults that
    // don't actually have an argv location.  The integers are indexes into the values_ vector.
    typedef std::map<Location, std::vector<size_t> > ArgvIndex;
    ArgvIndex argvIndex_;

    // Information about program arguments that the parser skipped over.
    typedef std::vector<size_t> SkippedIndex;
    SkippedIndex skippedIndex_;

    // Information about terminator switches like "--"
    SkippedIndex terminators_;

public:
    ParserResult(const std::vector<std::string> &argv): argv_(argv) {}

    // Returns the number of values parsed for the specified key.
    size_t have(const std::string &switchKey) { return keyIndex_[switchKey].size(); }

    // Returns values by key. Example the first value from switches that use the "verbose" key.
    //    bool b = cmdline.parsedKey("verbose")[0].valueAsBool();
    const ParsedValue& parsed(const std::string &switchKey, size_t idx);
    ParsedValues parsed(const std::string &switchKey);

    // Program arguments that were skipped over during parsing.  File inclusions are expanded.  Arguments skipped for any
    // reason are lumped together because we really don't know what they really are when they don't look like switches--they
    // could be switch arguments that couldn't be parsed for some reason (e.g., a switch that takes an optional integer
    // argument but a non-integer string was supplied), they could be switch arguments for a switch that we didn't recognize
    // (e.g., user entered "--auhter washington" instead of "--author washington"), or they could be program arguments that are
    // not switch arguments.
    std::vector<std::string> skippedArgs() const;

    // The arguments that were not parsed.
    std::vector<std::string> remainingArgs() const;

    // The skipped() and remaining() arguments along with termination switches.  This is basically the original program command
    // line with the parsed stuff removed.
    std::vector<std::string> unparsedArgs() const;

    // Program arguments that were parsed.
    std::vector<std::string> parsedArgs() const;

    // The original command line, except with command-line inclusion expanded.
    const std::vector<std::string>& allArgs() const { return argv_; }


public:                                                 // used internally FIXME[Robb Matzke 2014-02-19]
    // High water mark for parsing
    void stoppedAt(const Location &loc) { stoppedAt_ = loc; }
    const Location& stoppedAt() const { return stoppedAt_; }

    // Insert more parsed values. Also updates sequence numbers in the argument.
    void insert(ParsedValues&);

    // Indicate that we're skipping over a program argument
    void skip(const Location&);

    // Add a terminator
    void terminator(const Location&);

};


class Parser {
    std::vector<SwitchGroup> switchGroups_;
    ParsingProperties properties_;
    std::vector<std::string> terminationSwitches_;      // special switch to terminate parsing; default is "--"
    bool shortMayNestle_;                               // is "-ab" the same as "-a -b"?
    std::vector<std::string> inclusionPrefixes_;        // prefixes that mark command line file inclusion (e.g., "@")
    size_t maxInclusionDepth_;                          // max depth to prevent recursive inclusion
    bool skipNonSwitches_;                              // skip over non-switch arguments?
    bool skipUnknownSwitches_;                          // skip over switches we don't recognize?
public:
    Parser()
        : shortMayNestle_(true), maxInclusionDepth_(10), skipNonSwitches_(false), skipUnknownSwitches_(false) {
        init();
    }
    
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

    // Whether to skip over non-switch arguments when parsing.  If false, parsing stops at the first non-switch, otherwise
    // non-switches are simply skipped over and added to the parsing result that's eventually returned. In either case, parsing
    // stops when a terminator switch (usually "--") is found. Anything that looks like a switch but doesn't match a
    // declaration continues to result in an error regardless of this property.
    Parser& skipNonSwitches(bool b=true) { skipNonSwitches_ = b; return *this; }
    bool skippingNonSwitches() const { return skipNonSwitches_; }

    // Whether to skip over unrecognized switches.  An unrecognized switch is any program argument that looks like a switch
    // (see apparentSwitch()) but which doesn't match the name of any declared switch.  When not skipping (the default) such
    // program arguments cause an "unrecognized switch" exception.
    Parser& skipUnknownSwitches(bool b=true) { skipUnknownSwitches_ = b; return *this; }
    bool skippingUnkownSwitches() const { return skipUnknownSwitches_; }

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
    // the end of the command line then return false without updating the cursor or parsed values.  If the cursor is at a
    // termination switch (e.g., "--") then consume the terminator and return false.  If the switch name is valid but the
    // arguments cannot be parsed, then throw an error.  If the cursor is at what appears to be a switch but no matching switch
    // declaration can be found, then throw an error.  The cursor will not be modified when an error is thrown.
    bool parseOneSwitch(Cursor&, ParserResult&/*out*/);

    // Parse one short switch if possible.
    const Switch* parseShortSwitch(Cursor &cursor, ParserResult&/*out*/, boost::optional<std::runtime_error>&/*out*/);

    // Returns true if the program argument at the cursor looks like it might be a switch.  Apparent switches are any program
    // argument that starts with a long or short prefix.
    bool apparentSwitch(const Cursor&) const;

    // FIXME[Robb Matzke 2014-02-21]: Some way to parse command-lines from a config file, or to merge parsed command-lines with
    // a yaml config file, etc.

private:
    void init();
};

} // namespace
} // namespace

#endif

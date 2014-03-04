#include "Assert.h"
#include "CommandLine.h"
#include "MarkupRoff.h"
#include "Message.h"

#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>
#include <cerrno>
#include <set>
#include <sstream>
#include <termio.h>
#include <sys/ioctl.h>

namespace Sawyer {
namespace CommandLine {

const std::string STR_NONE(" %-NONE^}");                // arbitrary, but unusual
const Location NOWHERE(-1, -1);

static bool matchAnyString(const std::vector<std::string> &strings, const std::string &toMatch) {
    BOOST_FOREACH (const std::string &string, strings) {
        if (0==string.compare(toMatch))
            return true;
    }
    return false;
}

std::ostream& operator<<(std::ostream &o, const Location &x) {
    if (x == NOWHERE) {
        o <<"nowhere";
    } else {
        o <<x.idx <<"." <<x.offset;
    }
    return o;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Cursor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string Cursor::substr(const Location &limit1, const Location &limit2, const std::string &separator) {
    std::string retval;
    Location begin = limit1, end = limit2;
    if (end < begin)
        std::swap(begin, end);
    while (begin < end) {
        if (begin.idx < end.idx) {
            retval += rest(begin);
            if (end.offset)
                retval += separator;
            ++begin.idx;
            begin.offset = 0;
        } else {
            retval += rest(begin).substr(0, end.offset-begin.offset);
            break;
        }
    }
    return retval;
}

Cursor& Cursor::location(const Location &loc) {
    loc_ = loc;
    if (loc_.idx >= strings_.size()) {
        loc_.offset = 0;
    } else {
        while (loc_.idx < strings_.size() && loc_.offset >= strings_[loc_.idx].size()) {
            ++loc_.idx;
            loc_.offset = 0;
        }
    }
    return *this;
}

void Cursor::consumeChars(size_t nchars) {
    while (nchars > 0 && loc_.idx < strings_.size()) {
        ASSERT_require(strings_[loc_.idx].size() >= loc_.offset);
        size_t n = std::min(nchars, (strings_[loc_.idx].size() - loc_.offset));
        loc_.offset += n;
        nchars -= n;
        while (loc_.idx < strings_.size() && loc_.offset >= strings_[loc_.idx].size()) {
            ++loc_.idx;
            loc_.offset = 0;
        }
    }
}

void Cursor::replace(const std::vector<std::string> &args) {
    ASSERT_forbid(atEnd());
    std::vector<std::string>::iterator at = strings_.begin() + loc_.idx;
    at = strings_.erase(at);
    strings_.insert(at, args.begin(), args.end());
    Location newloc = loc_;
    newloc.offset = 0;
    location(newloc);
}

size_t Cursor::linearDistance() const {
    size_t retval = 0;
    for (size_t i=0; i<loc_.idx; ++i)
        retval += strings_[i].size();
    retval += loc_.offset;
    return retval;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Parsers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ParsedValue ValueParser::matchString(const std::string &str) {
    Cursor cursor = str;
    ParsedValue retval = match(cursor);
    if (cursor.atArgBegin())
        throw std::runtime_error("not matched");
    if (!cursor.atEnd())
        throw std::runtime_error("extra text after end of value");
    return retval;
}

ParsedValue ValueParser::match(Cursor &cursor) {
    return (*this)(cursor);
}

// Only called by match().  If the subclass doesn't override this, then we try calling the C-string version instead.
ParsedValue ValueParser::operator()(Cursor &cursor) {
    std::string str = cursor.rest();
    const char *s = str.c_str();
    const char *rest = s;
    ParsedValue retval = (*this)(s, &rest, cursor.location());
    if (NULL!=rest) {
        ASSERT_require(rest>=s && rest<= s+strlen(s));
        cursor.consumeChars(rest-s);
    }
    return retval;
}

// only called by ValueParser::operator()(Cursor&)
ParsedValue ValueParser::operator()(const char *s, const char **rest, const Location &loc) {
    throw std::runtime_error("subclass must implement an operator() with a cursor or C strings");
}

ParsedValue AnyParser::operator()(Cursor &cursor) {
    Location startLoc = cursor.location();
    std::string s = cursor.rest();
    cursor.consumeArg();
    return ParsedValue(s, startLoc, s, valueSaver());
}

ParsedValue StringSetParser::operator()(Cursor &cursor) {
    Location locStart = cursor.location();
    std::string input = cursor.rest();
    size_t bestMatchIdx = (size_t)(-1), bestMatchLen = 0;
    for (size_t i=0; i<strings_.size(); ++i) {
        if (boost::starts_with(input, strings_[i]) && ((size_t)(-1)==bestMatchIdx || strings_[i].size()>bestMatchLen)) {
            bestMatchIdx = i;
            bestMatchLen = strings_[i].size();
        }
    }
    if ((size_t)(-1)==bestMatchIdx)
        throw std::runtime_error("specific word expected");
    cursor.consumeChars(bestMatchLen);
    return ParsedValue(strings_[bestMatchIdx], locStart, strings_[bestMatchIdx], valueSaver());
}

ListParser::Ptr ListParser::limit(size_t minLength, size_t maxLength) {
    if (minLength > maxLength)
        throw std::runtime_error("minimum ListParser length must be less than or equal to maximum length");
    minLength_ = minLength;
    maxLength_ = maxLength;
    return boost::dynamic_pointer_cast<ListParser>(shared_from_this());
}

ParsedValue ListParser::operator()(Cursor &cursor) {
    ASSERT_forbid(elements_.empty());
    Location startLoc = cursor.location();
    ExcursionGuard guard(cursor);                       // parsing the list should be all or nothing
    ValueList values;
    std::string sep = "";

    for (size_t i=0; i<maxLength_; ++i) {
        const ParserSep &ps = elements_[std::min(i, elements_.size()-1)];

        // Advance over the value separator
        if (0!=i) {
            if (cursor.atArgBegin() || cursor.atEnd())
                break;                                  // we've advanced over the entire program argument
            std::string str = cursor.rest();
            const char *s = str.c_str();
            boost::regex re("\\A(" + sep + ")");
            boost::cmatch matched;
            if (!regex_search(s, matched, re))
                break;
            cursor.consumeChars(matched.str().size());
        }
        sep = ps.second;

        // Find the next value separator so we can prevent from parsing through it
        size_t endOfValue = cursor.rest().size();
        {
            boost::regex re(sep);
            boost::cmatch matched;
            std::string str = cursor.rest();
            const char *s = str.c_str();
            if (regex_search(s, matched, re))
                endOfValue = matched.position();
        }

        // Parse the value, stopping before the next separator
        Cursor valueCursor(cursor.rest().substr(0, endOfValue));
        ParsedValue value = ps.first->match(valueCursor);
        value.valueLocation(cursor.location());
        cursor.consumeChars(valueCursor.linearDistance());
        values.push_back(value);
    }

    if (values.size()<minLength_ || values.size()>maxLength_) {
        std::ostringstream ss;
        if (minLength_ == maxLength_) {
            ss <<"list with " <<maxLength_ <<" element" <<(1==maxLength_?"":"s") <<" expected (got " <<values.size() <<")";
            throw std::runtime_error(ss.str());
        } else if (minLength_+1 == maxLength_) {
            ss <<"list with " <<minLength_ <<" or " <<maxLength_ <<" element" <<(1==maxLength_?"":"s") <<" expected"
               <<" (got " <<values.size() <<")";
            throw std::runtime_error(ss.str());
        } else {
            std::ostringstream ss;
            ss <<"list with " <<minLength_ <<" to " <<maxLength_ <<" elements expected (got " <<values.size() <<")";
            throw std::runtime_error(ss.str());
        }
    }

    guard.cancel();
    return ParsedValue(values, startLoc, cursor.substr(startLoc), valueSaver());
}

AnyParser::Ptr anyParser(std::string &storage) {
    return AnyParser::instance(TypedSaver<std::string>::instance(storage));
}

AnyParser::Ptr anyParser() {
    return AnyParser::instance();
}

StringSetParser::Ptr stringSetParser(std::string &storage) {
    return StringSetParser::instance(TypedSaver<std::string>::instance(storage));
}

StringSetParser::Ptr stringSetParser() {
    return StringSetParser::instance();
}

ListParser::Ptr listParser(const ValueParser::Ptr &p, const std::string &sep) {
    return ListParser::instance(p, sep);
}

/*******************************************************************************************************************************
 *                                      Actions
 *******************************************************************************************************************************/

void ExitProgram::operator()(const Parser*) {
    exit(exitStatus_);
}

void ShowVersion::operator()(const Parser*) {
    std::cerr <<versionString_ <<"\n";
}

void ShowHelp::operator()(const Parser *parser) {
    parser->emitDocumentationToPager();
}

ExitProgram::Ptr exitProgram(int exitStatus) { return ExitProgram::instance(exitStatus); }
ShowVersion::Ptr showVersion(const std::string &versionString) { return ShowVersion::instance(versionString); }
ShowHelp::Ptr showHelp() { return ShowHelp::instance(); }


/*******************************************************************************************************************************
 *                                      Parsed values
 *******************************************************************************************************************************/

// A variety of common integer types
template<typename T>
static T fromSigned(const boost::any &v) {
    if (v.type() == typeid(boost::int64_t)) {
        return boost::any_cast<boost::int64_t>(v);
    } else if (v.type() == typeid(long long)) {
        return boost::any_cast<long long>(v);
    } else if (v.type() == typeid(long)) {
        return boost::any_cast<long>(v);
    } else if (v.type() == typeid(int)) {
        return boost::any_cast<int>(v);
    } else if (v.type() == typeid(short)) {
        return boost::any_cast<short>(v);
    } else if (v.type() == typeid(signed char)) {
        return boost::any_cast<signed char>(v);
    } else {
        return boost::any_cast<T>(v);
    }
}

template<typename T>
static T fromUnsigned(const boost::any &v) {
    if (v.type() == typeid(boost::uint64_t)) {
        return boost::any_cast<boost::uint64_t>(v);
    } else if (v.type() == typeid(unsigned long long)) {
        return boost::any_cast<unsigned long long>(v);
    } else if (v.type() == typeid(unsigned long)) {
        return boost::any_cast<unsigned long>(v);
    } else if (v.type() == typeid(unsigned int)) {
        return boost::any_cast<unsigned int>(v);
    } else if (v.type() == typeid(unsigned short)) {
        return boost::any_cast<unsigned short>(v);
    } else if (v.type() == typeid(unsigned char)) {
        return boost::any_cast<unsigned char>(v);
    } else if (v.type() == typeid(size_t)) {
        return boost::any_cast<size_t>(v);
    } else {
        return boost::any_cast<T>(v);
    }
}

template<typename T>
static T fromInteger(const boost::any &v) {
    try {
        return fromSigned<T>(v);
    } catch (const boost::bad_any_cast&) {
    }
    try {
        return fromUnsigned<T>(v);
    } catch (const boost::bad_any_cast&) {
    }
    if (v.type() == typeid(bool)) {
        return boost::any_cast<bool>(v);
    } else {
        return boost::any_cast<T>(v);                   // try blind luck
    }
}

// A variety of common floating point types
template<typename T>
T fromFloatingPoint(const boost::any &v) {
    if (v.type() == typeid(double)) {
        return boost::any_cast<double>(v);
    } else if (v.type() == typeid(float)) {
        return boost::any_cast<float>(v);
    } else {
        return fromInteger<T>(v);
    }
}

int ParsedValue::asInt() const {
    return fromInteger<int>(value_);
}

unsigned ParsedValue::asUnsigned() const {
    return fromInteger<unsigned>(value_);
}

long ParsedValue::asLong() const {
    return fromInteger<long>(value_);
}

unsigned long ParsedValue::asUnsignedLong() const {
    return fromInteger<unsigned long>(value_);
}

boost::int64_t ParsedValue::asInt64() const {
    return fromInteger<boost::int64_t>(value_);
}

boost::uint64_t ParsedValue::asUnsigned64() const {
    return fromInteger<boost::int64_t>(value_);
}

double ParsedValue::asDouble() const {
    return fromFloatingPoint<double>(value_);
}

float ParsedValue::asFloat() const {
    return fromFloatingPoint<float>(value_);
}

bool ParsedValue::asBool() const {
    return fromInteger<boost::uint64_t>(value_);
}

std::string ParsedValue::asString() const {
    try {
        boost::int64_t x = fromSigned<boost::int64_t>(value_);
        return boost::lexical_cast<std::string>(x);
    } catch (const boost::bad_any_cast&) {
    }
    try {
        boost::uint64_t x = fromUnsigned<boost::int64_t>(value_);
        return boost::lexical_cast<std::string>(x);
    } catch (const boost::bad_any_cast&) {
    }
    try {
        double x = fromFloatingPoint<double>(value_);
        return boost::lexical_cast<std::string>(x);
    } catch (const boost::bad_any_cast&) {
    }
    try {
        bool x = boost::any_cast<bool>(value_);
        return boost::lexical_cast<std::string>(x);
    } catch (const boost::bad_any_cast&) {
    }
    std::string x = boost::any_cast<std::string>(value_);
    return x;
}

void ParsedValue::save() const {
    if (valueSaver_)
        valueSaver_->save(value_);

    if (value_.type() == typeid(ListParser::ValueList)) {
        const ListParser::ValueList &values = boost::any_cast<ListParser::ValueList>(value_);
        BOOST_FOREACH (const ParsedValue &pval, values)
            pval.save();
    }
}

void ParsedValue::print(std::ostream &o) const {
    o <<"{switch=\"" <<switchString_ <<"\" at " <<switchLocation_ <<" key=\"" <<switchKey_ <<"\""
      <<"; value str=\"" <<valueString_ <<"\" at " <<valueLocation_
      <<"; seq={s" <<switchSequence_ <<", k" <<keySequence_ <<"}"
      <<"}";
}

std::ostream& operator<<(std::ostream &o, const ParsedValue &x) {
    x.print(o);
    return o;
}


/*******************************************************************************************************************************
 *                                      Switch Descriptors
 *******************************************************************************************************************************/

ParsingProperties ParsingProperties::inherit(const ParsingProperties &base) const {
    ParsingProperties retval;
    if (inheritLongPrefixes)
        retval.longPrefixes = base.longPrefixes;
    retval.longPrefixes.insert(retval.longPrefixes.end(), longPrefixes.begin(), longPrefixes.end());
    if (inheritShortPrefixes)
        retval.shortPrefixes = base.shortPrefixes;
    retval.shortPrefixes.insert(retval.shortPrefixes.end(), shortPrefixes.begin(), shortPrefixes.end());
    if (inheritValueSeparators)
        retval.valueSeparators = base.valueSeparators;
    retval.valueSeparators.insert(retval.valueSeparators.end(), valueSeparators.begin(), valueSeparators.end());
    return retval;
}

void Switch::init(const std::string &longName, char shortName) {
    if (shortName)
        shortNames_ = std::string(1, shortName);
    if (!longName.empty()) {
        longNames_.push_back(longName);
        key_ = documentationKey_ = longName;
    } else if (shortName) {
        key_ = documentationKey_ = std::string(1, shortName);
    } else {
        throw std::runtime_error("every Switch must have either a long or short name");
    }
}

Switch& Switch::longName(const std::string &name) {
    if (name.empty())
        throw std::runtime_error("switch long name cannot be empty");
    longNames_.push_back(name);
    return *this;
}

std::string Switch::synopsisForArgument(const SwitchArgument &sa) const {
    std::string retval;
    if (sa.isOptional())
        retval += "[";
    retval += "@v{" + sa.name() + "}";
    if (sa.isOptional())
        retval += "]";
    return retval;
}

std::string Switch::synopsis() const {
    if (!synopsis_.empty())
        return synopsis_;

    std::vector<std::string> perName;
    BOOST_FOREACH (const std::string &name, longNames_) {
        std::string s = "@s{" + name +"}";
        BOOST_FOREACH (const SwitchArgument &sa, arguments_)
            s += " " + synopsisForArgument(sa);
        perName.push_back(s);
    }
    BOOST_FOREACH (char name, shortNames_) {
        std::string s = "@s{" + std::string(1, name) + "}";
        BOOST_FOREACH (const SwitchArgument &sa, arguments_)
            s += " " + synopsisForArgument(sa);
        perName.push_back(s);
    }
    return boost::join(perName, "; ");
}

Switch& Switch::resetLongPrefixes(const std::string &s1, const std::string &s2, const std::string &s3, const std::string &s4) {
    properties_.inheritLongPrefixes = false;
    properties_.longPrefixes.clear();
    if (0!=s1.compare(STR_NONE))
        properties_.longPrefixes.push_back(s1);
    if (0!=s1.compare(STR_NONE))
        properties_.longPrefixes.push_back(s2);
    if (0!=s1.compare(STR_NONE))
        properties_.longPrefixes.push_back(s3);
    if (0!=s1.compare(STR_NONE))
        properties_.longPrefixes.push_back(s4);
    return *this;
}

Switch& Switch::resetShortPrefixes(const std::string &s1, const std::string &s2, const std::string &s3,
                                   const std::string &s4) {
    properties_.inheritShortPrefixes = false;
    properties_.shortPrefixes.clear();
    if (0!=s1.compare(STR_NONE))
        properties_.shortPrefixes.push_back(s1);
    if (0!=s1.compare(STR_NONE))
        properties_.shortPrefixes.push_back(s2);
    if (0!=s1.compare(STR_NONE))
        properties_.shortPrefixes.push_back(s3);
    if (0!=s1.compare(STR_NONE))
        properties_.shortPrefixes.push_back(s4);
    return *this;
}

Switch& Switch::resetValueSeparators(const std::string &s1, const std::string &s2, const std::string &s3,
                                     const std::string &s4) {
    properties_.inheritValueSeparators = false;
    properties_.valueSeparators.clear();
    if (0!=s1.compare(STR_NONE))
        properties_.valueSeparators.push_back(s1);
    if (0!=s1.compare(STR_NONE))
        properties_.valueSeparators.push_back(s2);
    if (0!=s1.compare(STR_NONE))
        properties_.valueSeparators.push_back(s3);
    if (0!=s1.compare(STR_NONE))
        properties_.valueSeparators.push_back(s4);
    return *this;
}

Switch& Switch::argument(const std::string &name, const ValueParser::Ptr &parser) {
    return argument(SwitchArgument(name, parser));
}

Switch& Switch::argument(const std::string &name, const ValueParser::Ptr &parser, const std::string &defaultValueStr) {
    return argument(SwitchArgument(name, parser, defaultValueStr));
}

Switch& Switch::intrinsicValue(const std::string &text, const ValueParser::Ptr &p) {
    intrinsicValue_ = p->matchString(text);
    intrinsicValue_.valueLocation(NOWHERE);
    return *this;
}

Switch& Switch::action(const SwitchAction::Ptr &action) {
    if (action!=NULL)
        actions_.push_back(action);
    return *this;
}

size_t Switch::nRequiredArguments() const {
    size_t retval = 0;
    BOOST_FOREACH (const SwitchArgument &sa, arguments_) {
        if (sa.isRequired())
            ++retval;
    }
    return retval;
}

std::runtime_error Switch::notEnoughArguments(const std::string &switchString, const Cursor &cursor, size_t nargs) const {
    std::ostringstream ss;
    ss <<"not enough arguments for " <<switchString <<" (found " <<nargs <<" but expected ";
    if (arguments_.size() != nRequiredArguments())
        ss <<"at least ";
    ss <<nRequiredArguments() <<")";
    return std::runtime_error(ss.str());
}

std::runtime_error Switch::noSeparator(const std::string &switchString, const Cursor &cursor,
                                       const ParsingProperties &props) const {
    std::string str = "expected one of the following separators between " + switchString + " and its argument:";
    BOOST_FOREACH (std::string sep, props.valueSeparators) {
        if (0!=sep.compare(" "))
            str += " \"" + sep + "\"";
    }
    return std::runtime_error(str);
}

std::runtime_error Switch::extraTextAfterArgument(const std::string &switchString, const Cursor &cursor) const {
    std::string str = "unexpected extra text after " + switchString + " argument: \"" + cursor.rest() + "\"";
    return std::runtime_error(str);
}

std::runtime_error Switch::extraTextAfterArgument(const std::string &switchString, const Cursor &cursor,
                                                  const SwitchArgument &sa) const {
    std::string str = "unexpected extra text after " + switchString + " " +
                      boost::to_upper_copy(sa.name()) + " argument: \"" + cursor.rest() + "\"";
    return std::runtime_error(str);
}
    
std::runtime_error Switch::missingArgument(const std::string &switchString, const Cursor &cursor,
                                           const SwitchArgument &sa, const std::string &reason) const {
    std::string str = switchString + " argument " + boost::to_upper_copy(sa.name()) + " is missing";
    if (!reason.empty())
        str += ": " + reason;
    return std::runtime_error(str);
}
    
size_t Switch::matchLongName(Cursor &cursor, const ParsingProperties &props) const {
    ASSERT_require(cursor.atArgBegin());
    BOOST_FOREACH (const std::string &prefix, props.longPrefixes) {
        if (boost::starts_with(cursor.arg(), prefix)) {
            std::string rest = cursor.arg().substr(prefix.size());
            BOOST_FOREACH (const std::string &name, longNames_) {
                if (boost::starts_with(rest, name)) {
                    size_t retval = prefix.size() + name.size();
                    rest = rest.substr(name.size());
                    if (rest.empty()) {
                        cursor.consumeChars(retval);
                        return retval;                  // switch name matches to end of program argument
                    }
                    if (0==arguments_.size())
                        continue;                       // switch name does not match to end, but has no declared args
                    BOOST_FOREACH (const std::string &sep, props.valueSeparators) {
                        if (0!=sep.compare(" ") && boost::starts_with(rest, sep)) {
                            cursor.consumeChars(retval);
                            return retval;              // found prefix, name, and separator for switch with args
                        }
                    }
                }
            }
        }
    }
    return 0;
}

size_t Switch::matchShortName(Cursor &cursor, const ParsingProperties &props, std::string &name /*out*/) const {
    BOOST_FOREACH (const std::string &prefix, props.shortPrefixes) {
        if (boost::starts_with(cursor.arg(), prefix)) {
            if (prefix.size() >= cursor.location().offset && prefix.size() < cursor.arg().size()) {
                // name must immediately follow the prefix
                if (strchr(shortNames_.c_str(), cursor.arg()[prefix.size()])) {
                    size_t retval = prefix.size() + 1;
                    name = cursor.arg().substr(0, retval);
                    Location p = cursor.location();
                    p.offset = retval;
                    cursor.location(p);
                    return retval;
                }
            } else if (prefix.size() < cursor.arg().size()) {
                if (strchr(shortNames_.c_str(), cursor.rest()[0])) {
                    name = cursor.arg().substr(0, prefix.size()) + cursor.rest().substr(0, 1);
                    cursor.consumeChars(1);
                    return prefix.size() + 1;
                }
            }
        }
    }
    name = "";
    return 0;
}

// optionally explodes a vector value into separate values
bool Switch::explode(ParsedValues &pvals /*in,out*/) const {
    if (!explosiveLists_)
        return false;

    bool retval = false;
    ParsedValues pvals2;
    BOOST_FOREACH (const ParsedValue &pval1, pvals) {
        if (pval1.value().type()==typeid(ListParser::ValueList)) {
            ListParser::ValueList elmts = boost::any_cast<ListParser::ValueList>(pval1.value());
            BOOST_FOREACH (const ParsedValue &elmt, elmts) {
                pvals2.push_back(elmt);
                retval = true;
            }
        } else {
            pvals2.push_back(pval1);
        }
    }
    pvals = pvals2;
    return retval;
}

// matches short or long arguments. Counts only arguments that are actually present.  The first present switch argument starts
// at the cursor, and subsequent switch arguments must be aligned at the beginning of a program argument.
size_t Switch::matchArguments(const std::string &switchString, Cursor &cursor /*in,out*/, ParsedValues &result /*out*/,
                              bool isLongSwitch) const {
    ExcursionGuard guard(cursor);
    size_t retval = 0;
    BOOST_FOREACH (const SwitchArgument &sa, arguments_) {
        // Second and subsequent arguments must start at the beginning of a program argument
        if (retval>0 && !cursor.atArgBegin())
            throw extraTextAfterArgument(switchString, cursor, sa);

        // Parse the argument value if possible, otherwise use a default if allowed
        Location valueLocation = cursor.location();
        try {
            ParsedValue value = sa.parser()->match(cursor);
            if (cursor.location() == valueLocation && sa.isRequired())
                throw std::runtime_error("not found");
            result.push_back(value);
            ++retval;
        } catch (const std::runtime_error &e) {
            if (sa.isRequired())
                throw missingArgument(switchString, cursor, sa, e.what());
            result.push_back(sa.defaultValue());
        }

        // Long switch arguments must end aligned with a program argument
        if (isLongSwitch && !cursor.atArgBegin() && !cursor.atEnd())
            throw extraTextAfterArgument(switchString, cursor, sa);
    }
    explode(result);
    guard.cancel();
    return retval;
}

void Switch::matchLongArguments(const std::string &switchString, Cursor &cursor /*in,out*/, const ParsingProperties &props,
                                ParsedValues &result /*out*/) const {
    ExcursionGuard guard(cursor);

    // If the switch has no declared arguments use its intrinsic value.
    if (arguments_.empty()) {
        ASSERT_require(cursor.atArgBegin() || cursor.atEnd());
        result.push_back(intrinsicValue_);
        return;
    }

    // Try to match the name/value separator.  Advance the cursor to the first character of the first value.
    bool matchedSeparator = false;
    if (cursor.atArgBegin()) {
        if (matchAnyString(props.valueSeparators, " "))
            matchedSeparator = true;
    } else {
        std::string s = cursor.rest();
        BOOST_FOREACH (const std::string &sep, props.valueSeparators) {
            if (boost::starts_with(s, sep)) {
                cursor.consumeChars(sep.size());
                matchedSeparator = true;
                break;
            }
        }
    }

    // If we couldn't match the name/value separator then this switch cannot have any values.
    if (!matchedSeparator && nRequiredArguments()>0)
        throw noSeparator(switchString, cursor, props);

    // Parse the arguments for this switch now that we've consumed the prefix, switch name, and argument separators.
    size_t nValuesParsed = matchArguments(switchString, cursor, result /*out*/, true /*longSwitch*/);

    // Post conditions
    if (!cursor.atArgBegin() && !cursor.atEnd())
        throw extraTextAfterArgument(switchString, cursor);
    if (nValuesParsed < nRequiredArguments())
        throw notEnoughArguments(switchString, cursor, nValuesParsed);
    guard.cancel();
}

void Switch::matchShortArguments(const std::string &switchString, Cursor &cursor /*in,out*/, const ParsingProperties &props,
                                ParsedValues &result /*out*/) const {
    // If the switch has no declared arguments, then parse its default.
    if (arguments_.empty()) {
        result.push_back(intrinsicValue_);
        return;
    }

    // Parse the arguments for this switch.
    size_t nValuesParsed = matchArguments(switchString, cursor, result /*out*/, false /*shortSwitch*/);
    if (nValuesParsed < nRequiredArguments())
        throw notEnoughArguments(switchString, cursor, nValuesParsed);
}

void Switch::runActions(const Parser *parser) const {
    BOOST_FOREACH (const SwitchAction::Ptr &action, actions_)
        action->run(parser);
}


/*******************************************************************************************************************************
 *                                      SwitchGroup
 *******************************************************************************************************************************/

SwitchGroup& SwitchGroup::resetLongPrefixes(const std::string &s1, const std::string &s2,
                                            const std::string &s3, const std::string &s4) {
    properties_.inheritLongPrefixes = false;
    properties_.longPrefixes.clear();
    if (0!=s1.compare(STR_NONE))
        properties_.longPrefixes.push_back(s1);
    if (0!=s1.compare(STR_NONE))
        properties_.longPrefixes.push_back(s2);
    if (0!=s1.compare(STR_NONE))
        properties_.longPrefixes.push_back(s3);
    if (0!=s1.compare(STR_NONE))
        properties_.longPrefixes.push_back(s4);
    return *this;
}

SwitchGroup& SwitchGroup::resetShortPrefixes(const std::string &s1, const std::string &s2,
                                             const std::string &s3, const std::string &s4) {
    properties_.inheritShortPrefixes = true;
    properties_.shortPrefixes.clear();
    if (0!=s1.compare(STR_NONE))
        properties_.shortPrefixes.push_back(s1);
    if (0!=s1.compare(STR_NONE))
        properties_.shortPrefixes.push_back(s2);
    if (0!=s1.compare(STR_NONE))
        properties_.shortPrefixes.push_back(s3);
    if (0!=s1.compare(STR_NONE))
        properties_.shortPrefixes.push_back(s4);
    return *this;
}

SwitchGroup& SwitchGroup::resetValueSeparators(const std::string &s1, const std::string &s2,
                                               const std::string &s3, const std::string &s4) {
    properties_.inheritValueSeparators = false;
    properties_.valueSeparators.clear();
    if (0!=s1.compare(STR_NONE))
        properties_.valueSeparators.push_back(s1);
    if (0!=s1.compare(STR_NONE))
        properties_.valueSeparators.push_back(s2);
    if (0!=s1.compare(STR_NONE))
        properties_.valueSeparators.push_back(s3);
    if (0!=s1.compare(STR_NONE))
        properties_.valueSeparators.push_back(s4);
    return *this;
}

SwitchGroup& SwitchGroup::insert(const Switch &sw) {
    switches_.push_back(sw);
    return *this;
}

bool SwitchGroup::nameExists(const std::string &s) {
    for (size_t i=0; i<switches_.size(); ++i) {
        const std::vector<std::string> &names = switches_[i].longNames();
        if (std::find(names.begin(), names.end(), s)!=names.end() ||
            (1==s.size() && boost::contains(switches_[i].shortNames(), s)))
            return true;
    }
    return false;
}

const Switch& SwitchGroup::getByName(const std::string &s) {
    for (size_t i=0; i<switches_.size(); ++i) {
        const std::vector<std::string> &names = switches_[i].longNames();
        if (std::find(names.begin(), names.end(), s)!=names.end() ||
            (1==s.size() && boost::contains(switches_[i].shortNames(), s)))
            return switches_[i];
    }
    throw std::runtime_error("switch \"" + s + "\" not found\n");
}

bool SwitchGroup::keyExists(const std::string &s) {
    for (size_t i=0; i<switches_.size(); ++i) {
        if (0==switches_[i].key().compare(s))
            return true;
    }
    return false;
}

const Switch& SwitchGroup::getByKey(const std::string &s) {
    for (size_t i=0; i<switches_.size(); ++i) {
        if (0==switches_[i].key().compare(s))
            return switches_[i];
    }
    throw std::runtime_error("switch key \"" + s + "\" not found\n");
}

/*******************************************************************************************************************************
 *                                      Parser results
 *******************************************************************************************************************************/

// Do not save the 'sw' pointer because we have no control over when the user will destroy the object.
// This should be called for at most one switch occurrence at a time.
void ParserResult::insertValuesForSwitch(const ParsedValues &pvals, const Parser *parser, const Switch *sw) {
    std::string key = sw->key();
    std::string name = sw->preferredName();

    // How to save this value
    bool shouldSave = true;
    
    switch (sw->whichValue()) {
        case SAVE_NONE:
            if (!pvals.empty())
                throw std::runtime_error(pvals.front().switchString() + " is illegal here");
        case SAVE_ONE:
            if (!keyIndex_[key].empty() && !pvals.empty())
                throw std::runtime_error("switch key \"" + key + "\" cannot appear multiple times (" +
                                         pvals.front().switchString() + ")");
            break;
        case SAVE_FIRST:
            if (!keyIndex_[key].empty())
                shouldSave = false;                 // skip this value since we already saved one
            break;
        case SAVE_LAST:
            keyIndex_[key].clear();
            break;
        case SAVE_ALL:
            break;
        case SAVE_AUGMENTED:
            ValueAugmenter::Ptr f = sw->valueAugmenter();
            if (f!=NULL && !keyIndex_[key].empty()) {
                ParsedValues oldValues;
                BOOST_FOREACH (size_t idx, keyIndex_[key])
                    oldValues.push_back(values_[idx]);
                ParsedValues newValues = (*f)(oldValues, pvals);
                keyIndex_[key].clear();
                BOOST_FOREACH (const ParsedValue &pval, newValues)
                    insertOneValue(pval, key, name);
                sw->runActions(parser);
                return;
            }
            keyIndex_[key].clear();                 // act like SAVE_LAST
            break;
    }

    if (shouldSave) {
        BOOST_FOREACH (const ParsedValue &pval, pvals)
            insertOneValue(pval, key, name);
        sw->runActions(parser);
    }
}

void ParserResult::insertOneValue(const ParsedValue &pval, const std::string &key, const std::string &name) {
    // Get sequences for this value and update the value.
    size_t keySequence = keyIndex_[key].size();
    size_t switchSequence = switchIndex_[name].size();

    // Insert the parsed value and update all the indexes
    size_t idx = values_.size();
    values_.push_back(pval);
    values_.back().switchKey(key);
    values_.back().sequenceInfo(keySequence, switchSequence);
    keyIndex_[key].push_back(idx);
    switchIndex_[name].push_back(idx);
    argvIndex_[pval.switchLocation()].push_back(idx);

#if 1 /*DEBUGGING [Robb Matzke 2014-02-18]*/
    std::cerr <<"    " <<values_.back() <<"\n";
#endif
}
    
void ParserResult::skip(const Location &loc) {
    skippedIndex_.push_back(loc.idx);
}

void ParserResult::terminator(const Location &loc) {
    terminators_.push_back(loc.idx);
}

const ParserResult& ParserResult::apply() const {
    for (NameIndex::const_iterator ki=keyIndex_.begin(); ki!=keyIndex_.end(); ++ki) {
        BOOST_FOREACH (size_t idx, ki->second) {
            values_[idx].save();
        }
    }
    return *this;
}

const ParsedValue& ParserResult::parsed(const std::string &switchKey, size_t idx) {
    return values_[keyIndex_[switchKey][idx]];
}

ParsedValues ParserResult::parsed(const std::string &switchKey) {
    ParsedValues retval;
    BOOST_FOREACH (size_t idx, keyIndex_[switchKey])
        retval.push_back(values_[idx]);
    return retval;
}

std::vector<std::string> ParserResult::skippedArgs() const {
    std::vector<std::string> retval;
    BOOST_FOREACH (size_t idx, skippedIndex_)
        retval.push_back(cursor_.strings()[idx]);
    return retval;
}

std::vector<std::string> ParserResult::unreachedArgs() const {
    std::vector<std::string> retval;
    for (size_t i=cursor_.location().idx; i<cursor_.strings().size(); ++i)
        retval.push_back(cursor_.strings()[i]);
    return retval;
}

std::vector<std::string> ParserResult::unparsedArgs(bool includeTerminators) const {
    std::set<size_t> indexes;
    BOOST_FOREACH (size_t idx, skippedIndex_)
        indexes.insert(idx);
    if (includeTerminators) {
        BOOST_FOREACH (size_t idx, terminators_)
            indexes.insert(idx);
    }
    for (size_t i=cursor_.location().idx; i<cursor_.strings().size(); ++i)
        indexes.insert(i);

    std::vector<std::string> retval;
    BOOST_FOREACH (size_t idx, indexes)
        retval.push_back(cursor_.strings()[idx]);
    return retval;
}

std::vector<std::string> ParserResult::parsedArgs() const {
    std::set<size_t> indexes;

    // Program arguments that have parsed switches, and the locations of the switch values
    for (ArgvIndex::const_iterator i=argvIndex_.begin(); i!=argvIndex_.end(); ++i) {
        indexes.insert(i->first.idx);
        BOOST_FOREACH (size_t valueIdx, i->second) {
            const Location valueLocation = values_[valueIdx].valueLocation();
            if (valueLocation != NOWHERE)
                indexes.insert(valueLocation.idx);
        }
    }

    BOOST_FOREACH (size_t idx, terminators_)
        indexes.insert(idx);

    std::vector<std::string> retval;
    BOOST_FOREACH (size_t idx, indexes)
        retval.push_back(cursor_.strings()[idx]);
    return retval;
}


/*******************************************************************************************************************************
 *                                      Parser
 *******************************************************************************************************************************/

void Parser::init() {
    properties_.longPrefixes.push_back("--");           // as in "--version"
    properties_.shortPrefixes.push_back("-");           // as in "-V"
    properties_.valueSeparators.push_back("=");         // as in "--switch=value"
    properties_.valueSeparators.push_back(" ");         // switch value is in next program argument
    terminationSwitches_.push_back("--");
    inclusionPrefixes_.push_back("@");
}

Parser& Parser::resetLongPrefixes(const std::string &s1, const std::string &s2, const std::string &s3, const std::string &s4) {
    properties_.inheritLongPrefixes = false;
    properties_.longPrefixes.clear();
    if (0!=s1.compare(STR_NONE))
        properties_.longPrefixes.push_back(s1);
    if (0!=s1.compare(STR_NONE))
        properties_.longPrefixes.push_back(s2);
    if (0!=s1.compare(STR_NONE))
        properties_.longPrefixes.push_back(s3);
    if (0!=s1.compare(STR_NONE))
        properties_.longPrefixes.push_back(s4);
    return *this;
}

Parser& Parser::resetShortPrefixes(const std::string &s1, const std::string &s2, const std::string &s3, const std::string &s4) {
    properties_.inheritShortPrefixes = false;
    properties_.shortPrefixes.clear();
    if (0!=s1.compare(STR_NONE))
        properties_.shortPrefixes.push_back(s1);
    if (0!=s1.compare(STR_NONE))
        properties_.shortPrefixes.push_back(s2);
    if (0!=s1.compare(STR_NONE))
        properties_.shortPrefixes.push_back(s3);
    if (0!=s1.compare(STR_NONE))
        properties_.shortPrefixes.push_back(s4);
    return *this;
}

Parser& Parser::resetValueSeparators(const std::string &s1, const std::string &s2,
                                     const std::string &s3, const std::string &s4) {
    properties_.inheritValueSeparators = false;
    properties_.valueSeparators.clear();
    if (0!=s1.compare(STR_NONE))
        properties_.valueSeparators.push_back(s1);
    if (0!=s1.compare(STR_NONE))
        properties_.valueSeparators.push_back(s2);
    if (0!=s1.compare(STR_NONE))
        properties_.valueSeparators.push_back(s3);
    if (0!=s1.compare(STR_NONE))
        properties_.valueSeparators.push_back(s4);
    return *this;
}

Parser& Parser::resetTerminationSwitches(const std::string &s1, const std::string &s2,
                                         const std::string &s3, const std::string &s4) {
    terminationSwitches_.clear();
    if (0!=s1.compare(STR_NONE))
        terminationSwitches_.push_back(s1);
    if (0!=s1.compare(STR_NONE))
        terminationSwitches_.push_back(s2);
    if (0!=s1.compare(STR_NONE))
        terminationSwitches_.push_back(s3);
    if (0!=s1.compare(STR_NONE))
        terminationSwitches_.push_back(s4);
    return *this;
}

Parser& Parser::resetInclusionPrefixes(const std::string &s1, const std::string &s2,
                                         const std::string &s3, const std::string &s4) {
    inclusionPrefixes_.clear();
    if (0!=s1.compare(STR_NONE))
        inclusionPrefixes_.push_back(s1);
    if (0!=s1.compare(STR_NONE))
        inclusionPrefixes_.push_back(s2);
    if (0!=s1.compare(STR_NONE))
        inclusionPrefixes_.push_back(s3);
    if (0!=s1.compare(STR_NONE))
        inclusionPrefixes_.push_back(s4);
    return *this;
}

ParserResult Parser::parse(int argc, char *argv[]) {
    std::vector<std::string> args(argv+1, argv+argc);
    return parse(args);
}

ParserResult Parser::parse(const std::vector<std::string> &programArguments) {
    if (errorStream_) {
        try {
            return parseInternal(programArguments);
        } catch (const std::runtime_error &e) {
            *errorStream_ << e.what() <<"\n";
            if (exitMessage_) {
                *errorStream_ <<*exitMessage_ <<"\n";
            } else {
                BOOST_FOREACH (const SwitchGroup &sg, switchGroups_) {
                    ParsingProperties sgProps = sg.properties().inherit(properties_);
                    BOOST_FOREACH (const Switch &sw, sg.switches()) {
                        BOOST_FOREACH (const std::string &name, sw.longNames()) {
                            if (0==name.compare("help")) {
                                ParsingProperties swProps = sw.properties().inherit(sgProps);
                                std::string prefix = swProps.longPrefixes.empty() ? std::string() : swProps.longPrefixes.front();
                                *errorStream_ <<"invoke with '" <<prefix <<"help' for usage information.\n";
                                exit(1);
                            }
                        }
                    }
                }
            }
            exit(1);
        }
    } else {
        return parseInternal(programArguments);
    }
}

ParserResult Parser::parseInternal(const std::vector<std::string> &programArguments) {
    ParserResult result(programArguments);
    Cursor &cursor = result.cursor();

    while (!cursor.atEnd()) {
        ASSERT_require(cursor.atArgBegin());

        // Check for termination switch.
        BOOST_FOREACH (std::string termination, terminationSwitches_) {
            if (0==cursor.arg().compare(termination)) {
                result.terminator(cursor.location());
                cursor.consumeArg();
                return result;
            }
        }

        // Check for file inclusion switch.
        bool inserted = false;
        BOOST_FOREACH (std::string prefix, inclusionPrefixes_) {
            const std::string &arg = cursor.arg();
            if (boost::starts_with(arg, prefix) && arg.size() > prefix.size()) {
                std::string filename = arg.substr(prefix.size());
                std::vector<std::string> args = readArgsFromFile(filename);
                cursor.replace(args);
                inserted = true;
            }
        }
        if (inserted)
            continue;
        
        // Does this look like a switch (even one that we might not know about)?
        bool isSwitch = apparentSwitch(cursor);
        if (!isSwitch) {
            if (skipNonSwitches_) {
                result.skip(cursor.location());
                cursor.consumeArg();
                continue;
            } else {
                return result;
            }
        }

        // Attempt to parse the switch. The parseOneSwitch() throws an exception if something goes wrong, but returns NULL if
        // there's no switch to parse.
        try {
            parseOneSwitch(cursor, result);
        } catch (const std::runtime_error &e) {
            if (skipUnknownSwitches_) {
                result.skip(cursor.location());
                cursor.consumeArg();
                continue;
            } else {
                throw;
            }
        }
    }

    return result;                                      // reached end of program arguments
}

bool Parser::parseOneSwitch(Cursor &cursor, ParserResult &result) {
    boost::optional<std::runtime_error> saved_error;

    // Single long switch
    ParsedValues values;
    if (const Switch *sw = parseLongSwitch(cursor, values, saved_error /*out*/)) {
        result.insertValuesForSwitch(values, this, sw);
        return true;
    }

    if (!shortMayNestle_) {
        // Single short switch
        if (const Switch *sw = parseShortSwitch(cursor, values, saved_error)) {
            if (!cursor.atArgBegin() && !cursor.atEnd()) {
                ASSERT_forbid(values.empty());
                throw sw->extraTextAfterArgument(values.front().switchString(), cursor);
            }
            result.insertValuesForSwitch(values, this, sw);
            return true;
        }
    } else {
        // Or multiple short switches.  If short switches are nestled, then the result is affected only if all the nesltled
        // switches can be parsed.
        typedef std::pair<const Switch*, ParsedValues> SwitchValues;
        std::list<SwitchValues> valuesBySwitch;
        bool allParsed = false;
        ExcursionGuard guard(cursor);
        while (const Switch *sw = parseShortSwitch(cursor, values, saved_error)) {
            valuesBySwitch.push_back(SwitchValues(sw, values));
            values.clear();
            if (cursor.atArgBegin() || cursor.atEnd()) {
                allParsed = true;
                break;
            }
        }
        if (allParsed) {
            BOOST_FOREACH (SwitchValues &svpair, valuesBySwitch)
                result.insertValuesForSwitch(svpair.second, this, svpair.first);
            guard.cancel();
            return true;
        }
    }

    // Throw or return zero?
    if (saved_error)
        throw *saved_error;                             // found at least one switch but couldn't ever parse arguments
    if (apparentSwitch(cursor))
        throw std::runtime_error("unrecognized switch: " + cursor.arg());
    return false;
}

// Parse one long switch and advance the cursor over the switch name and arguments.
const Switch* Parser::parseLongSwitch(Cursor &cursor, ParsedValues &parsedValues,
                                      boost::optional<std::runtime_error> &saved_error) {
    if (!cursor.atArgBegin())
        return NULL;
    BOOST_FOREACH (const SwitchGroup &sg, switchGroups_) {
        ParsingProperties sgProps = sg.properties().inherit(properties_);
        BOOST_FOREACH (const Switch &sw, sg.switches()) {
            ExcursionGuard guard(cursor);
            ParsingProperties swProps = sw.properties().inherit(sgProps);
            Location switchLocation = cursor.location();
            if (sw.matchLongName(cursor, swProps)) {
                const std::string switchString = cursor.substr(switchLocation);
                try {
                    ParsedValues pvals;
                    sw.matchLongArguments(switchString, cursor, swProps, pvals /*out*/);
                    BOOST_FOREACH (ParsedValue &pv, pvals)
                        pv.switchInfo(sw.key(), switchLocation, switchString);
                    parsedValues.insert(parsedValues.end(), pvals.begin(), pvals.end()); // may throw
                    guard.cancel();
                    return &sw;
                } catch (const std::runtime_error &e) {
                    saved_error = e;
                }
            }
        }
    }
    return NULL;
}

// Parse one short switch and advance the cursor over the switch name and arguments.
const Switch* Parser::parseShortSwitch(Cursor &cursor, ParsedValues &parsedValues,
                                       boost::optional<std::runtime_error> &saved_error) {
    BOOST_FOREACH (const SwitchGroup &sg, switchGroups_) {
        ParsingProperties sgProps = sg.properties().inherit(properties_);
        BOOST_FOREACH (const Switch &sw, sg.switches()) {
            ExcursionGuard guard(cursor);
            ParsingProperties swProps = sw.properties().inherit(sgProps);
            Location switchLocation = cursor.location();
            std::string switchString;
            if (sw.matchShortName(cursor, swProps, switchString /*out*/)) {
                try {
                    ParsedValues pvals;
                    sw.matchShortArguments(switchString, cursor, swProps, pvals /*out*/);
                    BOOST_FOREACH (ParsedValue &pv, pvals)
                        pv.switchInfo(sw.key(), switchLocation, switchString);
                    parsedValues.insert(parsedValues.end(), pvals.begin(), pvals.end()); // may throw
                    guard.cancel();
                    return &sw;
                } catch (const std::runtime_error &e) {
                    saved_error = e;
                }
            }
        }
    }
    return false;
}
    
bool Parser::apparentSwitch(const Cursor &cursor) const {
    BOOST_FOREACH (const SwitchGroup &sg, switchGroups_) {
        ParsingProperties sgProps = sg.properties().inherit(properties_);
        BOOST_FOREACH (const Switch &sw, sg.switches()) {
            ParsingProperties swProps = sw.properties().inherit(sgProps);
            BOOST_FOREACH (const std::string &prefix, swProps.longPrefixes) {
                if (!prefix.empty() && boost::starts_with(cursor.arg(), prefix) && cursor.arg().size() > prefix.size())
                    return true;
            }
            BOOST_FOREACH (const std::string &prefix, swProps.shortPrefixes) {
                if (!prefix.empty() && boost::starts_with(cursor.arg(), prefix) && cursor.arg().size() > prefix.size())
                    return true;
            }
        }
    }
    return false;
}

// Read a text file to obtain command line arguments which are returned.
std::vector<std::string> Parser::readArgsFromFile(const std::string &filename) {
    std::vector<std::string> retval;
    struct FileGuard {
        FILE *f;
        char *linebuf;
        size_t linebufsz;
        FileGuard(FILE *f): f(f), linebuf(NULL), linebufsz(0) {}
        ~FileGuard() {
            if (f) fclose(f);
            if (linebuf) free(linebuf);
        }
    } file(fopen(filename.c_str(), "r"));
    if (NULL==file.f)
        throw std::runtime_error("failed to open file \"" + filename + "\": " + strerror(errno));

    ssize_t nchars, nlines = 0;
    while ((nchars = getline(&file.linebuf, &file.linebufsz, file.f))>0) {
        ++nlines;
        std::string line = boost::trim_copy(std::string(file.linebuf, nchars));
        nchars = line.size();
        if (line.empty() || '#'==line[0])
            continue;
        char inQuote = '\0';
        std::string word;

        for (ssize_t i=0; i<nchars; ++i) {
            char ch = line[i];
            if ('\''==ch || '"'==ch) {
                if (ch==inQuote) {
                    inQuote = '\0';
                } else if (!inQuote) {
                    inQuote = ch;
                } else {
                    word += ch;
                }
            } else if ('\\'==ch && i+1<nchars && (strchr("'\"\\", line[i+1]) || isspace(line[i+1]))) {
                word += line[++i];
            } else if (isspace(ch) && !inQuote) {
                while (i+1<nchars && isspace(line[i+1]))
                    ++i;
                retval.push_back(word);
                word = "";
            } else {
                word += ch;
            }
        }
        retval.push_back(word);

        if (inQuote) {
            std::ostringstream ss;
            ss <<"unterminated quote at line " <<nlines <<" in " <<filename;
            throw std::runtime_error(ss.str());
        }
    }
    return retval;
}

const std::string& Parser::programName() const {
    if (programName_.empty()) {
        boost::optional<std::string> s = Message::Prefix::instance()->programName();
        if (s)
            programName_ = *s;
    }
    return programName_;
}

Parser& Parser::version(const std::string &versionString, const std::string &dateString) {
    versionString_ = versionString;
    dateString_ = dateString;
    return *this;
}

std::pair<std::string, std::string> Parser::version() const {
    if (dateString_.empty()) {
        time_t now = time(NULL);
        struct tm tm;
        if (localtime_r(&now, &tm)) {
            static const char *month[] = {"January", "February", "March", "April", "May", "June", "July",
                                          "August", "September", "October", "November", "December"};
            dateString_ = std::string(month[tm.tm_mon]) + " " + boost::lexical_cast<std::string>(1900+tm.tm_year);
        }
    }
    return std::make_pair(versionString_, dateString_);
}

Parser& Parser::chapter(int chapterNumber, const std::string &chapterName) {
    int cn = chapterNumber_ = chapterNumber < 1 || chapterNumber > 9 ? 1 : chapterNumber;
    if (chapterName.empty()) {
        static const char *chapter[] = {
            "",                                         // 0
            "User Commands",                            // 1
            "System Calls",                             // 2
            "Libraries",                                // 3
            "Devices",                                  // 4
            "File Formats",                             // 5
            "Games",                                    // 6
            "Miscellaneous",                            // 7
            "System Administration",                    // 8
            "Documentation"                             // 9
        };
        chapterName_ = chapter[cn];
    } else {
        chapterName_ = chapterName;
    }
    return *this;
}

std::pair<int, std::string> Parser::chapter() const {
    return std::make_pair(chapterNumber_, chapterName_);
}

Parser& Parser::doc(const std::string &sectionName, const std::string &docKey, const std::string &text) {
    sectionOrder_[docKey] = sectionName;
    sectionDoc_[boost::to_lower_copy(sectionName)] = text;
    return *this;
}

std::vector<std::string> Parser::docSections() const {
    std::vector<std::string> retval;
    for (StringStringMap::const_iterator di=sectionDoc_.begin(); di!=sectionDoc_.end(); ++di)
        retval.push_back(di->first);
    return retval;
}

// @s{NAME} where NAME is either a long or short switch name without prefix.
typedef std::map<std::string, std::string> PreferredPrefixes; // maps switch names to their best prefixes
typedef boost::shared_ptr<class SwitchTag> SwitchTagPtr;
class SwitchTag: public Markup::Tag {
    PreferredPrefixes preferredPrefixes_;
    std::string bestShortPrefix_;                       // short prefix if the switch name is not recognized
    std::string bestLongPrefix_;                        // long prefix if the switch name is not recognized
protected:
    SwitchTag(const PreferredPrefixes &known, const std::string &bestShort, const std::string &bestLong)
        : Markup::Tag("switch", 1), preferredPrefixes_(known), bestShortPrefix_(bestShort), bestLongPrefix_(bestLong) {}
public:
    static SwitchTagPtr instance(const PreferredPrefixes &known, const std::string &bestShort, const std::string &bestLong) {
        return SwitchTagPtr(new SwitchTag(known, bestShort, bestLong));
    }
    virtual Markup::ContentPtr eval(const Markup::TagArgs &args) /*override*/ {
        using namespace Markup;
        ASSERT_require(1==args.size());
        std::string raw = args.front()->asText();
        PreferredPrefixes::const_iterator i = preferredPrefixes_.find(raw);
        if (i==preferredPrefixes_.end()) {
            if (1==raw.size()) {
                raw = bestShortPrefix_ + raw;
            } else {
                raw = bestLongPrefix_ + raw;
            }
        } else {
            raw = i->second + raw;
        }
        TagInstancePtr nulltag = TagInstance::instance(NullTag::instance(raw));
        ContentPtr retval = Content::instance();
        retval->append(nulltag);
        return retval;
    }
};

// @seeAlso is replaced by the list @man references that have been processed so far.
typedef boost::shared_ptr<class SeeAlsoTag> SeeAlsoTagPtr;
class SeeAlsoTag: public Markup::Tag {
public:
public:
    typedef std::map<std::string, Markup::ContentPtr> SeeAlso;
private:
    SeeAlso seeAlso_;
protected:
    SeeAlsoTag(): Markup::Tag("seeAlso", 0) {}
public:
    static SeeAlsoTagPtr instance() { return SeeAlsoTagPtr(new SeeAlsoTag); }
    void insert(const std::string &name, const Markup::ContentPtr content) {
        seeAlso_[name] = content;
    }
    virtual Markup::ContentPtr eval(const Markup::TagArgs &args) /*override*/ {
        using namespace Markup;
        ASSERT_require(0==args.size());
        ContentPtr retval = Content::instance();
        for (SeeAlso::iterator sai=seeAlso_.begin(); sai!=seeAlso_.end(); ++sai) {
            if (sai!=seeAlso_.begin())
                retval->append(", ");
            retval->append(sai->second);
        }
        return retval;
    }
};

// @man{PAGE}{CHAPTER} converted to @b{PAGE}(CHAPTER) to cite Unix manual pages.
typedef boost::shared_ptr<class ManTag> ManTagPtr;
class ManTag: public Markup::Tag {
    SeeAlsoTagPtr seeAlso_;
protected:
    ManTag(const SeeAlsoTagPtr &seeAlso): Markup::Tag("man", 2), seeAlso_(seeAlso) {}
public:
    static ManTagPtr instance(const SeeAlsoTagPtr &seeAlso) { return ManTagPtr(new ManTag(seeAlso)); }
    virtual Markup::ContentPtr eval(const Markup::TagArgs &args) /*override*/ {
        using namespace Markup;
        ASSERT_require(2==args.size());
        ContentPtr retval = Content::instance();
        retval->append(TagInstance::instance(BoldTag::instance(), args[0]));
        retval->append("(");
        retval->append(args[1]);
        retval->append(")");
        seeAlso_->insert(args[0]->asText(), retval);
        return retval;
    }
};

// @prop{KEY} is replaced with the property string stored for KEY
typedef boost::shared_ptr<class PropTag> PropTagPtr;
class PropTag: public Markup::Tag {
    std::map<std::string, std::string> values_;
protected:
    PropTag(): Markup::Tag("prop", 1) {}
public:
    static PropTagPtr instance() { return PropTagPtr(new PropTag); }
    PropTagPtr with(const std::string &key, const std::string &value) {
        values_[key] = value;
        return boost::dynamic_pointer_cast<PropTag>(shared_from_this());
    }
    virtual Markup::ContentPtr eval(const Markup::TagArgs &args) /*overload*/ {
        using namespace Markup;
        ASSERT_require(1==args.size());
        std::string key = args.front()->asText();
        ContentPtr retval = Content::instance();
        retval->append(values_[key]);
        return retval;
    }
};

std::string Parser::docForSection(const std::string &sectionName) const {
    std::string docKey = boost::to_lower_copy(sectionName);
    StringStringMap::const_iterator section = sectionDoc_.find(docKey);
    std::string doc = section == sectionDoc_.end() ? std::string() : section->second;
    if (0==docKey.compare("name")) {
        if (doc.empty())
            doc = programName() + " - " + (purpose_.empty() ? std::string("Undocumented") : purpose_);
    } else if (0==docKey.compare("synopsis")) {
        if (doc.empty())
            doc = programName() + " [@v{switches}...]\n";
    } else if (0==docKey.compare("options")) {
        StringStringMap docByKey;
        BOOST_FOREACH (const SwitchGroup &sg, switchGroups_) {
            BOOST_FOREACH (const Switch &sw, sg.switches()) {
                if (!sw.hidden()) {
                    const std::string &swDoc = sw.doc();
                    docByKey[sw.docKey()] = "@defn{" + sw.synopsis() + "}"
                                            "{" + (swDoc.empty() ? std::string("Not documented.") : sw.doc()) + "}\n";
                }
            }
        }
        doc += "\n\n";
        for (StringStringMap::iterator di=docByKey.begin(); di!=docByKey.end(); ++di)
            doc += di->second;
    } else if (0==docKey.compare("see also")) {
            doc += "\n\n@seeAlso";
    }
    
    return doc;
}

// Returns a map that lists all known switches and their preferred prefix
void Parser::preferredSwitchPrefixes(std::map<std::string, std::string> &prefixMap /*out*/) const {
    BOOST_FOREACH (const SwitchGroup &sg, switchGroups_) {
        ParsingProperties sgProps = sg.properties().inherit(properties_);
        BOOST_FOREACH (const Switch &sw, sg.switches()) {
            ParsingProperties swProps = sw.properties().inherit(sgProps);
            if (!swProps.longPrefixes.empty()) {
                const std::string &prefix = swProps.longPrefixes.front();
                BOOST_FOREACH (const std::string &name, sw.longNames())
                    prefixMap[name] = prefix;
            }
            if (!swProps.shortPrefixes.empty()) {
                const std::string &prefix = swProps.shortPrefixes.front();
                BOOST_FOREACH (char name, sw.shortNames())
                    prefixMap[std::string(1, name)] = prefix;
            }
        }
    }
}

// Obtain the documentation markup for this parser.
std::string Parser::documentationMarkup() const {
    std::set<std::string> created;                      // sections that we've created

    // The man pages starts with some sections that are always present in the same order.
    std::string doc = "@section{Name}{" + docForSection("name") + "}\n" +
                      "@section{Synopsis}{" + docForSection("synopsis") + "}\n" +
                      "@section{Description}{" + docForSection("description") + "}\n" +
                      "@section{Options}{" + docForSection("options") + "}\n";
    created.insert("name");
    created.insert("synopsis");
    created.insert("description");
    created.insert("options");

    // Append user-defined sections
    for (StringStringMap::const_iterator soi=sectionOrder_.begin(); soi!=sectionOrder_.end(); ++soi) {
        if (created.insert(boost::to_lower_copy(soi->second)).second)
            doc += "@section{" + soi->second + "}{" + docForSection(soi->second) + "}\n";
    }

    // This section is always at the bottom unless the user forces it elsewhere.
    if (created.insert("see also").second)
        doc += "@section{See Also}{" + docForSection("see also") + "}\n";
    
    return doc;
}

// Generate an nroff manual page
std::string Parser::manpage() const {
    // The @s tag for expanding switch names from "foo" to "--foo", or whatever is appropriate
    std::map<std::string, std::string> prefixes;
    preferredSwitchPrefixes(prefixes);
    std::string bestShort = properties_.shortPrefixes.empty() ?
                            std::string("-") :
                            properties_.shortPrefixes.front();
    std::string bestLong = properties_.longPrefixes.empty() ?
                           std::string("--") :
                           properties_.longPrefixes.front();

    // Make some properties available in the markup
    PropTagPtr properties = PropTag::instance();
    properties
        ->with("inclusionPrefix", inclusionPrefixes_.empty() ? std::string() : inclusionPrefixes_.front())
        ->with("terminationSwitch", terminationSwitches_.empty() ? std::string() : terminationSwitches_.front())
        ->with("programName", programName())
        ->with("purpose", purpose())
        ->with("versionString", version().first)
        ->with("versionDate", version().second)
        ->with("chapterNumber", boost::lexical_cast<std::string>(chapter().first))
        ->with("chapterName", chapter().second);

    // This tag decl will accumulate all the @man references
    SeeAlsoTagPtr seeAlso = SeeAlsoTag::instance();

    Markup::Parser mp;
    mp.registerTag(SwitchTag::instance(prefixes, bestShort, bestLong), "s");
    mp.registerTag(ManTag::instance(seeAlso), "man");
    mp.registerTag(seeAlso, "seeAlso");
    mp.registerTag(properties, "prop");

    Markup::ParserResult markup = mp.parse(documentationMarkup());
    std::string chapterNumberStr = boost::lexical_cast<std::string>(chapter().first);
    Markup::RoffFormatterPtr nroff = Markup::RoffFormatter::instance(programName(), chapterNumberStr, chapter().second);
    nroff->version(version().first, version().second);
    std::ostringstream ss;
    markup.emit(ss, nroff);
    return ss.str();
}

// Try to determine screen width.
int Parser::terminalWidth() {
    int ttyfd = isatty(1) ? 1 : (isatty(0) ? 1 : (isatty(2) ? 2 : -1));
    if (ttyfd >= 0) {
        struct winsize ws;
        if (-1 != ioctl(ttyfd, TIOCGWINSZ, &ws))
            return ws.ws_col;
    }
        
    if (const char *columns = getenv("COLUMNS")) {
        char *rest = NULL;
        errno = 0;
        int n = strtol(columns, &rest, 0);
        if (0==errno && rest!=columns && !*rest)
            return n;
    }

    return 80;
}

// Send manual to the output
// nroff -mandoc -rLL=156n -rLT=156n |ul
void Parser::emitDocumentationToPager() const {
    std::string doc = manpage();
    int actualWidth = terminalWidth();
    int width = std::min(actualWidth * 39/40, std::max(actualWidth-2, 20));
    std::string cmd = std::string("nroff -man ") +
                      "-rLL=" + boost::lexical_cast<std::string>(width) + "n " +
                      "-rLT=" + boost::lexical_cast<std::string>(width) + "n " +
                      "| less";
    FILE *proc = popen(cmd.c_str(), "w");
    if (!proc)
        throw std::runtime_error("cannot run \"" + cmd + "\"");
    fputs(doc.c_str(), proc);
    pclose(proc);
}

} // namespace
} // namespace

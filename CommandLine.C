#include "CommandLine.h"

#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <cassert>
#include <cerrno>
#include <set>
#include <sstream>

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
        assert(strings_[loc_.idx].size() >= loc_.offset);
        size_t n = std::min(nchars, (strings_[loc_.idx].size() - loc_.offset));
        loc_.offset += n;
        nchars -= n;
        while (loc_.idx < strings_.size() && loc_.offset >= strings_[loc_.idx].size()) {
            ++loc_.idx;
            loc_.offset = 0;
        }
    }
}


        

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Parsers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

boost::any ValueParser::matchString(const std::string &str) {
    Cursor cursor = str;
    boost::any retval = match(cursor);
    if (cursor.atArgBegin())
        throw std::runtime_error("not matched");
    if (!cursor.atEnd())
        throw std::runtime_error("extra text after end of value");
    return retval;
}

boost::any ValueParser::match(Cursor &cursor) {
    Cursor start = cursor;
    return (*this)(cursor);
}

// Only called by match().  If the subclass doesn't override this, then we try calling the C-string version instead.
boost::any ValueParser::operator()(Cursor &cursor) {
    std::string str = cursor.rest();
    const char *s = str.c_str();
    const char *rest = s;
    boost::any retval = (*this)(s, &rest);
    if (NULL!=rest) {
        assert(rest>=s && rest<= s+strlen(s));
        cursor.consumeChars(rest-s);
    }
    return retval;
}

// only called by ValueParser::operator()(Cursor&)
boost::any ValueParser::operator()(const char *s, const char **rest) {
    throw std::runtime_error("subclass must implement an operator() with a cursor or C strings");
}

boost::any AnyParser::operator()(Cursor &cursor) {
    std::string retval = cursor.rest();
    cursor.consumeArg();
    return retval;
}

boost::any IntegerParser::operator()(const char *s, const char **rest) {
    errno = 0;
    boost::int64_t retval = strtoll(s, (char**)rest, 0);
    if (*rest==s)
        throw std::runtime_error("integer expected");
    while (isspace(**rest)) ++*rest;
    if (ERANGE==errno)
        throw std::range_error("integer overflow");
    return retval;
}

boost::any UnsignedIntegerParser::operator()(const char *s, const char **rest) {
    errno = 0;
    boost::uint64_t retval = strtoull(s, (char**)rest, 0);
    if (*rest==s)
        throw std::runtime_error("unsigned integer expected");
    while (isspace(**rest)) ++*rest;
    if (ERANGE==errno)
        throw std::range_error("integer overflow");
    return retval;
}

boost::any RealNumberParser::operator()(const char *s, const char **rest) {
    double retval = strtod(s, (char**)rest);
    if (*rest==s)
        throw std::runtime_error("real number expected");
    while (isspace(**rest)) ++*rest;
    return retval;
}

boost::any BooleanParser::operator()(const char *s, const char **rest) {
    // False and true values, longest to shortest
    static const char *neg[] = {"false", "off", "no", "0", "f", "n"};
    static const char *pos[] = {"true",  "yes", "on", "1", "t", "y"};
    while (isspace(*s)) ++s;

    for (int negpos=0; negpos<2; ++negpos) {
        const char **list = 0==negpos ? neg : pos;
        size_t listsz = 0==negpos ? sizeof(neg)/sizeof(*neg) : sizeof(pos)/sizeof(*pos);
        for (size_t i=0; i<listsz; ++i) {
            if (0==strncasecmp(list[i], s, strlen(list[i]))) {
                *rest = s + strlen(list[i]);
                while (isspace(**rest)) ++*rest;
                return 0!=negpos;
            }
        }
    }
    throw std::runtime_error("Boolean expected");
}

boost::any StringSetParser::operator()(Cursor &cursor) {
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
    return strings_[bestMatchIdx];
}

ListParser::Ptr ListParser::limit(size_t minLength, size_t maxLength) {
    if (minLength > maxLength)
        throw std::runtime_error("minimum ListParser length must be less than or equal to maximum length");
    if (minLength < elements_.size())
        throw std::runtime_error("minimum ListParser length is smaller than the number of elements specified");
    minLength_ = minLength;
    maxLength_ = maxLength;
    return boost::dynamic_pointer_cast<ListParser>(shared_from_this());
}

boost::any ListParser::operator()(Cursor &cursor) {
    assert(!elements_.empty());
    ExcursionGuard guard(cursor);                       // parsing the list should be all or nothing
    std::vector<boost::any> retval;
    std::string sep = "";

    for (size_t i=0; i<maxLength_; ++i) {
        const ParserSep &ps = elements_[std::min(i, elements_.size()-1)];

        // Advance over the value separator
        if (0!=i) {
            std::string str = cursor.rest();
            const char *s = str.c_str();
            boost::regex re("\\A" + sep);
            boost::cmatch matched;
            if (!regex_search(s, matched, re))
                return boost::any();
            cursor.consumeChars(matched.str().size());
            sep = ps.second;
        }

        // Parse the value
        boost::any value = ps.first->match(cursor);
        retval.push_back(value);
    }

    if (retval.size()<minLength_ || retval.size()>maxLength_) {
        std::ostringstream ss;
        if (minLength_ == maxLength_) {
            ss <<"list with " <<maxLength_ <<" element" <<(1==maxLength_?"":"s") <<" expected (got " <<retval.size() <<")";
            throw std::runtime_error(ss.str());
        } else if (minLength_+1 == maxLength_) {
            ss <<"list with " <<minLength_ <<" or " <<maxLength_ <<" element" <<(1==maxLength_?"":"s") <<" expected"
               <<" (got " <<retval.size() <<")";
            throw std::runtime_error(ss.str());
        } else {
            std::ostringstream ss;
            ss <<"list with " <<minLength_ <<" to " <<maxLength_ <<" elements expected (got " <<retval.size() <<")";
            throw std::runtime_error(ss.str());
        }
    }

    guard.cancel();
    return retval;
}

AnyParser::Ptr anyParser() { return AnyParser::instance(); }
IntegerParser::Ptr integerParser() { return IntegerParser::instance(); }
UnsignedIntegerParser::Ptr unsignedIntegerParser() { return UnsignedIntegerParser::instance(); }
RealNumberParser::Ptr realNumberParser() { return RealNumberParser::instance(); }
BooleanParser::Ptr booleanParser() { return BooleanParser::instance(); }
StringSetParser::Ptr stringSetParser() { return StringSetParser::instance(); }
ListParser::Ptr listParser(const ValueParser::Ptr &p, const std::string &sep) { return ListParser::instance(p, sep); }

/*******************************************************************************************************************************
 *                                      Actions
 *******************************************************************************************************************************/

void ExitProgram::operator()() {
    exit(exitStatus_);
}

void ShowVersion::operator()() {
    std::cerr <<versionString_ <<"\n";
}

void ShowHelp::operator()() {
    stream_ <<"FIXME[Robb Matzke 2014-02-14]: This is the help message!\n";
}

ExitProgram::Ptr exitProgram(int exitStatus) { return ExitProgram::instance(exitStatus); }
ShowVersion::Ptr showVersion(const std::string &versionString) { return ShowVersion::instance(versionString); }
ShowHelp::Ptr showHelp(std::ostream &o) { return ShowHelp::instance(o); }


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
    
void ParsedValue::print(std::ostream &o) const {
    if (switch_) {
        o <<"{switch=\"" <<switchString_ <<"\" at " <<switchLocation_ <<" key=\"" <<switch_->key() <<"\""
          <<"; value str=\"" <<valueString_ <<"\" at " <<valueLocation_
          <<"; seq={s" <<switchSequence_ <<", k" <<keySequence_ <<"}"
          <<"}";
    } else {
        o <<"empty";
    }
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

std::string Switch::synopsis() const {
    return "FIXME[Robb Matzke 2014-02-14]: This is the synopsis!";
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

Switch& Switch::resetShortPrefixes(const std::string &s1, const std::string &s2, const std::string &s3, const std::string &s4) {
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

Switch& Switch::resetValueSeparators(const std::string &s1, const std::string &s2, const std::string &s3, const std::string &s4) {
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

Switch& Switch::defaultValue(const std::string &text, const ValueParser::Ptr &p) {
    defaultValue_ = p->matchString(text);
    defaultValueString_ = text;
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
    ss <<"not enough arguments for switch \"" <<switchString <<"\", found " <<nargs <<" but expected ";
    if (arguments_.size() != nRequiredArguments())
        ss <<"at least ";
    ss <<nRequiredArguments();
    return std::runtime_error(ss.str());
}

std::runtime_error Switch::noSeparator(const std::string &switchString, const Cursor &cursor,
                                       const ParsingProperties &props) const {
    std::string str = "expected one of the following separators between switch \"" + switchString + "\" and its argument:";
    BOOST_FOREACH (std::string sep, props.valueSeparators) {
        if (0!=sep.compare(" "))
            str += " \"" + sep + "\"";
    }
    return std::runtime_error(str);
}

std::runtime_error Switch::extraTextAfterArgument(const std::string &switchString, const Cursor &cursor) const {
    std::string str = "unexpected extra text after switch \"" + switchString + "\" argument: " + cursor.rest();
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
    assert(cursor.atArgBegin());
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

// matches short or long arguments. Counts only arguments that are actually present.  The first present switch argument starts
// at the cursor, and subsequent switch arguments must be aligned at the beginning of a program argument.
size_t Switch::matchArguments(const std::string &switchString, Cursor &cursor /*in,out*/, ParsedValues &result /*out*/) const {
    ExcursionGuard guard(cursor);
    size_t retval = 0;
    BOOST_FOREACH (const SwitchArgument &sa, arguments_) {
        if (retval>0 && !cursor.atArgBegin())
            throw extraTextAfterArgument(switchString, cursor);

        Location valueLocation = cursor.location();
        try {
            boost::any value = sa.parser()->match(cursor);
            if (cursor.location() == valueLocation && sa.isRequired())
                throw std::runtime_error("not found");
            result.push_back(ParsedValue(value, valueLocation, cursor.substr(valueLocation)));
            ++retval;
        } catch (const std::runtime_error &e) {
            if (sa.isRequired())
                throw missingArgument(switchString, cursor, sa, e.what());
            result.push_back(ParsedValue(sa.defaultValue(), NOWHERE, sa.defaultValueString()));
        }
    }
    guard.cancel();
    return retval;
}

void Switch::matchLongArguments(const std::string &switchString, Cursor &cursor /*in,out*/, const ParsingProperties &props,
                                ParsedValues &result /*out*/) const {
    ExcursionGuard guard(cursor);

    // If the switch has no declared arguments, then parse its default.
    if (arguments_.empty()) {
        assert(cursor.atArgBegin() || cursor.atEnd());
        result.push_back(ParsedValue(defaultValue_, NOWHERE, defaultValueString_));
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
    size_t nValuesParsed = matchArguments(switchString, cursor, result /*out*/);

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
        result.push_back(ParsedValue(defaultValue_, NOWHERE, defaultValueString_));
        return;
    }

    // Parse the arguments for this switch.
    size_t nValuesParsed = matchArguments(switchString, cursor, result /*out*/);
    if (nValuesParsed < nRequiredArguments())
        throw notEnoughArguments(switchString, cursor, nValuesParsed);
}

void Switch::runActions() const {
    BOOST_FOREACH (const SwitchAction::Ptr &action, actions_)
        action->run();
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

void ParserResult::insert(ParsedValues &pvals) {
    std::set<const Switch*> seen;
    BOOST_FOREACH (ParsedValue &pval, pvals) {
        const Switch *sw = &pval.createdBy();

        // Get sequences for this value and update the value.
        size_t keySequence = keyIndex_[sw->key()].size();
        size_t switchSequence = switchIndex_[sw].size();
        pval.sequenceInfo(keySequence, switchSequence);

        // Insert the parsed value and update all the indexes
        size_t idx = values_.size();
        values_.push_back(pval);
        keyIndex_[sw->key()].push_back(idx);
        switchIndex_[sw].push_back(idx);
        argvIndex_[pval.switchLocation()].push_back(idx);

#if 1 /*DEBUGGING [Robb Matzke 2014-02-18]*/
        std::cerr <<"    " <<pval <<"\n";
#endif

        // Run the switch actions, but only once per switch (e.g., "-vvv" where "v" is a short switch will cause the action to
        // run only once.  Long switches won't have this ambiguity since insert() will be called with only one long switch at a
        // time.
        if (seen.insert(sw).second)
            sw->runActions();
    }
}

void ParserResult::skip(const Location &loc) {
    skippedIndex_.push_back(loc.idx);
}

void ParserResult::terminator(const Location &loc) {
    terminators_.push_back(loc.idx);
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
        retval.push_back(argv_[idx]);
    return retval;
}

std::vector<std::string> ParserResult::remainingArgs() const {
    std::vector<std::string> retval;
    for (size_t i=stoppedAt_.idx; i<argv_.size(); ++i)
        retval.push_back(argv_[i]);
    return retval;
}

std::vector<std::string> ParserResult::unparsedArgs() const {
    std::set<size_t> indexes;
    BOOST_FOREACH (size_t idx, skippedIndex_)
        indexes.insert(idx);
    BOOST_FOREACH (size_t idx, terminators_)
        indexes.insert(idx);
    for (size_t i=stoppedAt_.idx; i<argv_.size(); ++i)
        indexes.insert(i);

    std::vector<std::string> retval;
    BOOST_FOREACH (size_t idx, indexes)
        retval.push_back(argv_[idx]);
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
    ParserResult result(programArguments);
    Cursor cursor(programArguments);

    while (!cursor.atEnd()) {
        assert(cursor.atArgBegin());
        result.stoppedAt(cursor.location());

        // Check for termination switch.
        BOOST_FOREACH (std::string termination, terminationSwitches_) {
            if (0==cursor.arg().compare(termination)) {
                result.terminator(cursor.location());
                cursor.consumeArg();
                result.stoppedAt(cursor.location());
                return result;
            }
        }
        
        // Does this look like a switch (even one that we might not know about)?
        bool isSwitch = apparentSwitch(cursor);
        if (!isSwitch) {
            if (skipNonSwitches_) {
                result.skip(cursor.location());
                cursor.consumeArg();
                result.stoppedAt(cursor.location());
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
                result.stoppedAt(cursor.location());
                continue;
            } else {
                throw;
            }
        }
        result.stoppedAt(cursor.location());
    }

    return result;                                      // reached end of program arguments
}

bool Parser::parseOneSwitch(Cursor &cursor, ParserResult &result) {
    boost::optional<std::runtime_error> saved_error;

    // Single long switch
    ParsedValues values;
    if (parseLongSwitch(cursor, values, saved_error /*out*/)) {
        result.insert(values);
        return true;
    }

    if (!shortMayNestle_) {
        // Single short switch
        if (parseShortSwitch(cursor, values, saved_error)) {
            if (!cursor.atArgBegin() && !cursor.atEnd()) {
                assert(!values.empty());
                const Switch &sw = values.front().createdBy();
                throw sw.extraTextAfterArgument(values.front().switchString(), cursor);
            }
            result.insert(values);
            return true;
        }
    } else {
        // Or multiple short switches.  If short switches are nestled, then the result is affected only if all the nesltled
        // switches can be parsed.
        bool allParsed = false;
        ExcursionGuard guard(cursor);
        while (parseShortSwitch(cursor, values, saved_error)) {
            if (cursor.atArgBegin() || cursor.atEnd()) {
                allParsed = true;
                break;
            }
        }
        if (allParsed) {
            result.insert(values);
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
bool Parser::parseLongSwitch(Cursor &cursor, ParsedValues &parsedValues, boost::optional<std::runtime_error> &saved_error) {
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
                        pv.switchInfo(&sw, switchLocation, switchString);
                    parsedValues.insert(parsedValues.end(), pvals.begin(), pvals.end());
                    guard.cancel();
                    return true;
                } catch (const std::runtime_error &e) {
                    saved_error = e;
                }
            }
        }
    }
    return false;
}

// Parse one short switch and advance the cursor over the switch name and arguments.
bool Parser::parseShortSwitch(Cursor &cursor, ParsedValues &parsedValues, boost::optional<std::runtime_error> &saved_error) {
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
                        pv.switchInfo(&sw, switchLocation, switchString);
                    parsedValues.insert(parsedValues.end(), pvals.begin(), pvals.end());
                    guard.cancel();
                    return true;
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
                if (!prefix.empty() && boost::starts_with(cursor.arg(), prefix))
                    return true;
            }
            BOOST_FOREACH (const std::string &prefix, swProps.shortPrefixes) {
                if (!prefix.empty() && boost::starts_with(cursor.arg(), prefix))
                    return true;
            }
        }
    }
    return false;
}

} // namespace
} // namespace

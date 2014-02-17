#include "CommandLine.h"

#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <cassert>
#include <cerrno>
#include <sstream>

namespace Sawyer {
namespace CommandLine {

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


/*******************************************************************************************************************************
 *                                      Parsers
 *******************************************************************************************************************************/

boost::any ValueParser::match(const char *s, const char **rest) {
    assert(s!=NULL);
    assert(rest!=NULL);
    *rest = NULL;
    boost::any retval = (*this)(s, rest);
    assert(rest!=NULL);
    assert(*rest >= s);
    return retval;
}

boost::any ValueParser::matchAll(const std::string &str) {
    const char *s = str.c_str();
    const char *rest = NULL;
    boost::any retval = match(s, &rest);
    if (*rest)
        throw NotMatched();
    return retval;
}

boost::any AnyParser::operator()(const char *s, const char **rest) {
    *rest = s + strlen(s);
    return std::string(s);
}

boost::any IntegerParser::operator()(const char *s, const char **rest) {
    errno = 0;
    boost::int64_t retval = strtoll(s, (char**)rest, 0);
    if (*rest==s)
        throw NotMatched();
    while (isspace(**rest)) ++*rest;
    if (ERANGE==errno)
        throw std::range_error("integer overflow for \"" + std::string(s, *rest-s) + "\"");
    return retval;
}

boost::any UnsignedIntegerParser::operator()(const char *s, const char **rest) {
    errno = 0;
    boost::uint64_t retval = strtoull(s, (char**)rest, 0);
    if (*rest==s)
        throw NotMatched();
    while (isspace(**rest)) ++*rest;
    if (ERANGE==errno)
        throw std::range_error("integer overflow for \"" + std::string(s, *rest-s) + "\"");
    return retval;
}

boost::any RealNumberParser::operator()(const char *s, const char **rest) {
    double retval = strtod(s, (char**)rest);
    if (*rest==s)
        throw NotMatched();
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
    throw NotMatched();
}

boost::any StringSetParser::operator()(const char *s, const char **rest) {
    std::string input(s);
    size_t bestMatchIdx = (size_t)(-1), bestMatchLen = 0;
    for (size_t i=0; i<strings_.size(); ++i) {
        if (boost::starts_with(input, strings_[i]) && ((size_t)(-1)==bestMatchIdx || strings_[i].size()>bestMatchLen)) {
            bestMatchIdx = i;
            bestMatchLen = strings_[i].size();
        }
    }
    if ((size_t)(-1)==bestMatchIdx)
        throw NotMatched();
    *rest = s + bestMatchLen;
    return strings_[bestMatchIdx];
}

ListParserPtr ListParser::limit(size_t minLength, size_t maxLength) {
    if (minLength > maxLength)
        throw std::runtime_error("minimum ListParser length must be less than or equal to maximum length");
    if (minLength < elements_.size())
        throw std::runtime_error("minimum ListParser length is smaller than the number of elements specified");
    minLength_ = minLength;
    maxLength_ = maxLength;
    return boost::dynamic_pointer_cast<ListParser>(shared_from_this());
}

boost::any ListParser::operator()(const char *input, const char **rest) {
    assert(!elements_.empty());
    std::vector<boost::any> retval;
    std::string sep = "";
    const char *s = input;
    for (size_t i=0; i<maxLength_; ++i) {
        const ParserSep &ps = elements_[std::min(i, elements_.size()-1)];

        // Advance over the value separator
        if (0!=i) {
            boost::regex re("\\A" + sep);
            boost::cmatch matched;
            if (!regex_search(s, matched, re)) {
                *rest = s;
                break;
            }
            s += matched.str().size();
            sep = ps.second;
        }

        // Parse the value
        boost::any value = (*ps.first)(s, rest);
        retval.push_back(value);
        s = *rest;
    }

    if (retval.size()<minLength_ || retval.size()>maxLength_)
        throw NotMatched();
    return retval;
}

AnyParserPtr anyParser() { return AnyParserPtr(new AnyParser); }
IntegerParserPtr integerParser() { return IntegerParserPtr(new IntegerParser); }
UnsignedIntegerParserPtr unsignedIntegerParser() { return UnsignedIntegerParserPtr(new UnsignedIntegerParser); }
RealNumberParserPtr realNumberParser() { return RealNumberParserPtr(new RealNumberParser); }
BooleanParserPtr booleanParser() { return BooleanParserPtr(new BooleanParser); }
StringSetParserPtr stringSetParser() { return StringSetParserPtr(new StringSetParser); }
ListParserPtr listParser(const ValueParserPtr &p, const std::string &sep) { return ListParserPtr(new ListParser(p, sep)); }

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

ExitProgramPtr exitProgram(int exitStatus) { return ExitProgramPtr(new ExitProgram(exitStatus)); }
ShowVersionPtr showVersion(const std::string &versionString) { return ShowVersionPtr(new ShowVersion(versionString)); }
ShowHelpPtr showHelp(std::ostream &o) { return ShowHelpPtr(new ShowHelp(o)); }


/*******************************************************************************************************************************
 *                                      Parsed values
 *******************************************************************************************************************************/

void ParsedValue::print(std::ostream &o) const {
    if (switch_) {
        o <<"{switch=\"" <<switchString_ <<"\" at " <<switchLocation_ <<" key=\"" <<switch_->key() <<"\""
          <<"; value str=\"" <<valueString_ <<"\" at " <<valueLocation_
          <<"; ord={s" <<switchOrdinal_ <<", k" <<keyOrdinal_ <<"}"
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

void Switch::init(const std::string &longName, const std::string &shortNames) {
    shortNames_ = shortNames;
    if (!longName.empty()) {
        longNames_.push_back(longName);
        key_ = documentationKey_ = longName;
    } else if (!shortNames.empty()) {
        key_ = documentationKey_ = std::string(1, shortNames[0]);
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

Switch& Switch::argument(const std::string &name, const ValueParserPtr &parser) {
    return argument(SwitchArgument(name, parser));
}

Switch& Switch::argument(const std::string &name, const ValueParserPtr &parser, const std::string &defaultValueStr) {
    return argument(SwitchArgument(name, parser, defaultValueStr));
}

Switch& Switch::defaultValue(const std::string &text, const ValueParserPtr &p) {
    defaultValue_ = p->matchAll(text);
    return *this;
}

Switch& Switch::action(const SwitchActionPtr &action) {
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
    
size_t Switch::matchLongName(const std::string &arg, const ParsingProperties &props) const {
    BOOST_FOREACH (const std::string &prefix, props.longPrefixes) {
        if (boost::starts_with(arg, prefix)) {
            std::string rest = arg.substr(prefix.size());
            BOOST_FOREACH (const std::string &name, longNames_) {
                if (boost::starts_with(rest, name)) {
                    size_t retval = prefix.size() + name.size();
                    rest = rest.substr(name.size());
                    if (rest.empty())
                        return retval;                  // switch name matches to end of program argument
                    if (0==arguments_.size())
                        continue;                       // switch name does not match to end, but has no declared args
                    BOOST_FOREACH (const std::string &sep, props.valueSeparators) {
                        if (0!=sep.compare(" ") && boost::starts_with(rest, sep))
                            return retval;              // found prefix, name, and separator for switch with args
                    }
                }
            }
        }
    }
    return 0;
}

// Throws an exception if it cannot parse. The cursor might be updated even if an exception is thrown.
void Switch::matchArguments(const std::string &switchString, Cursor &cursor /*in,out*/, const ParsingProperties &props,
                            ParsedValues &result /*out*/) const {
    // If the switch has no declared arguments, then parse its default.
    if (arguments_.empty()) {
        assert(cursor.atArgEnd());
        cursor.consumeArg();
        result.push_back(ParsedValue(defaultValue_, NOWHERE, ""));
        return;
    }

    // Try to match the name/value separator.  Advance the cursor to the first character of the first value.
    bool matchedSeparator = false;
    if (cursor.atArgEnd()) {
        if (matchAnyString(props.valueSeparators, " ")) {
            cursor.consumeArg();
            matchedSeparator = true;
        }
    } else {
        std::string s = cursor.rest();
        BOOST_FOREACH (const std::string &sep, props.valueSeparators) {
            if (boost::starts_with(s, sep)) {
                cursor.consumeChars(sep.size());
                matchedSeparator = true;
            }
        }
    }

    // If we couldn't match the name/value separator then this switch cannot have any values.
    if (!matchedSeparator && nRequiredArguments()>0)
        throw noSeparator(switchString, cursor, props);
    
    // Try to match arguments, some of which might be optional.  Each value must end at the end of a program argument.
    // Some arguments might be optional.
    size_t nValuesParsed = 0; // number of values actually parsed from the program arguments, not assigned default values
    BOOST_FOREACH (const SwitchArgument &sa, arguments_) {
        if (cursor.atEnd()) {
            if (sa.isRequired()) {
                break;
            } else {
                result.push_back(ParsedValue(sa.defaultValue(), NOWHERE, ""));
            }
        } else {
            const char *s = cursor.rest().c_str();
            const char *rest = NULL;
            if (sa.isRequired()) {
                boost::any value = sa.parser()->match(s, &rest);
                result.push_back(ParsedValue(value, cursor.location(), std::string(s, rest)));
            } else {
                try {
                    boost::any value = sa.parser()->match(s, &rest);
                    result.push_back(ParsedValue(value, cursor.location(), std::string(s, rest)));
                } catch (const NotMatched&) {
                    result.push_back(ParsedValue(sa.defaultValue(), NOWHERE, ""));
                    continue;
                }
            }

            ++nValuesParsed;
            cursor.consumeChars(rest-s);
            if (cursor.atArgEnd())
                cursor.consumeArg();
        }
    }

    if (!cursor.atArgBegin() && !cursor.atArgEnd())
        throw extraTextAfterArgument(switchString, cursor);
    if (nValuesParsed < nRequiredArguments())
        throw notEnoughArguments(switchString, cursor, nValuesParsed);
}

void Switch::runActions() const {
    BOOST_FOREACH (const SwitchActionPtr &action, actions_)
        (*action)();
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
    throw NotMatched();
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
    throw NotMatched();
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
    ParserResult result;
    Cursor cursor(programArguments);
    while (!cursor.atEnd()) {
        assert(cursor.atArgBegin());
        ParsedValues valuesForSwitch;
        const Switch *sw = parseOneSwitch(cursor, valuesForSwitch);
        if (NULL==sw)
            break;
#if 1 /*DEBUGGING [Robb Matzke 2014-02-17]*/
        std::cerr <<"ROBB: parsed switch and got these values:\n";
        BOOST_FOREACH (const ParsedValue &v, valuesForSwitch)
            std::cerr <<"    " <<v <<"\n";
#endif

        sw->runActions();
    }
    return result;
}

const Switch* Parser::parseOneSwitch(Cursor &cursor, ParsedValues &parsedValues /*out*/) {
    assert(cursor.atArgBegin());
    parsedValues.clear();

    // Check for termination switch.
    BOOST_FOREACH (std::string termination, terminationSwitches_) {
        if (0==cursor.arg().compare(termination))
            return NULL;                                // FIXME[Robb Matzke 2014-02-17]: should we save this info?
    }

    // Try to parse one switch.
    boost::optional<std::runtime_error> saved_error;
    BOOST_FOREACH (const SwitchGroup &sg, switchGroups_) {
        ParsingProperties sgProps = sg.properties().inherit(properties_);
        BOOST_FOREACH (const Switch &sw, sg.switches()) {
            ParsingProperties swProps = sw.properties().inherit(sgProps);
            if (size_t nchars = sw.matchLongName(cursor.arg(), swProps)) {
                const std::string switchString = cursor.arg().substr(0, nchars);
                Location switchLocation = cursor.location();

                ExcursionSaver excursion(cursor);
                cursor.consumeChars(nchars);
                try {
                    sw.matchArguments(switchString, cursor, swProps, parsedValues /*out*/);
                    BOOST_FOREACH (ParsedValue &parsed, parsedValues) {
                        parsed.switchInfo(&sw, switchLocation, switchString);
                    }
                    excursion.cancel();
                    return &sw;
                } catch (const std::runtime_error &e) {
                    saved_error = e;
                    continue;                           // couldn't parse args, so try another switch
                }
            }
        }
    }
    if (saved_error)
        throw *saved_error;                             // found at least one switch but couldn't ever parse arguments

    if (apparentSwitch(cursor))
        throw std::runtime_error("unrecognized switch: " + cursor.arg());

    return NULL;
}

bool Parser::apparentSwitch(const Cursor &cursor) const {
    assert(cursor.atArgBegin());
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

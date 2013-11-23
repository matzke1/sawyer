#include "Sawyer.h"

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <stdexcept>
#include <sstream>

namespace Sawyer {

MessageId Message::next_id_ = 0;
struct timeval MessagePrefix::epoch_;
std::string MessagePrefix::exename_;
MessagePrefix default_prefix;
MessageFileSink merr(std::cerr, isatty(2));
MessageFileSink mout(std::cout, isatty(1));
MessageNullSink mnull;
MessageFacility log("");



/*******************************************************************************************************************************
 *                                      Messages
 *******************************************************************************************************************************/

std::string
stringifyMessageImportance(MessageImportance imp)
{
    switch (imp) {
        case DEBUG: return "DEBUG";
        case TRACE: return "TRACE";
        case WHERE: return "WHERE";
        case INFO:  return "INFO";
        case WARN:  return "WARN";
        case ERROR: return "ERROR";
        case FATAL: return "FATAL";
        default:    return "?????";
    }
}

void
MessageProps::set_color(MessageImportance imp)
{
#if 0
    switch (imp) { // this scheme looks good on black or white backgrounds
        case DEBUG: fgcolor=COLOR_DEFAULT; bgcolor=COLOR_DEFAULT; color_attr=0; break;
        case TRACE: fgcolor=COLOR_CYAN;    bgcolor=COLOR_DEFAULT; color_attr=0; break;
        case WHERE: fgcolor=COLOR_CYAN;    bgcolor=COLOR_DEFAULT; color_attr=0; break;
        case INFO:  fgcolor=COLOR_GREEN;   bgcolor=COLOR_DEFAULT; color_attr=0; break;
        case WARN:  fgcolor=COLOR_YELLOW;  bgcolor=COLOR_DEFAULT; color_attr=0; break;
        case ERROR: fgcolor=COLOR_RED;     bgcolor=COLOR_DEFAULT; color_attr=0; break;
        case FATAL: fgcolor=COLOR_RED;     bgcolor=COLOR_DEFAULT; color_attr=1; break;
        default: break;
    }
#else
    switch (imp) { // this scheme works on colored backgrounds
        case DEBUG: fgcolor=COLOR_DEFAULT; bgcolor=COLOR_DEFAULT; color_attr=0; break;
        case TRACE: fgcolor=COLOR_DEFAULT; bgcolor=COLOR_DEFAULT; color_attr=0; break;
        case WHERE: fgcolor=COLOR_DEFAULT; bgcolor=COLOR_DEFAULT; color_attr=0; break;
        case INFO:  fgcolor=COLOR_DEFAULT; bgcolor=COLOR_DEFAULT; color_attr=0; break;
        case WARN:  fgcolor=COLOR_DEFAULT; bgcolor=COLOR_DEFAULT; color_attr=1; break;
        case ERROR: fgcolor=COLOR_DEFAULT; bgcolor=COLOR_DEFAULT; color_attr=1; break;
        case FATAL: fgcolor=COLOR_DEFAULT; bgcolor=COLOR_DEFAULT; color_attr=1; break;
        default: break;
    }
#endif
}

void
Message::append(char c)
{
    if ('\n'==c) {
        complete_ = true;
    } else {
        assert(!complete_);
        body_ += c;
    }
}

void
Message::append(const std::string &s)
{
    for (size_t i=0; i<s.size(); ++i)
        append(s[i]);
}



/*******************************************************************************************************************************
 *                                      Message prefixes
 *******************************************************************************************************************************/

void
MessagePrefix::init()
{
    if (exename_.empty()) {
        std::ifstream in("/proc/self/cmdline");
        if (in) {
            char buf[4096];
            memset(buf, 0, sizeof buf);
            in.read(buf, sizeof(buf)-1);
            char *s = buf;
            if (char *slash = strrchr(s, '/'))
                s = slash+1;
            if (!strncmp(s, "lt-", 3))
                s += 3;
            exename_ = s;
        } else {
            exename_ = "?";
        }
    }
    if (epoch_.tv_sec==0 && epoch_.tv_usec==0)
        gettimeofday(&epoch_, NULL);
}

std::string
MessagePrefix::operator()(const MessageProps &props, const std::string &facility_name, MessageImportance imp)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double elapsed = tv.tv_sec - epoch_.tv_sec + 1e-6 * ((double)tv.tv_usec - epoch_.tv_usec);
    std::ostringstream ss;

    if (props.use_color && props.has_color()) {
        const char *semi="";
        ss <<"\033[";
        if (props.fgcolor!=COLOR_DEFAULT) {
            ss <<(30+props.fgcolor);
            semi=";";
        }
        if (props.bgcolor!=COLOR_DEFAULT) {
            ss <<semi <<(40+props.bgcolor);
            semi=";";
        }
        if (props.color_attr)
            ss <<semi <<props.color_attr;
        ss <<"m";
    }

    ss.precision(5);
    ss <<exename_ <<"[" <<getpid() <<"] " <<std::fixed <<elapsed <<"s "
       <<facility_name <<"[" <<std::setw(5) <<std::left <<stringifyMessageImportance(imp) <<"]";

    if (props.has_color())
        ss <<"\033[m";
    ss <<" ";
        
    return ss.str();
}



/*******************************************************************************************************************************
 *                                      Message sinks
 *******************************************************************************************************************************/

void
MessageFileSink::adjust_mprops(MessageProps &mprops)
{
    mprops.use_color = use_color_;
}

void
MessageFileSink::emit(Message &mesg)
{
    // Do not print messages with an empty body
    if (mesg.empty())
        return;

    // Interrupt a previous partial message?
    if (prev_mid_!=NO_MESSAGE && mesg.id()!=prev_mid_) {
        ostream_ <<prev_mprops_.interrupt_string;
        prev_mid_ = NO_MESSAGE;
    }

    // Emit this message
    if (prev_mid_!=mesg.id()) {
        ostream_ <<mesg.prefix() <<std::string(mesg.props().indentation*indent_width_, ' ') <<mesg.body();
    } else {
        ostream_ <<mesg.body().substr(mesg.nprinted());
    }
    mesg.printed();

    // Save properties for partial message for later calls.
    if (mesg.is_partial()) {
        prev_mid_ = mesg.id();
        prev_mprops_ = mesg.props();
    } else {
        ostream_ <<mesg.props().terminate_string;
        prev_mid_ = NO_MESSAGE;
    }
}



/*******************************************************************************************************************************
 *                                      Message streams
 *******************************************************************************************************************************/

void
MessageStreamBuf::init()
{
    assert(sink_!=NULL);
    dflt_mprops_.set_color(importance_);
    sink_->adjust_mprops(dflt_mprops_);
    mesg_.reset(dflt_mprops_);
    initialized_ = true;
}

std::streamsize
MessageStreamBuf::xsputn(const char *s, std::streamsize &n)
{
    assert(n>0);
    if (!initialized_)
        init();

    for (std::streamsize i=0; i<n; ++i) {

        // First letter of a message sets its indentation and prefix.
        if (mesg_.empty() && prefix_) {
            mesg_.props().indentation = sink_->indentation();
            mesg_.prefix((*prefix_)(mesg_.props(), name_, importance_));
        }
        mesg_.append(s[i]);

        // A linefeed marks the end of a message, but isn't actually part of its body.  The MessageSink is free to use whatever
        // termination it needs.
        if ('\n'==s[i]) {
            if (enabled_)
                sink_->emit(mesg_);      // full message output
            mesg_.reset(dflt_mprops_);
        }
    }

    // Emit partial message, however the sink wants to handle that.
    if (enabled_ && !mesg_.empty())
        sink_->emit(mesg_);

    return n;
}

MessageStreamBuf::int_type
MessageStreamBuf::overflow(int_type c)
{
    if (!initialized_)
        init();
    if (c==traits_type::eof())
        return traits_type::eof();
    char_type ch = traits_type::to_char_type(c);
    std::streamsize nchars = 1;
    return xsputn(&ch, nchars) == 1 ? c : traits_type::eof();
}

void
MessageStreamBuf::finish() 
{
    if (initialized_ && enabled_ && !mesg_.empty() && mesg_.is_partial()) {
        mesg_.append(mesg_.props().destroy_string);
        sink_->emit(mesg_);
        mesg_.reset(dflt_mprops_);
    }
}

void
MessageStreamBuf::transfer_ownership(MessageStreamBuf &other)
{
    assert(sink_ == other.sink_);
    mesg_.transfer_from(other.mesg_);
}



/*******************************************************************************************************************************
 *                                      Message facilities
 *******************************************************************************************************************************/

void
MessageFacility::init(const std::string &name, MessagePrefix &prefix, MessageSink &sink)
{
    name_ = name;
    memset(streams_, 0, sizeof streams_); // in case of constructor exception
    for (int i=0; i<N_LEVELS; ++i)
        streams_[i] = new MessageStream(name, (MessageImportance)i, prefix, sink);
}
    
void
MessageFacilities::insert(MessageFacility &facility, std::string name)
{
    if (name.empty())
        name = facility.name();
    if (name.empty())
        throw std::logic_error("facility name is empty and no name was supplied");
    const char *s = name.c_str();
    if (0!=name.compare(parse_facility_name(s)))
        throw std::logic_error("name '"+name+"' is not valid for the MessageFacilities::control language");
    FacilityMap::iterator found = facilities_.find(name);
    if (found!=facilities_.end()) {
        if (found->second != &facility)
            throw std::logic_error("message facility '"+name+"' is used more than once");
    } else {
        facilities_.insert(std::make_pair(name, &facility));
    }
}

void
MessageFacilities::erase(MessageFacility &facility)
{
    FacilityMap map = facilities_;;
    for (FacilityMap::iterator fi=map.begin(); fi!=map.end(); ++fi) {
        if (fi->second == &facility)
            facilities_.erase(fi->first);
    }
}

void
MessageFacilities::enable(const std::string &switch_name, bool b)
{
    FacilityMap::iterator found = facilities_.find(switch_name);
    if (found != facilities_.end()) {
        if (b) {
            for (int i=0; i<N_LEVELS; ++i) {
                MessageImportance imp = (MessageImportance)i;
                found->second->get(imp).enable(impset_.find(imp)!=impset_.end());
            }
        } else {
            for (int i=0; i<N_LEVELS; ++i)
                found->second->get((MessageImportance)i).disable();
        }
    }
}
    
void
MessageFacilities::enable(MessageImportance imp, bool b)
{
    if (b) {
        impset_.insert(imp);
    } else {
        impset_.erase(imp);
    }
    for (FacilityMap::iterator fi=facilities_.begin(); fi!=facilities_.end(); ++fi)
        fi->second->get(imp).enable(b);
}

void
MessageFacilities::enable(bool b)
{
    for (FacilityMap::iterator fi=facilities_.begin(); fi!=facilities_.end(); ++fi) {
        MessageFacility *facility = fi->second;
        if (b) {
            for (int i=0; i<N_LEVELS; ++i) {
                MessageImportance imp = (MessageImportance)i;
                facility->get(imp).enable(impset_.find(imp)!=impset_.end());
            }
        } else {
            for (int i=0; i<N_LEVELS; ++i)
                facility->get((MessageImportance)i).disable();
        }
    }
}

std::string
MessageFacilities::ControlTerm::to_string() const
{
    std::string s = enable ? "enable" : "disable";
    if (lo==hi) {
        s += " level " + stringifyMessageImportance(lo);
    } else {
        s += " levels " + stringifyMessageImportance(lo) + " through " + stringifyMessageImportance(hi);
    }
    s += " for " + (facility_name.empty() ? "all registered facilities" : facility_name);
    return s;
}

// Matches the Perl regular expression /^\s*([a-zA-Z]\w*((\.|::)[a-zA-Z]\w*)*/
// On match, returns $1 and str points to the next character after the regular expression
// When not matched, returns "" and str is unchanged
std::string
MessageFacilities::parse_facility_name(const char *&str)
{
    std::string name;
    const char *s = str;
    while (isspace(*s)) ++s;
    while (isalpha(*s)) {
        while (isalnum(*s) || '_'==*s) name += *s++;
        if ('.'==s[0] && (isalpha(s[1]) || '_'==s[1])) {
            name += ".";
            ++s;
        } else if (':'==s[0] && ':'==s[1] && (isalpha(s[2]) || '_'==s[2])) {
            name += "::";
            s += 2;
        }
    }
    if (!name.empty())
        str = s;
    return name;
}

// Matches the Perl regular expression /^\s*([+!]?)/ and returns $1 on success with str pointing to the character after the
// match.  Returns the empty string on failure with str not adjusted.
std::string
MessageFacilities::parse_enablement(const char *&str)
{
    const char *s = str;
    while (isspace(*s)) ++s;
    if ('!'==*s || '+'==*s) {
        str = s+1;
        return std::string(s, 1);
    }
    return "";
}

// Matches the Perl regular expression /^\s*(<=?|>=?)/ and returns $1 on success with str pointing to the character after
// the match. Returns the empty string on failure with str not adjusted.
std::string
MessageFacilities::parse_relation(const char *&str)
{
    const char *s = str;
    while (isspace(*s)) ++s;
    if (!strncmp(s, "<=", 2) || !strncmp(s, ">=", 2)) {
        str = s + 2;
        return std::string(s, 2);
    } else if ('<'==*s || '>'==*s) {
        str = s + 1;
        return std::string(s, 1);
    }
    return "";
}

// Matches the Perl regular expression /^\s*(all|none|debug|trace|where|info|warn|error|fatal)\b/
// On match, returns $1 and str points to the next character after the match
// On failure, returns "" and str is unchanged
std::string
MessageFacilities::parse_importance_name(const char *&str)
{
    static const char *words[] = {"all", "none", "debug", "trace", "where", "info", "warn", "error", "fatal",
                                  "ALL", "NONE", "DEBUG", "TRACE", "WHERE", "INFO", "WARN", "ERROR", "FATAL"};
    static const size_t nwords = sizeof(words)/sizeof(words[0]);

    const char *s = str;
    while (isspace(*s)) ++s;
    for (size_t i=0; i<nwords; ++i) {
        size_t n = strlen(words[i]);
        if (0==strncmp(s, words[i], n) && !isalnum(s[n]) && '_'!=s[n]) {
            str += (s-str) + n;
            return words[i];
        }
    }
    return "";
}

MessageImportance
MessageFacilities::importance_from_string(const std::string &str)
{
    if (0==str.compare("debug") || 0==str.compare("DEBUG"))
        return DEBUG;
    if (0==str.compare("trace") || 0==str.compare("TRACE"))
        return TRACE;
    if (0==str.compare("where") || 0==str.compare("WHERE"))
        return WHERE;
    if (0==str.compare("info")  || 0==str.compare("INFO"))
        return INFO;
    if (0==str.compare("warn")  || 0==str.compare("WARN"))
        return WARN;
    if (0==str.compare("error") || 0==str.compare("ERROR"))
        return ERROR;
    if (0==str.compare("fatal") || 0==str.compare("FATAL"))
        return FATAL;
    abort();
}

// parses a StreamControlList. On success, returns a non-empty vector and adjust 'str' to point to the next character after the
// list.  On failure, throw a ControlError.
std::list<MessageFacilities::ControlTerm>
MessageFacilities::parse_importance_list(const std::string &facility_name, const char *&str)
{
    const char *s = str;
    std::list<ControlTerm> retval;

    while (1) {
        const char *elmt_start = s;

        // List elements are separated by a comma.
        if (!retval.empty()) {
            while (isspace(*s)) ++s;
            if (','!=*s) {
                s = elmt_start;
                break;
            }
            ++s;
        }

        while (isspace(*s)) ++s;
        const char *enablement_start = s;
        std::string enablement = parse_enablement(s);

        while (isspace(*s)) ++s;
        const char *relation_start = s;
        std::string relation = parse_relation(s);

        while (isspace(*s)) ++s;
        const char *importance_start = s;
        std::string importance = parse_importance_name(s);
        if (importance.empty()) {
            if (!enablement.empty() || !relation.empty())
                throw ControlError("message importance level expected", importance_start);
            s = elmt_start;
            break;
        }

        ControlTerm term(facility_name, enablement.compare("!")!=0);
        if (0==importance.compare("all") || 0==importance.compare("none")) {
            if (!enablement.empty())
                throw ControlError("'"+importance+"' cannot be preceded by '"+enablement+"'", enablement_start);
            if (!relation.empty())
                throw ControlError("'"+importance+"' cannot be preceded by '"+relation+"'", relation_start);
            term.lo = DEBUG;
            term.hi = FATAL;
            term.enable = 0!=importance.compare("none");
        } else {
            MessageImportance imp = importance_from_string(importance);
            if (relation.empty()) {
                term.lo = term.hi = importance_from_string(importance);
            } else if (relation[0]=='<') {
                term.lo = DEBUG;
                term.hi = imp;
                if (1==relation.size()) {
                    if (DEBUG==imp)
                        continue; // empty set
                    term.hi = (MessageImportance)(term.hi - 1);
                }
            } else {
                term.lo = imp;
                term.hi = FATAL;
                if (1==relation.size()) {
                    if (FATAL==imp)
                        continue; // empty set
                    term.lo = (MessageImportance)(term.lo + 1);
                }
            }
        }
        retval.push_back(term);
    }

    if (!retval.empty())
        str = s;
    return retval;
}

std::string
MessageFacilities::control(const std::string &ss)
{
    const char *start = ss.c_str();
    const char *s = start;
    std::list<ControlTerm> terms;

    try {
        while (1) {
            std::list<ControlTerm> t2 = parse_importance_list("", s);
            if (t2.empty()) {
                // facility name
                while (isspace(*s)) ++s;
                const char *facility_name_start = s;
                std::string facility_name = parse_facility_name(s);
                if (facility_name.empty())
                    break;
                if (facilities_.find(facility_name)==facilities_.end())
                    throw ControlError("no such message facility '"+facility_name+"'", facility_name_start);

                // stream control list in parentheses
                while (isspace(*s)) ++s;
                if ('('!=*s)
                    throw ControlError("expected '(' after message facility name '"+facility_name+"'", s);
                ++s;
                t2 = parse_importance_list(facility_name, s);
                if (t2.empty())
                    throw ControlError("expected stream control list after '('", s);
                while (isspace(*s)) ++s;
                if (')'!=*s)
                    throw ControlError("expected ')' at end of stream control list for '"+facility_name+"'", s);
                ++s;
            }

            terms.insert(terms.end(), t2.begin(), t2.end());
            while (isspace(*s)) ++s;
            if (','!=*s)
                break;
            ++s;
        }

        while (isspace(*s)) ++s;
        if (*s) {
            if (terms.empty())
                throw ControlError("syntax error", s);
            if (terms.back().facility_name.empty())
                throw ControlError("syntax error in global list", s);
            throw ControlError("syntax error after '"+terms.back().facility_name+"' list", s);
        }
    } catch (const ControlError &error) {
        std::string s = error.mesg + "\n";
        size_t offset = error.input_position - start;
        if (offset <= ss.size()) {
            s += "    error occurred in \"" + ss + "\"\n";
            s += "    at this position   " + std::string(offset, '-') + "^\n";
        }
        return s;
    }

    for (std::list<ControlTerm>::iterator ti=terms.begin(); ti!=terms.end(); ++ti) {
        const ControlTerm &term = *ti;
#if 0 /*DEBUGGING [Robb Matzke 2013-11-22]*/
        static MessageFacility log("Sawyer::MessageFacilities::control");
        log[DEBUG] <<term.to_string() <<"\n";
#endif
        if (term.facility_name.empty()) {
            for (MessageImportance imp=term.lo; imp<=term.hi; imp=(MessageImportance)(imp+1))
                enable(imp, term.enable);
        } else {
            FacilityMap::iterator found = facilities_.find(term.facility_name);
            assert(found!=facilities_.end() && found->second!=NULL);
            for (MessageImportance imp=term.lo; imp<=term.hi; imp=(MessageImportance)(imp+1))
                found->second->get(imp).enable(term.enable);
        }
    }

    return ""; // no errors
}

void
MessageFacilities::print(std::ostream &log) const
{
    for (int i=0; i<N_LEVELS; ++i) {
        MessageImportance mi = (MessageImportance)i;
        log <<(impset_.find(mi)==impset_.end() ? '-' : stringifyMessageImportance(mi)[0]);
    }
    log <<" default enabled levels\n";

    if (facilities_.empty()) {
        log <<"no message facilities registered\n";
    } else {
        for (FacilityMap::const_iterator fi=facilities_.begin(); fi!=facilities_.end(); ++fi) {
            MessageFacility *facility = fi->second;

            // A short easy to read format. Letters indicate the importances that are enabled; dashes keep them aligned.
            // Sort of like the format 'ls -l' uses to show permissions.
            for (int i=0; i<N_LEVELS; ++i) {
                MessageImportance mi = (MessageImportance)i;
                log <<(facility->get(mi) ? stringifyMessageImportance(mi)[0] : '-');
            }
            log <<" " <<fi->first <<"\n";
        }
    }
}

/*******************************************************************************************************************************
 *                                      Manipulators
 *******************************************************************************************************************************/

std::ostream& operator<<(std::ostream &o, const pint &m)
{
    MessageStream *mstream = dynamic_cast<MessageStream*>(&o);
    assert(mstream!=NULL);
    mstream->interrupt_string(m.s, false);
    return o;
}

std::ostream& operator<<(std::ostream &o, const pdest &m)
{
    MessageStream *mstream = dynamic_cast<MessageStream*>(&o);
    assert(mstream!=NULL);
    mstream->destroy_string(m.s, false);
    return o;
}


} // namespace

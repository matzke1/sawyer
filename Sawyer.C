#include "Sawyer.h"

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
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

std::streamsize
MessageStreamBuf::xsputn(const char *s, std::streamsize &n)
{
    assert(n>0);
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
    if (c==traits_type::eof())
        return traits_type::eof();
    char_type ch = traits_type::to_char_type(c);
    std::streamsize nchars = 1;
    return xsputn(&ch, nchars) == 1 ? c : traits_type::eof();
}

void
MessageStreamBuf::finish() 
{
    if (enabled_ && !mesg_.empty() && mesg_.is_partial()) {
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
    memset(streams_, 0, sizeof streams_); // in case of constructor exception
    for (int i=0; i<N_LEVELS; ++i)
        streams_[i] = new MessageStream(name, (MessageImportance)i, prefix, sink);
}
    
void
MessageFacilities::insert(MessageFacility &facility, std::string switch_name)
{
    if (switch_name.empty())
        switch_name = facility.name();
    if (switch_name.empty())
        throw std::logic_error("no switch name and facility name is empty");

    FacilityMap::iterator found = facilities_.find(switch_name);
    if (found!=facilities_.end()) {
        if (found->second != &facility)
            throw std::logic_error("name is used more than once: "+switch_name);
    } else {
        facilities_.insert(std::make_pair(switch_name, &facility));
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
            for (ImportanceSet::iterator ii=impset_.begin(); ii!=impset_.end(); ++ii)
                found->second->get(*ii).enable();
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

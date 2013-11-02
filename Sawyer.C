#include "Sawyer.h"

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>

namespace Sawyer {

struct timeval MessagePrefix::epoch_;
std::string MessagePrefix::exename_;
unsigned MessageStreamBuf::next_id_ = 0;
MessagePrefix default_prefix;
MessageFileSink merr(std::cerr, isatty(2));
MessageFileSink mout(std::cout, isatty(1));
MessageNullSink mnull;
MessageFacility log("");

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

void
MessageFileSink::adjust_mprops(MessageProps &mprops)
{
    mprops.use_color = use_color_;
}

void
MessageFileSink::output(const MessageProps &mesg, const std::string &full_line, size_t new_stuff)
{
    assert(new_stuff<=full_line.size());
    if (new_stuff < full_line.size()) {
        if (prev_mesg_.stream_id!=NO_STREAM && prev_mesg_.stream_id!=mesg.stream_id) {
            if (need_linefeed_)
                ostream_ <<prev_mesg_.interrupt_string;
            ostream_ <<full_line; // includes the new_stuff
        } else {
            ostream_ <<full_line.substr(new_stuff);
        }
        prev_mesg_ = mesg;
        need_linefeed_ = '\n'!=full_line[full_line.size()-1];
    }
}

void
MessageStreamBuf::finish() 
{
    if (!buf_.empty() && enabled_) {
        size_t newstuff = buf_.size();
        buf_ += mesg_props_.cleanup_string;
        sink_->output(mesg_props_, buf_, newstuff);
    }
}

std::streamsize
MessageStreamBuf::xsputn(const char *s, std::streamsize &n)
{
    assert(n>0);
    assert(new_stuff_ <= buf_.size());
    for (std::streamsize i=0; i<n; ++i) {
        if (buf_.empty())
            buf_ = (*prefix_)(mesg_props_, name_, importance_) + std::string(sink_->indent(0)*indent_width_, ' ');

        buf_.push_back(s[i]);
        if ('\n'==s[i]) {
            if (enabled_)
                sink_->output(mesg_props_, buf_, new_stuff_);
            buf_ = "";
            new_stuff_ = 0;
            mesg_props_ = dflt_mesg_props_;
        }
    }
    if (!buf_.empty() && enabled_) {
        sink_->output(mesg_props_, buf_, new_stuff_);
        new_stuff_ = buf_.size();
    }
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
MessageStreamBuf::transfer_ownership(MessageStreamBuf &other)
{
    buf_ = other.buf_; other.buf_ = "";
    new_stuff_ = other.new_stuff_; other.new_stuff_ = 0;
    mesg_props_ = other.mesg_props_;
    mesg_props_.stream_id = dflt_mesg_props_.stream_id;
    sink_->transfer_ownership(mesg_props_.stream_id);
}

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

std::ostream& operator<<(std::ostream &o, const pterm &m)
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
    mstream->cleanup_string(m.s, false);
    return o;
}


} // namespace

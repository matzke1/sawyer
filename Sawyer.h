#ifndef Sawyer_H
#define Sawyer_H

#include <cassert>
#include <cstdlib>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <streambuf>
#include <string>
#include <sys/time.h>

/** An event logger.
 *
 *  This namespace provides functionality to conditionally emit diagnostic messages based on software component and message
 *  importance.
 *
 * @section features Features
 *
 * <ul>
 *   <li>Familiarity. Sawyer uses C++ paradigms for output--code to emit a message looks like any code that writes to an STL
 *       output stream.</li>
 *   <li>Ease of use. Sawyer uses existing C++ output stream insertion operators, so no new code needs to be written to emit
 *       messages for objects.</li>
 *   <li>Type safety. Sawyer avoids the use of unsafe C printf-like varargs functions.</li>
 *   <li>Consistent output.  Sawyer messages have a consistent look since they all contain information about the program
 *       from whence they came.</li>
 *   <li>Partial messages. Sawyer is able to emit messages incrementally for things like progress bars.</li>
 *   <li>Readable output. Sawyer output is human readable even when partial messages overlap or messages are being generated
 *       from multiple threads. It also has the capability to indent output based on run-time nesting.</li>
 *   <li>Granular logging. Sawyer supports multiple logging objects so each software component can have its own logging
 *       facility and coherently combines output across objects.</li>
 *   <li>Flexible output. Sawyer permits messages to be directed to one or more destinations, which can be an STL output
 *       stream, an SQL database, /dev/null, or a user-defined destination.</li>
 *   <li>Run-time configurable. Sawyer has functions that allow collections of message streams to be controlled with a
 *       simple language that can be specified on command lines, in configuration files, from user input, etc.</li>
 *   <li>Compile-time configurable. Saywer provides an easy way for a developer to temporarily enable or disable messages.</li>
 * </ul>
 *
 * @section usage Basic usage
 *
 *  The class of object that programs most often interact with is the MessageFacility, an array of MessageStream objects one
 *  per MessageImportance. The MessageFacility object is often named "log" and a message is created by inserting a line of text
 *  into a MessageStream like this:
 *
 * @code
 *  log[DEBUG] <<"this is a debug message\n";
 * @endcode
 *
 *  The "log[DEBUG]" selects the MessageStream for handling debug-level messages for the "log" MessageFacility, and the message
 *  is created with the normal STL std::ostream insertion operators, stream manipulators, etc.  Each message should eventually
 *  end with a linefeed (do not use std::endl since the message might not be going to the terminal or a text file). Messages
 *  that don't have a linefeed are called "partial messages" and will appear unbuffered in the output, but Sawyer takes care
 *  that other messages don't interfere with partial messages.
 *
 *  Each MessageFacility (like "log") has a name that usually corresponds to the software component for which this message
 *  facility serves as the logger.  The name could be the name of a program, file, namespace, class, method, function, etc. and
 *  will (by default) appear in the output. The constructor for a MessageFacility also takes an optional MessageSink object
 *  that describes the messages' ultimate destination; the default is Sawyer::merr, which sends messages to standard error.
 *
 * @code
 *  MessageFacility log("Sawyer::Example")
 * @endcode
 *
 *  Any combination of MessageFacility objects can be grouped together into one or more MessageFacilities (note plural) objects
 *  so they can be enabled and disabled collectively. A MessageFacilities object is able to parse a simple language that can be
 *  provided from the application's command-line, a configuration file, user input, hard-coded, or some other source. Sawyer is
 *  not itself a command-line switch parser--it only parses the strings which are provided to it.
 *
 * @code
 *  MessageFacilities facilities;
 *  facilities.insert(log);
 *  facilities.control("Sawyer::Example(fatal,error,warn)"); //FIXME: language is TBD
 * @endcode
 *
 * @section detail Details
 *
 * Sawyer is pronounced like SOY-er.  A sawyer is one who saws wood, like a logger (the lumberjack kind).  */
namespace Sawyer {

/** Level of importance for a message. Higher values are generally of more importance than lower values. */
enum MessageImportance {
    DEBUG,              /**< Messages intended to be useful primarily to the author of the code. This level of importance is
                         *   not often present in publically-released source code and serves mostly as an alternative for
                         *   authors that like to debug-by-print. */
    TRACE,             /**< Detailed tracing information useful to end-users that are trying to understand program
                         *   internals. These messages can also be thought of as debug messages that are useful to end
                         *   users. Tracing occurs in two levels, where TRACE is the low-level tracing that includes all
                         *   traceable messages. It can be assumed that if TRACE messages are enabled then WHERE messages are
                         *   also enabled to provide the broader context. */
    WHERE,             /**< Granular tracing information useful to end-users that are trying to understand program internals.
                         *   These can also be thought of as debug messages that are useful to end users.  Tracing occurs in
                         *   two levels, where WHERE provides a more granular overview of the trace. */
    INFO,               /**< Informative messages. These messages confer information that might be important but do not
                         *   indicate situations that are abnormal. */
    WARN,               /**< Warning messages that indicate an unusual situation from which the program was able to fully
                         *   recover. */
    ERROR,              /**< Error messages that indicate an abnormal situation from which the program was able to at least
                         *   partially recover. */
    FATAL,              /**< Messages that indicate an abnormal situation from which the program was unable to
                         *   recover. Producing a message of this type does not in itself terminate the program--it merely
                         *   indicates that the program is about to terminate. Since one software component's fatal error might
                         *   be a calling components's recoverable error, exceptions should generally be thrown.  About the
                         *   only time FATAL is used is when a logic assertion is about to fail, and even then it's usually
                         *   called from inside assert-like functions. */
    N_LEVELS            /**< Number of distinct importance levels. */
};

/** Colors used by sinks that support color. */
enum MessageColor {
    COLOR_BLACK         = 0,    // the values are important: they are the ANSI foreground and background color offsets
    COLOR_RED           = 1,
    COLOR_GREEN         = 2,
    COLOR_YELLOW        = 3,
    COLOR_BLUE          = 4,
    COLOR_MAGENTA       = 5,
    COLOR_CYAN          = 6,
    COLOR_WHITE         = 7,
    COLOR_DEFAULT       = 8
};

/** Convert a message importance to a string. */
std::string stringifyMessageImportance(MessageImportance imp);

typedef unsigned MessageId;
static const MessageId NO_MESSAGE = (unsigned)-1;

// Used internally
struct MessageProps {
    MessageProps()
        : indentation(0), interrupt_string("...\n"), terminate_string("\n"), destroy_string("\n"),
          use_color(false), fgcolor(COLOR_DEFAULT), bgcolor(COLOR_DEFAULT), color_attr(0) {}
    bool has_color() const { return fgcolor!=COLOR_DEFAULT || bgcolor!=COLOR_DEFAULT || color_attr!=0; }
    void set_color(MessageImportance);  // set color properties based on importance
    size_t indentation;                 // indentation level
    std::string interrupt_string;       // string to print at the end of an interrupted partial message
    std::string terminate_string;       // string to print for end of message
    std::string destroy_string;         // text to print to partial message when its stream is destroyed
    bool use_color;                     // use the following ANSI colors?
    MessageColor fgcolor;               // foreground color
    MessageColor bgcolor;               // background color
    int color_attr;                     // zero, or ANSI color attribute (bold, reverse, etc).
};

class Message {
    MessageId id_;                      // unique message ID
    MessageProps props_;                //
    std::string prefix_;                // message prefix string
    std::string body_;                  // message body (no linefeeds)
    bool complete_;                     // message has been completed (a linefeed has been inserted, but is not in body_)
    size_t body_nprinted_;              // number of characters of the message body that have been emitted by the sink
    static MessageId next_id_;
public:
    Message()
        : id_(next_id_++), complete_(false), body_nprinted_(0) {}

    explicit Message(const MessageProps &mprops)
        : id_(next_id_++), props_(mprops), complete_(false), body_nprinted_(0) {}

    void reset(const MessageProps &p) { // new message created in place
        id_ = next_id_++;
        props_ = p;
        prefix_ = body_ = "";
        complete_ = false;
        body_nprinted_ = 0;
    }

    void transfer_from(Message &other) {
        id_ = other.id_;                        other.id_ = next_id_++;
        props_ = other.props_;
        prefix_ = other.prefix_;
        body_ = other.body_;                    other.body_ = "";
        complete_ = other.complete_;            other.complete_ = false;
        body_nprinted_ = other.body_nprinted_;  other.body_nprinted_ = 0;
    }
    
private: // messages are not copyable
    Message& operator=(const Message&) { abort(); }
    Message(const Message&) { abort(); }

public:
    /** Unique message ID. */
    MessageId id() const { return id_; }

    /** Returns true if the message has an empty body. */
    bool empty() const { return body_.empty(); }

    /** Message properties.
     * @{ */
    const MessageProps& props() const { return props_; }
    MessageProps& props() { return props_; }
    /** @} */

    /** Returns true if part of this message has not been emitted yet. */
    bool pending() const { return body_nprinted_ < body_.size(); }

    /** Number of characters of the body that have been emitted already.
     * @{ */
    size_t nprinted() const { return body_nprinted_; }
    void nprinted(size_t n) { body_nprinted_ = n; }
    /** @} */

    /** Marke entire body as having been emitted. */
    void printed() { body_nprinted_ = body_.size(); }

    /** Messge prefix.
     * @{ */
    const std::string& prefix() const { return prefix_; }
    void prefix(const std::string &s) { prefix_ = s; }
    /** @} */

    /** Message body. */
    const std::string& body() const { return body_; }

    /** Append to message body.
     * @{ */
    void append(char c);
    void append(const std::string &s);
    /** @} */

    /** Returns true if a message body is partial.  Empty messages are partial messages. */
    bool is_partial() const { return !complete_; }

    /** Returns true if a message is complete. Empty messages are not complete. */
    bool is_complete() const { return complete_; }
};

/*******************************************************************************************************************************
 *                                      Message prefixes
 *******************************************************************************************************************************/

/** Create a message prefix.
 *
 *  Messages look best when they all have a similar prefix that describes from whence they came.  The MessagePrefix is a base
 *  class for user-defined prefixes, and provides an implementation that displays the base name of the program, the process ID,
 *  the high-resolution time since the program started, the name of the facility that generated the message, and the message
 *  importance level.  The time is the number of seconds that have elapsed since Sawyer was initialized (initialization happens
 *  automatically before main() is executed).
 *
 *  Example output:
 * @code
 *  a.out[16216] 0.00004s Disassembler[INFO ]: this is a sample message from running /tmp/saywer/a.out
 * @endcode
 */
class MessagePrefix {
protected:
    static struct timeval epoch_;       // approximate time at which executable started
    static std::string exename_;        // base name of executable
public:
    MessagePrefix() { init(); }
    virtual ~MessagePrefix() {}

    /** Produce a message prefix. */
    virtual std::string operator()(const MessageProps&, const std::string &facility_name, MessageImportance);
private:
    void init();
};

/** A message prefix class that produces empty prefixes. */
class MessageEmptyPrefix: public MessagePrefix {
public:
    virtual std::string operator()(const MessageProps&, const std::string &facility_name, MessageImportance) /*override*/ {
        return "";
    }
};
    
/** Default message prefix. See MessagePrefix for details. */
extern MessagePrefix default_prefix;

/** Empty message prefix. */
extern MessageEmptyPrefix empty_prefix;


/*******************************************************************************************************************************
 *                                      Message sinks
 *******************************************************************************************************************************/

/** Base class for message destinations.
 *
 *  A MessageSink is a place where messages go to be emitted. A message sink controls all aspects of message output and is
 *  responsible for combining messages from various MessageStream objects to present them in a cohesive manner (continuation,
 *  indentation, buffering, etc.). */
class MessageSink {
protected:
    size_t indentation_;                // Indentation level for new messages
    MessageId prev_mid_;                // ID of previously emitted partial message, or NO_MESSAGE
    MessageProps prev_mprops_;          // Properties of previous partial message if prev_mid_!=NO_MESSAGE
public:
    MessageSink(): indentation_(0), prev_mid_(NO_MESSAGE) {}
    virtual ~MessageSink() {}

    /** Emit (part of) a message. */
    virtual void emit(Message&) = 0;

    /** Indentation level.
     * @{ */
    size_t indentation() const { return indentation_; }
    void indentation(size_t level) { indentation_=level; }
    /** @} */

    /** Update partial message properties if necessary. The update occurs only if the sink has just emitted (via emit()) a
     *  partial message with the same message ID as that specified here. */
    void save_mprops(MessageId mid, const MessageProps &mprops) {
        if (prev_mid_==mid)
            prev_mprops_ = mprops;
    }

    /** Adjust the default message properties. For instance, a sink that supports color output might initialize the default
     * colors. This method is called shortly after the sink is associated with a MessageStream. */
    virtual void adjust_mprops(MessageProps &adjustable) {};
};



/** Discards all messages.  This isn't usually used since messages are emitted or not based on whether MessageStream used to
 * construct the message is enabled or disabled. */
class MessageNullSink: public MessageSink {
public:
    virtual void emit(Message&) /*override*/ {}
};



/** Sends messages to an STL std::ostream. */
class MessageFileSink: public MessageSink {
    std::ostream &ostream_;             // STL stream where output goes
    bool need_linefeed_;                // was the last character of output a linefeed?
    bool use_color_;                    // has color capability using ANSI escape sequences?
    static const size_t indent_width_ = 2;
public:
    explicit MessageFileSink(std::ostream &ostream, bool use_color=false)
        : ostream_(ostream), need_linefeed_(false), use_color_(use_color) {}
    virtual void emit(Message&) /*override*/;
    virtual void adjust_mprops(MessageProps&) /*override*/;

    /** Whether color output is enabled.
     * @{ */
    bool use_color() const { return use_color_; }
    void use_color(bool b) { use_color_=b; }
    /** @} */
};

/** Sink that sends messages to standard error. */
extern MessageFileSink merr;

/** Sink that always discards its output. */
extern MessageNullSink mnull;



/*******************************************************************************************************************************
 *                                      Stream
 *******************************************************************************************************************************/

class MessageStream;

// Only used internally
class MessageStreamBuf: public std::streambuf {
    friend class MessageStream;
    std::string name_;                  // name of the software facility
    MessageImportance importance_;      // importance for all messages generated here
    MessagePrefix *prefix_;             // format the prefix for each message
    MessageSink *sink_;                 // where these messages go
    bool enabled_;                      // should messages be emitted?
    Message mesg_;                      // current message
    MessageProps dflt_mprops_;          // default properties to initialize current message properties
    bool initialized_;                  // false until init() has been called

protected:
    MessageStreamBuf(): importance_(INFO), prefix_(NULL), sink_(NULL), enabled_(false), initialized_(false) {}

    // The constructor is not allowed to do much because it might be called for global variables before the MessagePrefix
    // and MessageSink have had a chance to be constructed.
    MessageStreamBuf(const std::string name, MessageImportance imp, MessagePrefix *prefix, MessageSink *sink)
        : name_(name), importance_(imp), prefix_(prefix), sink_(sink), enabled_(true), initialized_(false) {}

    // Copies an existing stream without copying that stream's partial message (if any)
    MessageStreamBuf(const MessageStreamBuf &other)
        : name_(other.name_), importance_(other.importance_), prefix_(other.prefix_), sink_(other.sink_),
          enabled_(other.enabled_), dflt_mprops_(other.dflt_mprops_), initialized_(other.initialized_) {
        if (initialized_)
            mesg_.reset(dflt_mprops_);
    }

    // Copies an existing stream without copying that stream's partial message (if any)
    MessageStreamBuf& operator=(const MessageStreamBuf &other) {
        name_ = other.name_;
        importance_ = other.importance_;
        prefix_ = other.prefix_;
        sink_ = other.sink_;
        enabled_ = other.enabled_;
        dflt_mprops_ = other.dflt_mprops_;
        initialized_ = other.initialized_;
        if (initialized_)
            mesg_.reset(dflt_mprops_);
        return *this;
    }

    ~MessageStreamBuf() { finish(); }
    virtual std::streamsize xsputn(const char *s, std::streamsize &n) /*override*/;
    virtual int_type overflow(int_type c = traits_type::eof()) /*override*/;

private:
    // (Re)initialize the streambuf.
    void init();

    // Cancel any current partial message without attempting to undo anything that was emitted already
    void cancel() { mesg_.reset(dflt_mprops_); }

    // Move a partial message from another stream.
    void transfer_ownership(MessageStreamBuf &other);

    // Called during destruction to terminate partial message
    void finish();
};



/** Constructs, formats, and conditionally emits a stream of messages.  Although this is the type of object upon which most
 *  operations are performed, the user seldom explicitly instantiates a MessageStream--the preferred way to create streams is
 *  via the MessageFacility interface. */
class MessageStream: public std::ostream {
    MessageStreamBuf streambuf_;
public:
    /** Primary constructor.  Each stream serves a particular software component (program, namespace, class, etc.) and
     *  importance level and is born enabled. The @p prefix and @p sink objects are incorporated by reference and must not be
     *  destroyed until after this MessageStream is destroyed. */ 
    explicit MessageStream(const std::string facility_name, MessageImportance importance,
                           MessagePrefix &prefix=default_prefix, MessageSink &sink=merr)
        : std::ostream(&streambuf_), streambuf_(facility_name, importance, &prefix, &sink) {}

    /** Copy constructor. The new message stream is like the specified one, but does not have any outstanding partial message. */
    MessageStream(MessageStream &other): std::ostream(&streambuf_), streambuf_(other.streambuf_) {}

    /** Message ownership transfer.  The API supports the following style of partial message completion:
     * @code
     *  MessageStream m1(log[DEBUG] <<"partial message");
     *  log[DEBUG] <<"a different message\n";
     *  ...
     *  m1 <<" completed\n";
     * @endcode
     *
     * The insertion operation inside the @p m1 constructor argument list causes the "partial message" to be initially owned by
     * the log[DEBUG] stream.  The insertion operation returns an std::ostream static type which triggers this constructor, and
     * which transfers ownership of "partial message" to "m1" so it doesn't interfere with "a different message" and so that it
     * can be completed by inserting to @p m1. */
    MessageStream(std::ostream &s): std::ostream(&streambuf_) {
        MessageStream *other = dynamic_cast<MessageStream*>(&s);
        assert(other!=NULL);
        streambuf_ = other->streambuf_; // doesn't copy partial message
        streambuf_.transfer_ownership(other->streambuf_);
    }

    /** Returns true if this stream is emitting messages. */
    bool enabled() const { return streambuf_.enabled_; }

    /** Returns true if this stream is enabled.  This implicit conversion to bool can be used to conveniently avoid expensive
     *  insertion operations when a stream is disabled.  For example, if printing MemoryMap is an expensive operation then the
     *  logging can be written like this to avoid formatting memorymap when logging is disabled:
     * @code
     *  log[DEBUG] and log[DEBUG] <<"the memory map is:" <<memorymap <<"\n";
     * @endcode
     *
     * In fact, the SAWYER macro does exactly this and can be used instead:
     * @code
     *  SAWYER(log[DEBUG]) <<"the memory map is: " <<memorymap <<"\n";
     * @endcode
     */
    operator bool() {return enabled(); }

    /** See MessageStream::bool() */
    #define SAWYER(message_stream) message_stream and message_stream

    /** Enable or disable a stream.  A disabled stream buffers the latest partial message and enabling the stream will cause
     * the entire accumulated message to be emitted--whether the partial message immediately appears on the output is up to
     * the message sink (MessageFileSink output is immediate).
     * @{ */
    void enable(bool b=true) { streambuf_.enabled_ = b; }
    void disable() { enable(false); }
    /** @} */

    /** Indentation level.  A stream that is indented inserts additional white space between the message prefix and the message
     *  text.  Although indentation is adjusted on a per-stream basis and the stream is where the indentation is applied, the
     *  actual indentation level is a property of the MessageSink. Therefore, all messages streams using the same sink are
     *  affected.
     *
     *  Users seldom use this method to change the indentation since it requires careful programming to restore the old
     *  indentation in the presence of non-local exits from scopes where the indentation was indented to be in effect (e.g.,
     *  exceptions). A more useful interface is the MessageIndenter class.
     * @{ */
    size_t indentation() const { return streambuf_.sink_->indentation(); }
    void indentation(size_t level) { streambuf_.sink_->indentation(level); }
    /** @} */

    /** Partial message interrupt string.  This string is printed when a partial message is interrupted by some other
     *  message and should include any necessary line termination. The default is "...\n". */
    void interrupt_string(const std::string &s, bool as_dflt=true) {
        streambuf_.mesg_.props().interrupt_string = s;
        streambuf_.sink_->save_mprops(streambuf_.mesg_.id(), streambuf_.mesg_.props());
        if (as_dflt)
            streambuf_.dflt_mprops_.interrupt_string = s;
    }

    /** Message termination string.  This string is printed when a message is finished. The default is "\n". */
    void terminate_string(const std::string &s, bool as_dflt=true) {
        streambuf_.mesg_.props().terminate_string = s;
        streambuf_.sink_->save_mprops(streambuf_.mesg_.id(), streambuf_.mesg_.props());
        if (as_dflt)
            streambuf_.dflt_mprops_.terminate_string = s;
    }

    /** How to finish up a partial message when this message stream is destroyed. Should include message termination. */
    void destroy_string(const std::string &s, bool as_dflt=true) {
        streambuf_.mesg_.props().destroy_string = s;
        streambuf_.sink_->save_mprops(streambuf_.mesg_.id(), streambuf_.mesg_.props());
        if (as_dflt)
            streambuf_.dflt_mprops_.destroy_string = s;
    }

    /** Returns a pointer to the message sink. */
    MessageSink *sink() const { return streambuf_.sink_; }
};



/*******************************************************************************************************************************
 *                                      Manipulators
 *******************************************************************************************************************************/

/** Manipulator to change the partial message interrupt string.  Changes the string that will be printed for the current
 *  message if it is interrupted by another message.  The setting persists until the end of the current message, at which time
 *  the default is restored.   The default can be changed with MessageStream::interrupt_string(). */
struct pint {
    explicit pint(const std::string &s): s(s) {}
    std::string s;
};

std::ostream& operator<<(std::ostream&, const pint&);

/** Manipulator to change the partial message destruction string. Changes the string that will be printed for the current
 *  message if its message stream is destroyed without completing the message. The setting persists until the end of the
 *  current message, at which time the default is restored.  The default can be changed with MessageStream::cleanup_string(). */
struct pdest {
    explicit pdest(const std::string &s): s(s) {}
    std::string s;
};

std::ostream& operator<<(std::ostream&, const pdest&);
    


/*******************************************************************************************************************************
 *                                      Utility classes
 *******************************************************************************************************************************/

/** Indent messages for the duration of the enclosing scope.  All messages emitted to the same MessageSink are subsequently
 *  indented one additional level. The increment remains in effect until this object is canceled or destroyed. Using a message
 *  indenter is more convenient that using the MessageStream::indentation() method because there is no need to manually restore
 *  the previous indentation in the presence of non-local exits from the scope (return, break, throw goto, etc).
 *
 *  Example usage:
 * @code
 *  double f1(double x, double y) {
 *      MessageStream m1(log[TRACE] <<"f1(x=" <<x <<", y=" <<y <<") =");
 *      MessageIndenter indent(m1);             // increase indentation for all streams using same sink
 *      double retval = f2(x,y) + f2(x,y);      // messages emitted here will be indented
 *      indent.cancel() <<" " <<retval <<"\n";  // explicitly cancel indentation and complete the message
 *      return retval;
 *  }
 * @endcode
 */
class MessageIndenter {
    MessageStream *stream_;
    size_t old_level_;
    bool canceled_;
public:

    /** Increase indentation until canceled or destroyed.  See class documentation for details. */
    explicit MessageIndenter(std::ostream &s): canceled_(false) {
        stream_ = dynamic_cast<MessageStream*>(&s);
        assert(stream_!=NULL);
        old_level_ = stream_->indentation();
        stream_->indentation(old_level_+1);
    }

    ~MessageIndenter() {
        cancel();
    }

    /** Explicitly cancels indentation. The destructor will have no further effect. The return value is conveniently the message
     *  stream used in the constructor so that cancelation and message completion can be performed with a single C++
     *  statement. See class documentation for details. */
    MessageStream& cancel() {
        if (!canceled_) {
            stream_->indentation(old_level_);
            canceled_ = true;
        }
        return *stream_;
    }
};

/** Temporarily enable or disable a stream.  The indicated stream is enabled or disabled for the duration of the enclosing
 *  scope. Using this class is more convenient than explicitly calling MessageStream::enable() because this class automatically
 *  handles all exits from the enclosing scope, including exceptions.
 *
 *  Example usage: temporarily turning on debugging within a single function:
 * @code
 *  int series_sum(int from, int to) {
 *      TemporaryEnable te(log[DEBUG]);
 *      log[DEBUG] <<"debugging series_sum(from=" <<from <<", to=" <<to <<")\n");
 *      int sum = 0;
 *      for (int i=from; i<=to; ++i) {
 *          sum += i;
 *          log[DEBUG] <<"  i=" <<i <<" sum=" <<sum <<"\n";
 *      }
 *      return i;
 *  }
 * @endcode
 *
 * Adding the declaration for @p te temporarily enables the log[DEBUG] stream until after the return statement, at which time
 * its original enabled or disabled state is restored.
 */
class TemporaryEnable {
    MessageStream *stream_;
    bool old_state_;
    bool canceled_;
public:

    /** Temporarily enable or disable a stream. See class documentation for details. */
    explicit TemporaryEnable(std::ostream &s, bool enabled=true): canceled_(false) {
        stream_ = dynamic_cast<MessageStream*>(&s);
        assert(stream_!=NULL);
        old_state_ = stream_->enabled();
        stream_->enable(enabled);
    }

    ~TemporaryEnable() {
        cancel();
    }

    /** Explicitly cancels temporary setting. The stream used in the constructor is restored to the enabled or disabled state
     * that it had when the constructor was called, and the destructor will have no further effect. */
    void cancel() {
        if (!canceled_) {
            stream_->enable(old_state_);
            canceled_ = true;
        }
    }
};



/*******************************************************************************************************************************
 *                                      Facility
 *******************************************************************************************************************************/

/** Collection of streams. This forms a collection of message streams for a software component and contains one stream per
 *  message importance level.  A MessageFacility is intended to be used by a software component at whatever granularity is
 *  desired by the author (program, name space, class, method, etc.) and is usually given a string name that is related to the
 *  software component which it serves.  The string name becomes part of messages and is also the default name used by
 *  Facilities::control().  All MessageStream objects created for the facility are given the same name, message prefix
 *  generator, and message sink (the sink can be adjusted later).
 *
 *  The C++ name for the facility is often just "log" (appropriately scoped) so that code to emit messages is self
 *  documenting:
 * @code
 *  log[ERROR] <<"I got an error\n";
 * @endcode
 *
 *  When a MessageFacility is intended to serve an entire program, the name of the facility is redundant with the program name
 *  printed by the default message prefix.  The best way to remedy this is to supply an empty name for the facility, and if the
 *  facility is registered with a MessageFacilities object to supply the program name when it is registered:
 *
 * @code
 *  #include <Sawyer.h>
 *  using namespace Sawyer;
 *  MessageFacility log("");
 *  MessageFacilities logging;
 *
 *  int main(int argc, char *argv[]) {
 *      std::string progname = get_program_name(argv[0]);
 *      logging.insert(log, progname);
 *      ...
 * @endcode
 *
 * FIXME: get the program name from default_prefix.progname() */
class MessageFacility {
    std::string name_;
    MessageStream *streams_[N_LEVELS];
public:
    /** Creates streams of all importance levels.  The prefix and sink are incorporated by reference and should not be
     *  destroyed until after this facility is destroyed. See class documentation for details. */
    MessageFacility(const std::string &name, MessagePrefix &prefix=default_prefix, MessageSink &sink=merr) {
        init(name, prefix, sink);
    }

    ~MessageFacility() {
        for (int i=0; i<N_LEVELS; ++i)
            delete streams_[i];
    }

    /** Return the stream for the specified importance level.
     * @{ */
    MessageStream& get(MessageImportance imp) const {
        assert(imp>=0 && imp<N_LEVELS);
        return *streams_[imp];
    }
    MessageStream& operator[](MessageImportance imp) const {
        return get(imp);
    }
    /** @} */

    /** Return the name of the facility. This is a read-only field initialized at construction time. */
    const std::string name() const { return name_; }

private:
    void init(const std::string &name, MessagePrefix&, MessageSink&);
};

// Default library wide message facility
extern MessageFacility log;


/*******************************************************************************************************************************
 *                                      Facilities
 *******************************************************************************************************************************/

/** Collection of facilities. Allows all facilities to be configured collectively, such as from command-line parsing. */
class MessageFacilities {
    typedef std::map<std::string, MessageFacility*> FacilityMap;
    typedef std::set<MessageImportance> ImportanceSet;
    FacilityMap facilities_;
    ImportanceSet impset_;
public:
    /** Register a facility so it can be controlled as part of a collection of facilities.  The optional @p name is the
     *  FACILITY_TERM string of the control language for the inserted @p facility, and defaults to the facility's name.  The
     *  name consists of one or more symbols separated by '::', or '.' and often corresponds to the source code component that
     *  it serves.  Names are case-sensitive in the control language.  No two facilities in the same MessageFacilities object
     *  can have the same name, but a single facility may appear multiple times with different names.
     *
     *  The @p facility is incorporated by reference and should not be destroyed until after this MessageFacilities object is
     *  destroyed. */
    void insert(MessageFacility &facility, std::string name="");

    /** Remove a facility by name. */
    void erase(const std::string &name) {
        facilities_.erase(name);
    }

    /** Remove all occurrences of the facility. */
    void erase(MessageFacility &facility);

    /** Parse a single command-line switch and enable/disable the indicated streams.  Returns an empty string on success, or an
     *  error message on failure.  No configuration changes are made if a failure occurs.
     *
     *  The control string, @p s, is a sentence in the following language:
     *
     * @code
     *  Sentence             := (FacilitiesControl ( ',' FacilitiesControl )* )?
     *
     *  FacilitiesControl    := AllFacilitiesControl | NamedFacilityControl
     *  AllFacilitiesControl := StreamControlList
     *  NamedFacilityControl := FACILITY_NAME '(' StreamControlList ')'
     *
     *  StreamControlList    := StreamControl ( ',' StreamControl )*
     *  StreamControl        := StreamState? Relation? STREAM_NAME | 'all' | 'none'
     *  StreamState          := '+' | '!'
     *  Relation             := '>=' | '>' | '<=' | '<'
     * @endcode
     *
     *  The language is processed left-to-right and stream states are affected immediately.  This allows, for instance, turning
     *  off all streams and then selectively enabling streams.
     *
     *  Some examples:
     * @code
     *  all                             // enable all event messages
     *  none,debug                      // turn on only debugging messages
     *  all,!debug                      // turn on all except debugging
     *  fatal                           // make sure fatal errors are enabled (without changing others)
     *  none,<=trace                    // disable all except those less than or equal to 'trace'
     *
     *  frontend(debug)                 // enable debugging streams in the 'frontend' facility
     *  warn,frontend(!warn)            // turn on warning messages everywhere except the frontend
     * @endcode
     *
     *  The global list (AllFacilitiesControl) also affects the list of default-enabled importance levels.
     */
    std::string control(const std::string &s);

    /** Enable/disable specific importance level across all facilities.  This method also affects the set of "current
     *  importance levels" used when enabling an entire facility.
     * @{ */
    void enable(MessageImportance, bool b=true);
    void disable(MessageImportance imp) { enable(imp, false); }
    /** @} */

    /** Enable/disable a facility by name. When disabling, all importance levels of the specified facility are disabled. When
     *  enabling, only the current importance levels are enabled for the facility.  If the facility is not found then nothing
     *  happens.
     * @{ */
    void disable(const std::string &switch_name) { enable(switch_name, false); }
    void enable(const std::string &switch_name, bool b=true);
    /** @} */

    /** Enable/disable all facilities.  When disabling, all importance levels of all facilities are disabled.  When enabling,
     *  only the current importance levels are enabled for each facility. */
    void enable(bool b=true);
    void disable() { enable(false); }

    /** Print the list of facilities and their states. This is mostly for debugging purposes. The output may have internal
     *  line feeds and will end with a line feed. */
    void print(std::ostream&) const;

private:
    // Private info used by control() to indicate what should be adjusted.
    struct ControlTerm {
        ControlTerm(const std::string &facility_name, bool enable)
            : facility_name(facility_name), lo(DEBUG), hi(DEBUG), enable(enable) {}
        std::string to_string() const;      // string representation of this struct for debugging
        std::string facility_name;          // optional facility name; empty implies all facilities
        MessageImportance lo, hi;           // inclusive range of importances
        bool enable;                        // new state
    };

    // Error information thrown internally
    struct ControlError {
        ControlError(const std::string &mesg, const char *position): mesg(mesg), input_position(position) {}
        std::string mesg;
        const char *input_position;
    };

    // Functions used by the control() method
    static std::string parse_facility_name(const char* &input);
    static std::string parse_enablement(const char* &input);
    static std::string parse_relation(const char* &input);
    static std::string parse_importance_name(const char* &input);
    static MessageImportance importance_from_string(const std::string&);
    static std::list<ControlTerm> parse_importance_list(const std::string &facility_name, const char* &input);

};

} // namespace

#endif

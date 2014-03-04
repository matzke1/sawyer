#ifndef Sawyer_H
#define Sawyer_H

/** @mainpage
 *
 *  Sawyer is a library that provides the following:
 *
 *  @li Conditionally enable streams for program diagnostics.  These are C++ std::ostreams organized by software component
 *      and message importance and which can be enabled/disabled with a simple language. A complete plumbing system similar to
 *      Unix file I/O redirection but more flexible can do things like multiplexing messages to multiple locations (e.g.,
 *      stderr and syslogd), rate-limiting, colorizing on ANSI terminals, and so on. See Sawyer::Message for details.
 *
 *  @li Logic assertions in the same vein as \<cassert> but using the diagnostic streams and multi-line output to make them
 *      more readable.  See Sawyer::Assert for details.
 *
 *  @li Progress bars for text-based output using the diagnostic streams so that progress bars interact sanely with other
 *      diagnostic output.  See Sawyer::ProgressBar for details.
 *
 *  @li Command-line parsing to convert program switches and their arguments into C++ objects with the goal of being able to
 *      support nearly any command-line style in use, including esoteric command switches from programs like tar and
 *      dd. Additionally, documentation can be provided in the C++ source code for each switch and Unix man pages can be
 *      produced. See Sawyer::CommandLine for details.
 *
 *  Other things on the way but not yet ready:
 *
 *  @li A simple, extensible, terse markup language that lets users write documentation that can be turned into TROFF, HTML,
 *      Doxygen, PerlDoc, TeX, etc. The markup language supports calling of C++ functors to transform the text as it is
 *      processed.
 *
 *  <b>Good starting places for reading documentation are the namespaces.</b> */

/** Name space for the entire library.  All Sawyer functionality except for some C preprocessor macros exists inside this
 * namespace.  Most of the macros begin with the string "SAWYER_". */
namespace Sawyer {

/** Explicitly initialize the library. This initializes any global objects provided by the library to users.  This happens
 *  automatically for many API calls, but sometimes needs to be called explicitly. Calling this after the library has already
 *  been initialized does nothing. The function always returns true. */
bool initializeLibrary();

/** True if the library has been initialized. */
extern bool isInitialized;

} // namespace

#endif

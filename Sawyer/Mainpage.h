/** @mainpage
 *
 *  %Sawyer is a library that provides the following:
 *
 *  @li Conditionally enable streams for program diagnostics.  These are C++ <code>std::ostreams</code> organized by software
 *      component and message importance and which can be enabled/disabled with a simple language. A complete plumbing system
 *      similar to Unix file I/O redirection but more flexible can do things like multiplexing messages to multiple locations
 *      (e.g., stderr and syslogd), rate-limiting, colorizing on ANSI terminals, and so on. See Sawyer::Message for details.
 *
 *  @li Logic assertions in the same vein as <tt>\<cassert></tt> but using the diagnostic streams and multi-line output to make
 *      them more readable.  See Sawyer::Assert for details.
 *
 *  @li Progress bars for text-based output using the diagnostic streams so that progress bars interact sanely with other
 *      diagnostic output.  See Sawyer::ProgressBar for details.
 *
 *  @li Command-line parsing to convert program switches and their arguments into C++ objects with the goal of being able to
 *      support nearly any command-line style in use, including esoteric command switches from programs like tar and
 *      dd. Additionally, documentation can be provided in the C++ source code for each switch and Unix man pages can be
 *      produced. See Sawyer::CommandLine for details.
 *
 *  @li Container classes: @ref Sawyer::Container::Graph "Graph", storing vertex and edge connectivity information along with
 *      user-defined values attached to each vertex and edge, sequential ID numbers, and constant time complexity for most
 *      operations; @ref Sawyer::Container::IndexedList "IndexedList", a combination list and vector having constant time
 *      insertion and erasure and constant time lookup-by-ID; @ref Sawyer::Container::Interval "Interval" represents integral
 *      intervals including empty and whole intervals; @ref Sawyer::Container::IntervalSet "IntervalSet" and @ref
 *      Sawyer::Container::IntervalMap "IntervalMap" similar to STL's <code>std::set</code> and <code>std::map</code>
 *      containers but optimized for cases when very large numbers of keys are adjacent; @ref Sawyer::Container::Map "Map",
 *      similar to STL's <code>std::map</code> but with an API that's consistent with other containers in this library; @ref
 *      Sawyer::Container::BitVector "BitVector" bit vectors with operations defined across index intervals.  These can be
 *      found in the Sawyer::Container namespace.
 *
 *   @li Miscellaneous: @ref Sawyer::SynchronizedPoolAllocator "PoolAllocator" (synchronized and unsynchronized variants) to
 *       allocate memory from large pools rather than one object at a time; and @ref Sawyer::SmallObject "SmallObject", a base
 *       class for objects that are only a few bytes; @ref Sawyer::Stopwatch "Stopwatch" for high-resolution elapsed time; @ref
 *       Sawyer::Optional "Optional" for optional values.
 *
 *  Design goals for this library can be found in the [Design goals](group__design__goals.html) page.
 *
 *  Installation instructions can be found on the [Installation](group__installation.html) page.
 *
 *  Other things on the way but not yet ready:
 *
 *  @li A simple, extensible, terse markup language that lets users write documentation that can be turned into TROFF, HTML,
 *      Doxygen, PerlDoc, TeX, etc. The markup language supports calling of C++ functors to transform the text as it is
 *      processed.
 *
 *  <b>Good starting places for reading documentation are the [namespaces](namespaces.html).</b> */

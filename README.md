Sawyer
======

Sawyer is a library that provides functionality that's often needed
when writing other C++ libraries and tools.  It's main components are:

+ Streams for conditionally emitting diagnostics, including logic
  assertions and text-based progress bars

+ Parsing of program command lines.

Design Goals
============

These design goals are used throughout Sawyer:

+ The API should be well documented and tested.  Every public symbol
  in Sawyer is documented with doxygen and includes all pertinent
  information about what it does, how it relates to other parts of
  Sawyer, restrictions and caveats, etc.  This kind of information is
  not evident from the C++ interface itself and is often omitted in
  other libraries' documentation.

+ The library should be easy to use. Common things should be simple
  and terse, but less common things should still be possible without
  significant work. Users should not have to worry about who owns what
  objects--Sawyer uses reference counting pointers to control
  deallocation.  The library should use a consistent naming scheme. It
  should avoid extensive use of templates since they're a common cause
  of diffulty for beginners.

+ The library should be familiar to experienced C++
  programmers. It should use the facilities of the C++ language, C++
  idioms, and an object oriented design.  The API should not look like
  a C library being used in a C++ program, and the library should not
  be concerned about being used in languages that are not object
  oriented.

+ The library should be safe to use.  Errors should be handled in
  clear, recoverable, and predictable ways. The library should make
  every attempt to avoid situations where the user can cause a
  non-recoverable fault due to misunderstanding the API.

+ The library should be highly configurable, but use defaults that
  configure it for the most common cases.  Common things should be
  simple, but less common things should not require exceptionally
  advanced coding.

+ Functionality is more important than efficiency. Every attempt is
  made to have an efficient implementation, but when there is a choice
  between functionality and efficiencey, functionality wins.

Emitting Diagnostics
====================

Example
-------

      using namespace Sawyer::Message;
      mlog[INFO] <<"processing " <<filename <<"...\n";

Purpose
-------

Most programs and libraries emit diagnostics by writing to std::cerr,
and enclosing that code in `if` statements of `#ifdef` directives and
a variety of ways to control what gets emitted (global variables,
command-line switches, GNU `configure` or `cmake` switches, etc). The
Sawyer::Message namespace provides streams that can be individually or
collectively enabled and grouped according to software components.  It
defines a simple language for configuring diagnostics, and the
output is consistent.

Features
--------

+ *Familiar API:* Uses C++ paradigms for printing messages, namely the
  STL `std::ostream` insertion operators, manipulators, etc.

+ *Ease of use:* All STL `std::ostream` insertion operators (`<<`) and
  manipulators work as expected, so no need to write any new functions
  for logging complex objects.

+ *Type safety:* Sawyer avoids the use of C `printf`-like varargs
  functions.

+ *Consistent output:* Messages have a consistent look, which by
  default includes the name of the program, process ID, software
  component, importance level, and optional color. The look of this
  prefix area can be controlled by the programmer.

+ *Multiple backends:* Sawyer supports multiple backends which can
  be extended by the programmer. Possibilities include colored output
  to the terminal, output to a structured file, output to a database,
  etc. (Currently only text-based backends are implemented. [2013-11-01])

+ *Partial messages:* Sawyer is able to emit messages incrementally
  as the data becomes available.  This is akin to C++ `std::cerr`
  output.

+ *Readable output:* Sawyer output is readable even when multiple
  messages are being created incrementally.  Colored text can be used
  to visually distinguish between different importance levels (e.g.,
  errors versus warnings).

+ *Granular output:* Sawyer supports multiple messaging facilities
  at once so that each software component can be configured
  individually at whatever granularity is desired: program, file,
  namespace, class, method, function, whatever.

+ *Run-time controls:* Sawyer has a simple language for enabling and
  disabling collections of error streams to provide a consistent user
  API for things like command-line switches and configuration
  files. Using these controls is entirely optional.

+ *Efficient:* Sawyer message output can be written in such a way that
  the right hand size of `<<` operators is not evaluated when the
  message stream is in a disabled state.

Logic Assertions
================

Example
-------

        ASSERT_require2(diff>=0, "distance between points cannot be negative");
        ASSERT_not_null2(ptr, "the surface must be allocated");

Purpose
-------

Macros like `assert` those provided by <cassert> except more
descriptive and configurable.

Emitting Progress Bars
======================

Example
-------

        using namespace Sawyer::Message;
        size_t totalWork = 1000;
        Sawyer::ProgressBar progress(totalWork, mlog[INFO]);
        for (size_t i=0; i<totalWork; ++i, ++progress) {
            ....
        }

Purpose
-------

Emits continually updating progress bars as text, but in a way that
doesn't interfere with other diagnostic output.  Supports counting
progress bars (like the synopsis), arbitrary ranges, reverse progress
bars, progress with unknown limits, etc.

Command-line Parsing
====================

Example
-------

        using namespace Sawyer::CommandLine;

        bool verbose = false;

        Switch sw = Switch("verbose", 'v')
                    .doc("Make output verbose.")
                    .intrinsicValue("true", booleanParser(verbose));

        Parser().with(sw).parse().apply();

Purpose
-------

Command line parsing with the goals of being able to produce a
full-fledged Unix manual page and being able to parse almost any
command line with very little work using command-line paradigms that
are familiar on a variety of operating systems.

Features
--------

+ Clean distinction between declarations, parsing, and results.

+ Ability for a library to defer command-line parsing to the tool
  that's using the library.

+ Documentation next to the thing it documents.
  Automatically-generated documentation where that's possible. Control
  over the order of documentation sections, simple markup language so
  Unix man pages can be created without needing to learn ROFF
  formatting commands. Consistent output for help and version
  switches.

+ Long vs. short (single letter) switch names with distinct and
  configurable prefixes for each form, each switch, each switch group,
  and each parser and multiple long and/or short names for the same
  switch ("--gray" and "--grey").

+ Consistent handling of termination switches like "--"
  (configurable).

+ Single letter switches can be nestled together so "-a100b" is the
  same as "-a100 -b" (configurable).

+ Expansion of command-line switches by including switches from a
  file (configurable).

+ Immediate user-defined actions when a switch is parsed.

+ Configurable ways to handle multiple occurrences of a switch or
  related switches on the command line.

+ A switch can have any number of arguments, and those arguments can
  be separated from switches in configurable ways. Arguments can be
  optional with default values of any type.

+ Switch arguments parsers and argument value storage are largely
  distinct from each other. I.e., a switch can take a mathematical
  integer as an argument, and store that value in an 'unsigned
  short'. An error is raised if the conversion fails (and lack of
  error is one of the requirements for a valid switch syntax).

+ User defined parsers are supported and may return any copyable C++
  type.

+ Switch argument values can be automatically copied into
  user-supplied storage, and the copying only happens if parsing of
  the entire command-line is successful (configurable).  Scalar and
  vector storage is supported.

+ Related switches can share argument values. E.g., --verbose and
  --quiet can be set up so they produce a single result.

+ Switches whose argument is a list, like "-L/usr/lib32:/usr/lib64",
  and a mechanism for optionally exploding the list so it appears to
  have come from multiple switches like "-L/usr/lib32 -L/usr/lib64".

+ Parsing of C++ enum constants from the command-line directly into
  C++ variables.

+ Support for switches that have multiple sytaxes, like
  "--width=SIZE|narrow|wide".

+ Full introspection of switch declarations.

+ Complete information about where a parsed value came from in the
  program command-line.

+ Configurable error reporting by throwing exceptions, emitting
  messages with Sawyer::Message, or skipping over the parts of the
  command-line that couldn't be parsed.

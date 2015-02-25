Sawyer
======

Sawyer is a library that provides functionality that's often needed
when writing other C++ libraries and tools.  It's main components are:

+ Streams for conditionally emitting diagnostics, including logic
  assertions and text-based progress bars

+ Parsers for program command lines and automatically generating
  Unix manual pages.

+ Containers, including Graph, IndexedList, IntervalSet,
  IntervalMap, Map, and BitVector.

+ Miscellaneous: Memory pool allocators, small object support,
  stopwatch-like timers, optional values.

Documentation
=============

The Saywer user manual and API reference manual are combined in a
single document.  A version of the documentation can be found
[here](http://www.hoosierfocus.com/~matzke/sawyer), or users can run

    $ doxygen docs/doxygen.conf

and then browse to docs/html/index.html.

Installing
==========

Installation instructions are
[here](http://www.hoosierfocus.com/~matzke/sawyer/group__installation.html). In
summary:

    $ SAWYER_SRC=/my/sawyer/source/code
    $ git clone https://github.com/matzke1/sawyer $SAWYER_SRC
    $ SAWYER_BLD=/my/sawyer/build/directory
    $ mkdir $SAWYER_BLD
    $ cd $SAWYER_BLD
    $ cmake $SAWYER_SRC -DCMAKE_INSTALL_PREFIX:PATH="/some/directory"
    $ make
    $ make install

One commonly also needs
"-DBOOST_ROOT=/the/boost/installation/directory", and
"-DCMAKE_BUILD_TYPE=Debug" is useful during development.

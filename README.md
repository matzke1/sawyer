Sawyer
======

Sawyer is a library that provides functionality that's often needed
when writing other C++ libraries and tools.  It's main components are:

+ Streams for conditionally emitting diagnostics, including logic
  assertions and text-based progress bars

+ Parsers for program command lines and automatically generating
  Unix manual pages.

Documentation
=============

The Saywer user manual and API reference manual are combined in a
single document.  A version of the documentation can be found
[here](http://hoosierfocus.com/sawyer), or users can run

    $ doxygen docs/doxygen.conf

and then browse to docs/html/index.html.

Installing
==========

Run the CMake configuration in your compilation directroy:

    $ mkdir /path/for/your/compilation/directory
    $ cd /path/for/your/compilation/directory
    $ cmake "${SAWYER_SOURCE}" -DBOOST_ROOT="${BOOST_HOME}" -DCMAKE_INSTALL_PREFIX:PATH="/path/for/installation"

This will generate your build files, which may then be used to compile Sawyer:

    $ make

Finally, install Sawyer's shared library and header files:

    $ make install

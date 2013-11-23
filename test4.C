// Test MessageFacilities command-line switch parsing.
//
// Some things to try:
//   ./a.out
//              Should show what the defaults are.
//   ./a.out --log='none,debug'
//              Turns off all logging except for debugging.  You could do this in two steps
//              if you like: ./a.out --log=none --log=debug
//   ./a.out --log='main.third(debug)'
//              Turns on debugging for one facility without affecting anything else.
//   ./a.out --log='none, <=info'
//              Turns on levels INFO and below (after first disabling everything).
//   ./a.out --log='debug, main.third(!debug)'
//              Turns on DEBUG everywhere but in the main.third facility, where it's
//              explicitly turned off.  There's not an easy way to control a facility
//              everywhere without affecting it in some facilities.
//   ./a.out --log='debug, man.third(!debug)'
//              An error ("main.third" is misspelled). The output will be like this:
//                  ./a.out: no such message facility 'man.third'
//                      error occurred in "debug, man.third(!debug)"
//                      at this position   -------^
//              Furthermore, no changes are made to the configuration.

#include "Sawyer.h"

#include <iostream>
#include <string.h>

int main(int argc, char *argv[]) 
{
    using namespace Sawyer;

    // Instantiate some message facilities for these tests
    MessageFacility mf1("main::mf1");
    MessageFacility mf2("main.mf2");
    MessageFacility mf3("message facility 3");

    // Register the message facilities so they can be controlled as a group.  Note
    // that "message facility 3" is not a valid name for the control language, so we
    // need to provide a different name (if you don't, you'll get an std::logic_error
    // thrown to describe the problem).  Alternatively, the insert_and_adjust() method
    // can be used so that each inserted facility's streams are enabled/disabled based
    // on the settings of the log_facilities (we don't need to do that here because
    // we're about to adjust them all anyway).
    MessageFacilities log_facilities;
    log_facilities.insert(mf1);
    log_facilities.insert(mf2);
    log_facilities.insert(mf3, "main.third");

    // This is how you would set defaults before command-line parsing.  This particular
    // example turns off everything, and then enables messages at INFO level and higher.
    log_facilities.control("none, >=info");

    // Parse command lines.  This is one way to do it, but every program will probably have
    // its own mechanism.  By separating command-line processing from MessageFacility control
    // operations we enable messages to also be controlled from config files, strings, etc.
    // (like the statement above).
    for (int argno=1; argno<argc; ++argno) {
        if (0==strncmp(argv[argno], "--log=", 6)) {
            // Don't exit on errors -- we want to see what effect errors have on the state.
            // An error should not affect anything even if occurs late in the string. Also
            // note that error messages are reasonable and show where the error was
            // detected.  If you don't want to print the location info, just chop the
            // message at the first linefeed.  The control() method never prints errors
            // itself, thus giving the caller complete flexibility in how to handle things.
            std::string errmesg = log_facilities.control(argv[argno]+6);
            if (!errmesg.empty())
                std::cerr <<argv[0] <<": " <<errmesg <<"\n";
        } else {
            std::cerr <<"usage: " <<argv[0] <<" --log=LOG_SPECS\n";
            exit(1);
        }
    }

    // Show the results.  Since Sawyer::MessageStream is a subclass of std::ostream we
    // can print our results to it instead of to std::cout if we like.  We'll demonstrate
    // indentation while we're at it. The "log" is Sawyer::log, the default log facility. Since
    // it isn't part of our log_facilities, the command-line switches don't control it. We
    // could have registered it if we wanted to (you'd have to supply a name since it has none).
    log[INFO] <<"the following message streams are enabled after command-line processing:\n";
    MessageIndenter indenter(log[INFO]);
    log_facilities.print(log[INFO]);
    
    return 0;
}

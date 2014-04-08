// Tests basic command-line parsing using only Sawyer objects.
#include <sawyer/Message.h>

using namespace Sawyer::Message;

int main(int argc, char *argv[]) 
{
    // This is meant to be a SIMPLE example, so we use only the objects already defined by Sawyer.  This next line demonstrates
    // how to disable low-importance messages.  It's processed left-to-right: first disable everything, then enable things of
    // informational and greater importance.
    mfacilities.control("none, >=info");

    // Parse command-line switches. Sawyer configuration is separated from switch parsing so that you can parse switches
    // however you like.  Note that our error message might not show up because we might have disabled the very stream to which
    // we're emitting error messages.  The control() function makes no changes if the string has a syntax error, but the syntax
    // error doesn't necessarily have to be in the first switch. E.g., "a.out --log=none --log=syntax_error".
    for (int argno=1; argno<argc; ++argno) {
        if (0==strncmp(argv[argno], "--log=", 6)) {
            std::string errmesg = mfacilities.control(argv[argno]+6);
            if (!errmesg.empty())
                mlog[ERROR] <<errmesg <<"\n";
        } else {
            std::cerr <<"usage: " <<argv[0] <<" --log=LOG_SPECS\n";
            exit(1);
        }
    }

    // Show the results. We could emit this stuff using Sawyer::Message::mlog[INFO] (or other level) but if we happened to
    // disable that stream with our command-line parsing this test wouldn't display anything.  So instead we just send the
    // output to std::cerr.  This is another benefit of Sawyer: std::cerr and mlog[INFO] etc are mostly interchangeable.
    std::cerr <<"The following message streams are enabled after command-line processing:\n"
              <<"'-' means disabled. Letters are (D)ebug, (T)race, (W)here, (I)nfo, (W)arning, (E)rror, and (F)atal.\n";
    mfacilities.print(std::cerr);

    // Demonstrate the results by emitting some messages
    std::cerr <<"\nAbout to emit one of each kind of message (only enabled streams will show):\n";
    mlog[DEBUG] <<"a low-level debugging message intended for developers\n";
    mlog[TRACE] <<"a fine-level trace message intended mostly for developers\n";
    mlog[WHERE] <<"a coarse-level trace message intended for developers and users\n";
    mlog[INFO] <<"an informational message intended for end users\n";
    mlog[WARN] <<"a warning message about an unusual situation\n";
    mlog[ERROR] <<"a message about a recoverable error situation\n";
    mlog[FATAL] <<"a message about a non-recoverable error\n";
    return 0;
}

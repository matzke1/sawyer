// Test that we can use a default constructed facility
#include "Message.h"
#include <iostream>

using namespace Sawyer::Message::Common;

static Facility facility1("default");

static int g1 = ((facility1[INFO]<<"global initializer\n"), 0); // not guaranteed to work, but often does

int main(int argc, char *argv[]) {
    facility1[INFO] <<"made it to main()\n";

    // Now try changing the destinations to what we really want: buffered I/O.  We'll use stdout so we can distinguish the
    // output from the default, which was stderr.
    facility1.initStreams(Sawyer::Message::StreamSink::instance(std::cout));
    facility1[INFO] <<"this should be on standard output now\n";

    return 0;
}

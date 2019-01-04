// Test that we can use a default constructed facility
#include <boost/lexical_cast.hpp>
#include <Sawyer/Message.h>
#include <iostream>

using namespace Sawyer::Message::Common;

class Definer {
public:
    void *x;
    int y[100];
    int initialized;
    Facility facility;
    Definer(): x(NULL), initialized(123) {
        facility.initialize("default");
        std::cerr <<"Definer constructor called\n";
    }
};

class User {
public:
    User(Definer &definer) {
        std::cerr <<"User constructor called\n";

        // We're trying to test what happens if we use a statically-constructed message stream before the C++ runtime has had a
        // chance to initialize it.
        if (123 == definer.initialized)
            throw std::runtime_error("Definer was initialized before user!");

        // A statically-constructed object should be initialized to zero before its constructor is called. This is because
        // they exist in the BSS.
        if (definer.x)
            throw std::runtime_error("in user: definer.x is not null");
        for (int i=0; i<100; ++i) {
            if (definer.y[i])
                throw std::runtime_error("in user: definer.y[" + boost::lexical_cast<std::string>(i) + "] != 0");
        }
        
        // Try using an uninitialized facility. This should throw a facility-not-initialized error, not a segfault.
        std::cerr <<"Segmentation fault indicates a logic error in Sawyer::Message...\n";
        try {
            definer.facility[INFO] << "global initializer\n";
            throw std::runtime_error("expected an std::runtime_error");
        } catch (const std::runtime_error &e) {
            if (0 != strncmp(e.what(), "stream ", 7))
                throw; // expected something like "stream INFO in facility 0x60ece0 is not initailized yet"
            std::cerr <<"got facility-not-initialized error; good!\n";
        }
    }
};

// Cause definer to be initialized after its user
Definer definer __attribute__((init_priority(2001)));
User user __attribute__((init_priority(2000))) (definer);

int main() {
    definer.facility[INFO] <<"made it to main()\n";

    // Now try changing the destinations to what we really want: buffered I/O.  We'll use stdout so we can distinguish the
    // output from the default, which was stderr.
    definer.facility.initStreams(Sawyer::Message::StreamSink::instance(std::cout));
    definer.facility[INFO] <<"this should be on standard output now\n";

    return 0;
}

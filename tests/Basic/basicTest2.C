// Override version numbers in the Sawyer.h header file
#define SAWYER_VERSION_MAJOR 900
#define SAWYER_VERSION_MINOR 0
#define SAWYER_VERSION_PATCH 0

#include <sawyer/Sawyer.h>
#include <iostream>

int
main() {
    try {
        Sawyer::initializeLibrary();
        std::cerr <<"***ERROR: an std::runtime_error was expected regarding mismatched version numbers.\n";
        exit(1);
    } catch (const std::runtime_error &e) {
        if (strncmp(e.what(), "inconsistent ", 13)) {
            std::cerr <<"***ERROR: expected an std::runtime_error regarding mismatched version numbers.\n";
            exit(1);
        }
    }
}

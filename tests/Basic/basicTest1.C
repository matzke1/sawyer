#include <sawyer/Sawyer.h>
#include <iostream>

int
main() {
    try {
        Sawyer::initializeLibrary(911);
        std::cerr <<"***ERROR: an std::runtime_error was expected regarding mismatched version numbers.\n";
        exit(1);
    } catch (const std::runtime_error &e) {
        if (strncmp(e.what(), "inconsistent ", 13)) {
            std::cerr <<"***ERROR: expected an std::runtime_error regarding mismatched version numbers.\n";
            exit(1);
        }
    }
}

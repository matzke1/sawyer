#include "CommandLine.h"

using namespace Sawyer::CommandLine;

int main(int argc, char *argv[]) {

    Parser p;

    int i=0;
    p.with(Switch("log", 'l')
           .longPrefix("-log"));

    p.parse(argc, argv);
}


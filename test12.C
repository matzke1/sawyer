#include "CommandLine.h"

using namespace Sawyer::CommandLine;

int main(int argc, char *argv[]) {

    Parser p;

    p.with(Switch("home"));


    p.parse(argc, argv);
}


#include "CommandLine.h"

using namespace Sawyer::CommandLine;

int main(int argc, char *argv[]) {

    Parser p;

    int i=0;
    p.with(Switch("sw", 's')
           .argument("a1", integerParser(i))
           .whichValue(SAVE_FIRST));


    p.parse(argc, argv);
}


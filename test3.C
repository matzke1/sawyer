#include "Sawyer.h"
#include "Assert.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

enum Color {RED, GREEN, BLUE};

std::string
color_name(Color c)
{
    switch (c) {
        case RED: return "RED";
        case GREEN: return "GREEN";
        case BLUE: return "BLUE";
    }
    not_reachable("impossible color");
}

void adjust(Color *color)
{
    if (BLUE!=*not_null(color)) {
        *color = (Color)(*color+1);
    } else {
        not_implemented("blue is a difficult color");
    }
}

int main(int argc, char *argv[])
{
    using namespace Sawyer;
    MessageFacility log("");

    // Poor man's argument checking
    require2(argc>=2, "wrong number of program arguments");
    require(strlen(argv[1])>0);
    forbid2(argv[1][0]=='/', "absolute path names prohibited");

    // Emit a log message with a possible error right in the middle of the "color is" message
    Color c = (Color)strtol(argv[1], NULL, 0);
    log[INFO] <<"color " <<c;
    log[INFO] <<" is " <<color_name(c) <<"\n";

    adjust(c==RED ? NULL : &c);

    TODO("if we got this far, then we should do some real work");
    return 0;
}

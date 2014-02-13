#include "Assert.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

enum Color {RED, GREEN, BLUE, ORANGE, OTHER};

std::string
color_name(Color c)
{
    switch (c) {
        case RED: return "RED";
        case GREEN: return "GREEN";
        case BLUE: return "BLUE";
        case ORANGE: FIXME("I have no idea how to do this [Robb Matzke 2014-01-18]");
        // BTW, the citation on the previous line can be entered by typing "~~~" in Emacs Pilf mode.
    }
    ASSERT_not_reachable("impossible color");
}

void adjust(Color *color)
{
    ASSERT_not_null(color);
    if (BLUE!=*color) {
        *color = (Color)(*color+1);
    } else {
        ASSERT_not_implemented("blue is a difficult color");
    }
}

int main(int argc, char *argv[])
{
    using namespace Sawyer::Message;

    // Poor man's argument checking. Normally you'd want to recover gracefully from user errors.
    ASSERT_require2(argc>=2, "wrong number of program arguments");
    ASSERT_require(strlen(argv[1])>0);
    ASSERT_forbid2(argv[1][0]=='/', "absolute path names prohibited");

    // Emit a log message with a possible error right in the middle of the "color is" message
    Color c = (Color)strtol(argv[1], NULL, 0);
    mlog[INFO] <<"color " <<c;
    mlog[INFO] <<" is " <<color_name(c) <<"\n";

    adjust(c==RED ? NULL : &c);

    TODO("if we got this far, then we should do some real work");
    return 0;
}

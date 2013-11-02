#include "ProgressBar.h"

using namespace Sawyer;

struct timespec delay = {0, 25000000};

void test1()
{
    merr.use_color(isatty(2));
    MessageFacility log("test1");

    int total = 200;
    ProgressBar<int> progress("test1", total);

    for (int i=0; i<total; ++i, ++progress) {
        nanosleep(&delay, NULL); // represents substantial work
        if (i==total/2)
            log[DEBUG] <<"about half way\n";
    }
}

int main()
{
    test1();
    return 0;
}

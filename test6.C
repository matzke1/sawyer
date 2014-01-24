// The most basic test possible

#include "Message.h"
using namespace Sawyer::Message;

int main(int argc, char *argv[]) {
    logger[DEBUG] <<"a low-level debugging message intended for developers\n";
    logger[TRACE] <<"a fine-level trace message intended mostly for developers\n";
    logger[WHERE] <<"a coarse-level trace message intended for developers and users\n";
    logger[INFO] <<"an informational message intended for end users\n";
    logger[WARN] <<"a warning message about an unusual situation\n";
    logger[ERROR] <<"a message about a recoverable error situation\n";
    logger[FATAL] <<"a message about a non-recoverable error\n";

    return 0;
}

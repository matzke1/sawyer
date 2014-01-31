// The most basic test possible

#include "Message.h"
using namespace Sawyer::Message;

int main(int argc, char *argv[]) {
    log[DEBUG] <<"a low-level debugging message intended for developers\n";
    log[TRACE] <<"a fine-level trace message intended mostly for developers\n";
    log[WHERE] <<"a coarse-level trace message intended for developers and users\n";
    log[INFO] <<"an informational message intended for end users\n";
    log[WARN] <<"a warning message about an unusual situation\n";
    log[ERROR] <<"a message about a recoverable error situation\n";
    log[FATAL] <<"a message about a non-recoverable error\n";

    return 0;
}

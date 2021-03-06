// The most basic test possible

#include <Sawyer/Message.h>
using namespace Sawyer::Message;

int main() {
    Sawyer::initializeLibrary();
    mlog[DEBUG] <<"a low-level debugging message intended for developers\n";
    mlog[TRACE] <<"a fine-level trace message intended mostly for developers\n";
    mlog[WHERE] <<"a coarse-level trace message intended for developers and users\n";
    mlog[MARCH] <<"a progress report\n";
    mlog[INFO] <<"an informational message intended for end users\n";
    mlog[WARN] <<"a warning message about an unusual situation\n";
    mlog[ERROR] <<"a message about a recoverable error situation\n";
    mlog[FATAL] <<"a message about a non-recoverable error\n";

    return 0;
}

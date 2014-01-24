#include "ProgressBar.h"
#include <cstdio>

using namespace Sawyer;
using namespace Sawyer::Message;

struct timespec delay = {0, 100000};
typedef int ValueType;

void work(ValueType niter, ProgressBar<ValueType> &progress) {
    logger[WHERE] <<"starting work, going up\n";
    for (ValueType i=0; i<niter; ++i, ++progress) {
        nanosleep(&delay, NULL); // represents substantial work
        if (i==niter/ValueType(2))
            logger[WARN] <<"about half way\n";
    }
    logger[WHERE] <<"now going down\n";
    for (ValueType i=0; i<niter; ++i, --progress) {
        nanosleep(&delay, NULL); // represents substantial work
        if (i==niter/ValueType(2))
            logger[WARN] <<"about half way\n";
    }
}

// Basic progress bar
void test1(ValueType niter, const SProxy &stream) {
    logger[WHERE] <<"basic progress bar\n";
    ProgressBar<ValueType> progress(niter, stream);
    work(niter, progress);
}

// Offset progress bar
void test2(ValueType niter, const SProxy &stream) {
    logger[WHERE] <<"progress bar with offset\n";
    ProgressBar<ValueType> progress(ValueType(1000000), ValueType(1000000), ValueType(1000000)+niter, stream);
    work(niter, progress);
}

// Backward progress bar
void test3(ValueType niter, const SProxy &stream) {
    logger[WHERE] <<"backward progress bar\n";
    ProgressBar<int> progress(niter, ValueType(0), ValueType(0), stream);
    work(niter, progress);
}

// Empty progress bar (spinner)
void test4(ValueType niter, const SProxy &stream) {
    logger[WHERE] <<"spinner progress bar\n";
    ProgressBar<int> progress(stream);
    work(niter, progress);
}

int main()
{
    ProgressBar<ValueType>::minimumUpdateInterval(0.1);
    ValueType niter = 40000;
    test1(niter, logger[INFO]);
    test2(niter, logger[INFO]);
    test3(niter, logger[INFO]);
    test4(niter, logger[INFO]);
    return 0;
}

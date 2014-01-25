#include "ProgressBar.h"
#include <cstdio>

using namespace Sawyer;
using namespace Sawyer::Message;

struct timespec delay = {0, 100000};

template <typename T>
void work(T niter, ProgressBar<T> &progress) {
    logger[WHERE] <<"starting work, going up\n";
    for (T i=0; i<niter; ++i, ++progress) {
        nanosleep(&delay, NULL); // represents substantial work
        if (i==niter/2)
            logger[WARN] <<"about half way\n";
    }
    logger[WHERE] <<"now going down\n";
    for (T i=0; i<niter; ++i, --progress) {
        nanosleep(&delay, NULL); // represents substantial work
        if (i==niter/2)
            logger[WARN] <<"about half way\n";
    }
}

// Basic progress bar
void test1(size_t niter, const SProxy &stream) {
    logger[WHERE] <<"basic progress bar\n";
    ProgressBar<size_t> progress(niter, stream, "test1");
    work(niter, progress);
}

// Offset progress bar
void test2(size_t niter, const SProxy &stream) {
    logger[WHERE] <<"progress bar with offset\n";
    ProgressBar<size_t> progress(1000000, 1000000, 1000000+niter, stream);
    work(niter, progress);
}

// Backward progress bar
void test3(size_t niter, const SProxy &stream) {
    logger[WHERE] <<"backward progress bar\n";
    ProgressBar<size_t> progress(niter, 0, 0, stream, "test3");
    work(niter, progress);
}

// Empty progress bar (spinner)
void test4(size_t niter, const SProxy &stream) {
    logger[WHERE] <<"spinner progress bar\n";
    ProgressBar<size_t> progress(stream, "test4");
    work(niter, progress);
}

// Floating point progress bar
void test5(size_t niter, const SProxy &stream) {
    logger[WHERE] <<"floating point progress\n";
    ProgressBar<double> progress(0.0, 0.0, 1.0, stream, "probability of ending");
    double delta = 1.0 / niter;
    for (size_t i=0; i<niter; ++i, progress+=delta)
        nanosleep(&delay, NULL); // represents substantial work
}

// Floating point progress bar with negative values
void test6(size_t niter, const SProxy &stream) {
    logger[WHERE] <<"negative floating point progress\n";
    ProgressBar<double> progress(-1.0, -1.0, 0.0, stream, "FP-negative");
    double delta = 1.0 / niter;
    for (size_t i=0; i<niter; ++i, progress+=delta)
        nanosleep(&delay, NULL); // represents substantial work
}

// underflow and overflow
void test7(size_t niter, const SProxy &stream) {
    logger[WHERE] <<"under and overflow\n";
    ProgressBar<size_t> progress(niter/4, 0, niter-niter/4, stream, "underflow/overflow");
    work(niter, progress);
}

int main()
{
    ProgressBarSettings::initialDelay(1.0);
    size_t niter = 40000;

    test1(niter, logger[INFO]);
    test2(niter, logger[INFO]);
    test3(niter, logger[INFO]);
    test4(niter, logger[INFO]);
    test5(niter, logger[INFO]);
    test6(niter, logger[INFO]);
    test7(niter, logger[INFO]);

    return 0;
}

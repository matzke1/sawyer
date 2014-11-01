#include <sawyer/ProgressBar.h>
#include <boost/config.hpp>
#include <cstdio>

using namespace Sawyer;
using namespace Sawyer::Message;

#ifdef BOOST_WINDOWS // FIXME[Robb Matzke 2014-06-10]: how to specify duration on Windows and boost < 1.47?
size_t delayDuration = 1000000;                         // loop iterations

static void delay(size_t duration) {
    for (volatile size_t i=0; i<duration; ++i) /*void*/;
}
#else
struct timespec delayDuration = {0, 100000};

static void delay(const struct timespec &duration) {
    nanosleep(&duration, NULL);
}
#endif



template <typename T, typename S>
void work(T niter, ProgressBar<T, S> &progress) {
    Message::mlog[WHERE] <<"starting work, going up\n";
    for (T i=0; i<niter; ++i, ++progress) {
        delay(delayDuration);                           // represents substantial work
        if (i==niter/2)
            Message::mlog[WARN] <<"about half way\n";
    }
    Message::mlog[WHERE] <<"now going down\n";
    for (T i=0; i<niter; ++i, --progress) {
        delay(delayDuration);                           // represents substantial work
        if (i==niter/2)
            Message::mlog[WARN] <<"about half way\n";
    }
}

// Basic progress bar
void test1(size_t niter, const SProxy &stream) {
    Message::mlog[WHERE] <<"basic progress bar\n";
    ProgressBar<size_t> progress(niter, stream, "test1");
    progress.suffix(" items");
    work(niter, progress);
}

struct Suffix {
    std::string name;
    Suffix() {}
    Suffix(const std::string &name): name(name) {}
};

std::ostream& operator<<(std::ostream &o, const Suffix &suffix) {
    o <<" (" <<suffix.name <<")";
    return o;
}

// Offset progress bar
void test2(size_t niter, const SProxy &stream) {
    Suffix suffix("hello world");
    Message::mlog[WHERE] <<"progress bar with offset\n";
    ProgressBar<size_t, Suffix> progress(1000000, 1000000, 1000000+niter, stream);
    progress.suffix(suffix);
    work(niter, progress);
}

// Backward progress bar
void test3(size_t niter, const SProxy &stream) {
    Message::mlog[WHERE] <<"backward progress bar\n";
    ProgressBar<size_t> progress(niter, 0, 0, stream, "test3");
    work(niter, progress);
}

// Empty progress bar (spinner)
void test4(size_t niter, const SProxy &stream) {
    Message::mlog[WHERE] <<"spinner progress bar\n";
    ProgressBar<size_t> progress(stream, "test4");
    work(niter, progress);
}

// Floating point progress bar
void test5(size_t niter, const SProxy &stream) {
    Message::mlog[WHERE] <<"floating point progress\n";
    ProgressBar<double> progress(0.0, 0.0, 1.0, stream, "probability");
    double delta = 1.0 / niter;
    for (size_t i=0; i<niter; ++i, progress+=delta)
        delay(delayDuration);                           // represents substantial work
}

// Floating point progress bar with negative values
void test6(size_t niter, const SProxy &stream) {
    Message::mlog[WHERE] <<"negative floating point progress\n";
    ProgressBar<double> progress(-1.0, -1.0, 0.0, stream, "negative");
    double delta = 1.0 / niter;
    for (size_t i=0; i<niter; ++i, progress+=delta)
        delay(delayDuration);                           // represents substantial work
}

// underflow and overflow
void test7(size_t niter, const SProxy &stream) {
    Message::mlog[WHERE] <<"under and overflow\n";
    ProgressBar<size_t> progress(niter/4, 0, niter-niter/4, stream, "under/over");
    work(niter, progress);
}

int main()
{
    ProgressBarSettings::initialDelay(1.0);
    size_t niter = 40000;

    test1(niter, Message::mlog[MARCH]);
    test2(niter, Message::mlog[MARCH]);
    test3(niter, Message::mlog[MARCH]);
    test4(niter, Message::mlog[MARCH]);
    test5(niter, Message::mlog[MARCH]);
    test6(niter, Message::mlog[MARCH]);
    test7(niter, Message::mlog[MARCH]);

    return 0;
}

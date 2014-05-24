#include <sawyer/Stopwatch.h>

namespace Sawyer {

static Stopwatch::TimePoint getCurrentTime() {
    return boost::chrono::high_resolution_clock::now();
}

double Stopwatch::report(bool clear) const {
    if (running_) {
        TimePoint now = getCurrentTime();
        elapsed_ += now - begin_;
        begin_ = now;
    }
    double retval = elapsed_.count();
    if (clear)
        elapsed_ = Duration();
    return retval;
}

double Stopwatch::start(bool clear) {
    double retval = report(clear);
    if (!running_) {
        begin_ = getCurrentTime();
        running_ = true;
    }
    return retval;
}

double Stopwatch::stop(bool clear) {
    double retval = report(clear);
    running_ = false;
    return retval;
}

double Stopwatch::clear(double value) {
    double retval = stop();
    elapsed_ = Duration(value);
    return retval;
}

} // namespace

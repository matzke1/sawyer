#ifndef Sawyer_ProgressBar_H
#define Sawyer_ProgressBar_H

#include "Sawyer.h"
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sys/time.h>

// documented in Sawyer.h
namespace Sawyer {

/** Progress bars.
 *
 *  Progress bars are fully integrated with the Sawyer logging system so that they behave with respect to other messages.
 *
 *  Example usage:
 * @code
 *  void f() {
 *      int total = 200;
 *      ProgressBar<int> progress("test", total);
 *      for (int i=0; i<total; ++i, ++progress)
 *          do_some_work();
 *  }
 * @endcode
 *
 *  The progress bar is created with a name and capacity. As the progress bar is incremented the bar will increase.  Messages
 *  printed while the progress bar is active do not interfere with the progress bar. When the progress bar object is destructed
 *  the progress bar disappears. */
template<typename T>
class ProgressBar {
public:
    typedef T value_type;
private:
    struct Position {
        value_type vcur, vmin, vmax;    // current, minimum, maximum
        Position(value_type vcur, value_type vmin, value_type vmax): vcur(vcur), vmin(vmin), vmax(vmax) {}
        bool operator==(const Position &other) const {
            return vcur==other.vcur && vmin==other.vmin && vmax==other.vmax;
        }
    };

    Position value_;                            // current position of progress bar
    Position update_value_;                     // value at time of last update
    double min_update_, max_update_;            // min and max elapsed time between updates
    size_t width_;                              // width of the bar itself
    char barchar_lt_, barchar_rt_, endchar_lt_, endchar_rt_;
    bool show_percent_;                         // show progress bar percent complete?
    MessageStream mstream_;                     // stream to which the progress bar is emitted
    struct timeval last_update_;                // time of last update
    bool use_ansi_;                             // use ANSI escape sequences for fancy progress bars?
public:
    ProgressBar(const std::string &name, value_type minval, value_type maxval)
        : value_(minval, minval, maxval), update_value_(minval-1, minval, maxval),
          min_update_(0.2), max_update_(0), width_(70),
          barchar_lt_('#'), barchar_rt_('-'), endchar_lt_('['), endchar_rt_(']'),
          show_percent_(true), mstream_(name, INFO) { init(); }
    ProgressBar(const std::string &name, value_type maxval)
        : value_(0, 0, maxval), update_value_(-1, 0, maxval),
          min_update_(0.2), max_update_(0), width_(70),
          barchar_lt_('#'), barchar_rt_('-'), endchar_lt_('['), endchar_rt_(']'),
          show_percent_(true), mstream_(name, INFO)  { init(); }

    /** Value for the progress bar. */
    value_type value() const { return value_.vcur; }
    void value(value_type value) { value_.vcur = value; update_maybe(); }

    /** Value of progress bar as a ration of completeness clipped between 0 and 1. */
    double ratio() const;

    /** Range of possible values. These indicate the zero and 100% end points. */
    value_type minvalue() const { return value_.vmin; }
    void minvalue(value_type minval) { value_.vmin = minval; update_maybe(); }
    value_type maxvalue() const { return value_.vmax; }
    void maxvalue(value_type maxval) { value_.vmax = maxval; update_maybe(); }

    /** Minimum time between updates. Time is measured in seconds. */
    double min_update() const { return min_update_; }
    void min_update(double nseconds) { min_update_ = nseconds; update_maybe(); }

    /** If non-zero, then a separate thread updates the bar every N seconds. */
    double max_update() const { return max_update_; }
    void max_update(double nseconds) { max_update_ = nseconds; }

    /** Increment the progress bar value by one. */
    void increment(value_type delta=1);
    ProgressBar& operator++() { increment(1); return *this; }
    ProgressBar& operator++(int) { increment(1); return *this; }

    /** Width of progress bar at 100% */
    size_t width() const { return width_; }
    void width(size_t width) { width_ = width; update_maybe(); }

    /** Characters to use for the bar. The first is from zero to the current ratio() and the second is the character with which
     *  to fill the rest of the bar's area. */
    std::pair<char, char> barchars() const { return std::make_pair(barchar_lt_, barchar_rt_); }
    void barchars(char lt, char rt) { barchar_lt_=lt; barchar_rt_=rt; update_maybe(); }

    /** Characters to use for the left and right ends of the bar.  The default is '[' and ']'. */
    std::pair<char, char> endchars() const { return std::make_pair(endchar_lt_, endchar_rt_); }
    void endchars(char lt, char rt) { endchar_lt_=lt; endchar_rt_=rt; update_maybe(); }

    /** Whether to show the percent completion. */
    bool show_percent() const { return show_percent_; }
    void show_percent(bool b) const { show_percent = b; update_maybe(); }

    /** Use ANSI escape sequences?  If standard error is a terminal, then escape sequences are used the the progress bar is
     * (hopefully) pretty. */
    bool use_ansi() const { return use_ansi_; }
    void use_ansi(bool b);

    /** Whether to show the progress bar. */
    bool enabled() const { return mstream_.enabled(); }
    void enable(bool b=true);
    void disable() { enable(false); }

    /** Unconditionally update the progress bar. */
    void update();

    /** Update only if it's been a while. */
    void update_maybe();

private:
    void init();
};

template <typename T>
void
ProgressBar<T>::init()
{
    // FIXME: we're assuming that the progress bar is using the merr sink because there's no way I know of to tell whether an
    // arbitrary std::ofstream is directed to a tty.
    memset(&last_update_, 0, sizeof last_update_);
    use_ansi(isatty(2));
}

template <typename T>
void
ProgressBar<T>::use_ansi(bool b)
{
    if (!b) {
        mstream_.interrupt_string("\n");
        mstream_.cleanup_string("\n");
    } else {
        mstream_.interrupt_string("\r\033[K");
        mstream_.cleanup_string("\r\033[K");
    }
    use_ansi_ = b;
}

template <typename T>
void
ProgressBar<T>::increment(value_type delta)
{
    value_type old_value = value_.vcur;
    value_.vcur += delta;
    if (old_value < value_.vmax && value_.vcur >= value_.vmax) {
        update();
    } else {
        update_maybe();
    }
}

template <typename T>
double
ProgressBar<T>::ratio() const
{
    assert(value_.vmin <= value_.vmax);
    if (value_.vcur <= value_.vmin)
        return 0.0;
    if (value_.vcur >= value_.vmax)
        return 1.0;
    return (value_.vcur - value_.vmin) / (double)(value_.vmax - value_.vmin);
}

template <typename T>
void
ProgressBar<T>::enable(bool b)
{
    if (mstream_.enabled() && !b) {
        mstream_ <<" DISABLED";
        mstream_ <<(use_ansi_?"\r\033[K":"\n");
        mstream_.clear();
        mstream_.disable();
    } else if (!mstream_.enabled() && b) {
        mstream_.enable();
        update();
    }
}

template <typename T>
void
ProgressBar<T>::update_maybe()
{
    // Skip call if the previous update was too recent
    struct timeval now;
    gettimeofday(&now, NULL);
    if (last_update_.tv_sec > 0) {
        double elapsed = (now.tv_sec - last_update_.tv_sec) + 1e-6 * ((double)now.tv_usec - last_update_.tv_usec);
        if (elapsed < min_update_)
            return;
    }

    // Skip the call if there's no change
    if (update_value_ == value_)
        return;

    last_update_ = now;
    update();
}

template <typename T>
void
ProgressBar<T>::update()
{
    // Redraw the whole message.
    mstream_ <<(use_ansi_?"\r":"\n");
    mstream_.clear();

    // Percent complete
    if (show_percent_) {
        int pct = round(100*ratio());
        mstream_ <<std::setw(3) <<std::right <<pct <<"% ";
    }

    // Progress bar
    if (endchar_lt_)
        mstream_ <<endchar_lt_;
    size_t barsz = round(ratio()*width_);
    if (use_ansi_)
        mstream_ <<"\033[7m"; // green background to match INFO level color
    mstream_ <<std::string(barsz, barchar_lt_);
    if (use_ansi_)
        mstream_ <<"\033[m";
    mstream_ <<std::string(width_-barsz, barchar_rt_);
    if (endchar_rt_)
        mstream_ <<endchar_rt_;
    if (use_ansi_)
        mstream_ <<"\033[K";

    update_value_ = value_;
}



} // namespace
#endif

#ifndef Sawyer_ProgressBar_H
#define Sawyer_ProgressBar_H

#include "Message.h"
#include <cmath>

namespace Sawyer {

class ProgressBarImpl {
public:
    double value_;                                      // between zero and one, inclusive
    size_t width_;                                      // width of bar in characters
    std::string leftEnd_, rightEnd_;                    // strings for left and right ends of progress bar
    char barChar_, nonBarChar_;                         // characters for the bar and non-bar parts
    static double minUpdateInterval_;                   // min number of seconds between update
    double lastUpdateTime_;                             // time of previous update
    Message::MesgProps overridesAnsi_;                  // properties we override from the stream_ when using color
    Message::SProxy stream_;                            // stream to which messages are sent
    size_t nUpdates_;                                   // number of times a message was emitted
    bool shouldSpin_;                                   // spin instead of progress

    ProgressBarImpl(const Message::SProxy &stream)
        : value_(0.0), width_(30), leftEnd_("["), rightEnd_("]"), barChar_('#'), nonBarChar_('-'),
          lastUpdateTime_(0.0), stream_(stream), nUpdates_(0), shouldSpin_(false) {
        init();
    }
    ~ProgressBarImpl() {
        cleanup();
    }

    void init();
    void cleanup();                                     // deletes the progress bar from the screen
    void update(double ratio, bool backward);           // update regardless of time (if stream is enabled)
    void configUpdate(double ratio, bool backward);     // update for configuration changes
    void valueUpdate(double ratio, bool backward);      // update for changes in value
    std::string makeBar(double ratio, bool backward);   // make the bar itself
};

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
    typedef T ValueType;
private:
    struct Position {
        ValueType leftValue, curValue, rightValue;
        Position(ValueType leftValue, ValueType curValue, ValueType rightValue)
            : leftValue(leftValue), curValue(curValue), rightValue(rightValue) {}
        bool operator==(const Position &other) const {
            return curValue==other.curValue && leftValue==other.leftValue && rightValue==other.rightValue;
        }
    };

    Position value_;
    ProgressBarImpl bar_;

public:
    explicit ProgressBar(const Message::SProxy &stream)
        : value_(0, 0, 0), bar_(stream) {
        bar_.shouldSpin_ = true;
    }
    ProgressBar(ValueType rightValue, const Message::SProxy &stream)
        : value_(0, 0, rightValue), bar_(stream) {
        bar_.shouldSpin_ = isEmpty();
    }
    ProgressBar(ValueType leftValue, ValueType curValue, ValueType rightValue, const Message::SProxy &stream)
        : value_(leftValue, curValue, rightValue), bar_(stream) {
        bar_.shouldSpin_ = isEmpty();
    }

    /** Value for the progress bar.
     *  @{ */
    ValueType value() const {
        return value_.curValue;
    }
    void value(ValueType curValue) {
        value_.curValue = curValue;
        bar_.valueUpdate(ratio(), isBackward());
    }
    void value(ValueType curValue, ValueType rightValue) {
        value_.curValue = curValue;
        value_.rightValue = rightValue;
        bar_.shouldSpin_ = isEmpty();
        bar_.configUpdate(ratio(), isBackward());
    }
    void value(ValueType leftValue, ValueType curValue, ValueType rightValue) {
        value_ = Position(leftValue, curValue, rightValue);
        bar_.shouldSpin_ = isEmpty();
        bar_.configUpdate(ratio(), isBackward());
    }
    /** @} */

    /** Value of progress bar as a ratio of completeness clipped between 0 and 1.  A progress bar that is backward (min value
     *  is greater than max value) also returns a value between zero and one, and also is a measurement of how far the progress
     *  bar should be drawn from the left side toward the right. */
    double ratio() const;
    
    /** True if the distance between the minimum and maximum is zero. */
    bool isEmpty() const {
        return value_.leftValue == value_.rightValue;
    }

    /** True if the minimum value is greater than the maximum value. */
    bool isBackward() const {
        return value_.leftValue > value_.rightValue;
    }

    /** Possible values. These indicate the zero and 100% end points.
     *  @{ */
    std::pair<ValueType, ValueType> domain() const {
        return std::make_pair(value_.leftValue, value_.rightValue);
    }
    void domain(const std::pair<ValueType, ValueType> &p) {
        value_.leftValue = p.first;
        value_.rightValue = p.second;
        bar_.configUpdate(ratio(), isBackward());
    }
    void domain(ValueType leftValue, ValueType rightValue) {
        value_.leftValue = leftValue;
        value_.rightValue = rightValue;
        bar_.configUpdate(ratio(), isBackward());
    }
    /** @} */

    /** Increment or decrement the progress bar.
     *  @{ */
    void increment(ValueType delta=1);
    void decrement(ValueType delta=1) {
        increment(-delta);
    }
    ProgressBar& operator++() {
        increment(1);
        return *this;
    }
    ProgressBar& operator++(int) {                      // same as a pre-increment
        increment(1);
        return *this;
    }
    ProgressBar& operator--() {
        decrement(1);
        return *this;
    }
    ProgressBar& operator--(int) {                      // same as pre-decrement
        decrement(1);
        return *this;
    }
    /** @} */

    /** Width of progress bar in characters at 100%
     *  @{ */
    size_t width() const {
        return bar_.width_;
    }
    void width(size_t width) {
        bar_.width_ = width;
        bar_.configUpdate(ratio(), isBackward());
    }
    /** @} */

    /** Characters to use for the bar. The first is from zero to the current ratio() and the second is the character with which
     *  to fill the rest of the bar's area.  The defaults are '#' and '-'.
     *  @{ */
    std::pair<char, char> barchars() const {
        return std::make_pair(bar_.barChar_, bar_.nonBarChar_);
    }
    void barchars(char bar, char nonBar) {
        bar_.barChar_ = bar;
        bar_.nonBarChar_ = nonBar;
        bar_.configUpdate(ratio(), isBackward());
    }
    /** @} */

    /** Characters to use for the left and right ends of the bar.  The default is '[' and ']'.
     *  @{ */
    std::pair<std::string, std::string> endchars() const {
        return std::make_pair(bar_.leftEnd_, bar_.rightEnd_);
    }
    void endchars(const std::string &lt, const std::string &rt) {
        bar_.leftEnd_ = lt;
        bar_.rightEnd_ = rt;
        bar_.configUpdate(ratio(), isBackward());
    }
    /** @} */

    /** Minimum time between updates.  Measured in seconds.
     *  @{ */
    static double minimumUpdateInterval() { return ProgressBarImpl::minUpdateInterval_; }
    static void minimumUpdateInterval(double s) { ProgressBarImpl::minUpdateInterval_ = s; }
    /** @} */
};

// try not to get negative values when subtracting because they might behave strangely if T is something weird.
template <typename T>
double ProgressBar<T>::ratio() const {
    if (isEmpty()) {
        return value_.curValue <= value_.leftValue ? 0.0 : 1.0;
    } else if (isBackward()) {
        if (value_.curValue >= value_.leftValue) {
            return 0.0;
        } else if (value_.curValue <= value_.rightValue) {
            return 1.0;
        } else {
            return 1.0 * (value_.leftValue - value_.curValue) / (value_.leftValue - value_.rightValue);
        }
    } else {
        if (value_.curValue <= value_.leftValue) {
            return 0.0;
        } else if (value_.curValue >= value_.rightValue) {
            return 1.0;
        } else {
            return 1.0 * (value_.curValue - value_.leftValue) / (value_.rightValue - value_.leftValue);
        }
    }
}

template <typename T>
void ProgressBar<T>::increment(ValueType delta) {
    ValueType oldValue = value_.curValue;
    value_.curValue += delta;
    if (oldValue!=value_.curValue)
        bar_.valueUpdate(ratio(), isBackward());
}

} // namespace

#endif

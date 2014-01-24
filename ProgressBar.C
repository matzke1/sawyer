#include "ProgressBar.h"

namespace Sawyer {

double ProgressBarImpl::minUpdateInterval_ = 1.0;

void ProgressBarImpl::init() {
    static const char *clearLine = "\r\033[K";          // clear entire line of an ANSI terminal
    overridesAnsi_.isBuffered = false;
    overridesAnsi_.completionStr = clearLine;
    overridesAnsi_.interruptionStr = clearLine;
    overridesAnsi_.cancelationStr = clearLine;
    overridesAnsi_.lineTermination = "";                // empty because we erase lines instead
}

void ProgressBarImpl::cleanup() {
    if (*stream_) {
        Message::BakedDestinations baked;
        stream_->destination()->bakeDestinations(stream_->properties(), baked);
        for (size_t i=0; i<baked.size(); ++i) {
            if (baked[i].second.useColor) {
                Message::MesgProps props = overridesAnsi_.merge(baked[i].second);
                Message::Mesg mesg(props, "done");      // any non-empty message
                baked[i].first->post(mesg, props);
            }
        }
    }
}

std::string ProgressBarImpl::makeBar(double ratio, bool isBackward) {
    std::string bar;
    if (shouldSpin_) {
        int centerIdx = nUpdates_ % (2*width_);
        if (centerIdx >= (int)width_)
            centerIdx = 2*width_ - centerIdx - 1;
        assert(centerIdx>=0 && (size_t)centerIdx < width_);
        int indicatorWidth = ceil(0.3 * width_);
        int indicatorIdx = centerIdx - indicatorWidth/2;
        bar = std::string(width_, nonBarChar_);
        for (int i=std::max(indicatorIdx, 0); i<std::min(indicatorIdx+indicatorWidth, (int)width_); ++i)
            bar[i] = barChar_;
    } else {
        size_t barLen = round(ratio * width_);
        if (isBackward) {
            bar += std::string(width_-barLen, nonBarChar_) + std::string(barLen, barChar_);
        } else {
            bar += std::string(barLen, barChar_) + std::string(width_-barLen, nonBarChar_);
        }
    }
    return leftEnd_ + bar + rightEnd_;
}

void ProgressBarImpl::update(double ratio, bool isBackward) {
    if (*stream_ && width_>0) {
        Message::BakedDestinations baked;
        stream_->destination()->bakeDestinations(stream_->properties(), baked);
        if (!baked.empty()) {
            for (size_t i=0; i<baked.size(); ++i) {
                if (baked[i].second.useColor) {
                    Message::MesgProps props = overridesAnsi_.merge(baked[i].second);
                    Message::Mesg mesg(props);
                    mesg.insert(makeBar(ratio, isBackward));
                    baked[i].first->post(mesg, props);
                } else {
                    Message::Mesg mesg(baked[i].second, makeBar(ratio, isBackward));
                    baked[i].first->post(mesg, baked[i].second);
                }
            }
            ++nUpdates_;
        }
    }
}

void ProgressBarImpl::configUpdate(double ratio, bool isBackward) {
    update(ratio, isBackward);
}

void ProgressBarImpl::valueUpdate(double ratio, bool isBackward) {
    double curTime = Message::now();
    if (curTime - lastUpdateTime_ >= minUpdateInterval_) {
        update(ratio, isBackward);
        lastUpdateTime_ = curTime;
    }
}

} // namespace


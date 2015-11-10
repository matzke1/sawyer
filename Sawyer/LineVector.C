#include <Sawyer/LineVector.h>
#include <Sawyer/MappedBuffer.h>
#include <Sawyer/StaticBuffer.h>

namespace Sawyer {
namespace Container {

LineVector::LineVector(const std::string &fileName)
    : charBuf_(NULL), nextCharToScan_(0) {
    buffer_ = MappedBuffer<size_t, char>::instance(fileName);
    charBuf_ = buffer_->data();
    ASSERT_not_null(charBuf_);
}

LineVector::LineVector(const Buffer<size_t, char>::Ptr &buffer)
    : buffer_(buffer), charBuf_(NULL), nextCharToScan_(0) {
    ASSERT_not_null(buffer);
    charBuf_ = buffer_->data();
    ASSERT_not_null(charBuf_);
}

LineVector::LineVector(size_t nBytes, const char *buf)
    : charBuf_(NULL), nextCharToScan_(0) {
    buffer_ = StaticBuffer<size_t, char>::instance(buf, nBytes);
    charBuf_ = buffer_->data();
    ASSERT_not_null(charBuf_);
}

bool
LineVector::isEmpty() const {
    return 0 == buffer_->size();
}

bool
LineVector::isLastLineTerminated() const {
    size_t n = buffer_->size();
    return n>0 && '\n' == charBuf_[n-1];
}

size_t
LineVector::nLines() const {
    cacheLines((size_t)(-1));                           // cache to end of file
    return lineFeeds_.size() + (!isEmpty() && !isLastLineTerminated() ? 1 : 0);
}

size_t
LineVector::nCharacters() const {
    return buffer_->size();
}

size_t
LineVector::characterIndex(size_t lineIdx) const {
    if (0 == lineIdx)
        return 0;
    cacheLines(lineIdx);
    if (lineIdx-1 >= lineFeeds_.size())
        return nCharacters();
    return lineFeeds_[lineIdx-1] + 1;
}

size_t
LineVector::nCharacters(size_t lineIdx) const {
    return characterIndex(lineIdx+1) - characterIndex(lineIdx);
}

void
LineVector::cacheLines(size_t nLineFeeds) const {
    size_t n = nCharacters();
    size_t i = lineFeeds_.empty() ? (size_t)0 : lineFeeds_.back() + 1;
    i = std::max(i, nextCharToScan_);
    while (lineFeeds_.size() < nLineFeeds && i < n) {
        if ('\n' == charBuf_[i])
            lineFeeds_.push_back(i);
        ++i;
    }
    nextCharToScan_ = i;
}

void
LineVector::cacheCharacters(size_t nChars) const {
    size_t n = std::min(nCharacters(), nChars);
    size_t i = lineFeeds_.empty() ? (size_t)0 : lineFeeds_.back() + 1;
    i = std::max(i, nextCharToScan_);
    while (i < n) {
        if ('\n' == charBuf_[i])
            lineFeeds_.push_back(i);
        ++i;
    }
    nextCharToScan_ = i;
}

int
LineVector::character(size_t charIdx) const {
    return charIdx >= nCharacters() ? EOF : (int)charBuf_[charIdx];
}

int
LineVector::character(size_t lineIdx, size_t colIdx) const {
    size_t i = characterIndex(lineIdx);
    if (i >= nCharacters())
        return EOF;
    size_t n = nCharacters(lineIdx);
    if (colIdx >= n)
        return 0;
    return charBuf_[i+colIdx];
}

const char*
LineVector::characters(size_t charIndex) const {
    if (charIndex >= nCharacters())
        return NULL;
    return charBuf_ + charIndex;
}

const char*
LineVector::line(size_t lineIdx) const {
    size_t i = characterIndex(lineIdx);
    if (i >= nCharacters())
        return NULL;
    return charBuf_ + i;
}

size_t
LineVector::lineIndex(size_t charIdx) const {
    if (charIdx >= nCharacters())
        return nLines();
    cacheCharacters(charIdx+1);
    std::vector<size_t>::const_iterator found = std::lower_bound(lineFeeds_.begin(), lineFeeds_.end(), charIdx);
    if (found == lineFeeds_.end()) {
        return lineFeeds_.size();
    } else {
        return found - lineFeeds_.begin();
    }
}

std::pair<size_t, size_t>
LineVector::location(size_t charIdx) const {
    size_t lineIdx = lineIndex(charIdx);
    size_t colIdx = charIdx - characterIndex(lineIdx);
    return std::make_pair(lineIdx, colIdx);
}

} // namespace
} // namespace

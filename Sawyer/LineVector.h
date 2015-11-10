#ifndef Sawyer_Container_LineVector_H
#define Sawyer_Container_LineVector_H

#include <Sawyer/Buffer.h>

#include <algorithm>
#include <vector>

namespace Sawyer {
namespace Container {

/** A buffer of characters indexed by line number.
 *
 *  This is a character array indexed lines. The line indexes are computed only when necessary. */
class LineVector {
    Buffer<size_t, char>::Ptr buffer_;
    const char *charBuf_;
    mutable std::vector<size_t> lineFeeds_;
    mutable size_t nextCharToScan_;

public:
    /** Constructor that opens a file.
     *
     *  This constructor doesn't actually read the entire file, it only maps it into memory. */
    explicit LineVector(const std::string &fileName);

    /** Construct a line vector from a buffer. */
    explicit LineVector(const Buffer<size_t, char>::Ptr&);

    /** Construct a line vector from a string. */
    LineVector(size_t nBytes, const char *buf);

    /** Number of lines.
     *
     *  Total number of lines including any final line that lacks line termination. Calling this function will cause the entire
     *  file to be read (if it hasn't been already) in order to find all line termination characters. */
    size_t nLines() const;

    /** Number of characters.
     *
     *  Total number of characters including all line termination characters. */
    size_t nCharacters() const;

    /** Number of characters in a line.
     *
     *  Returns the number of characters in the specified line including line termination characters, if any. If @p lineIdx
     *  refers to a line that doesn't exist then the return value is zero. */
    size_t nCharacters(size_t lineIdx) const;

    /** Character at file offset.
     *
     *  Returns the character (as an int) at the specified file offset. If the @p charIdx is beyond the end of the file then
     *  EOF is returned. */
    int character(size_t charIdx) const;

    /** Characters at file offset.
     *
     *  Returns a pointer to the character at the specified file offset. The arrary of characters is valid through at least the
     *  following line terminator or the end of the file, whichever comes first.  Returns a null pointer if the character index
     *  is beyond the end of the file. */
    const char *characters(size_t charIdx) const;

    /** Character at line offset.
     *
     *  Returns the character (as an int) at the specified offset within a line. If @p lineIdx is beyond the end of the file
     *  then EOF is returne. Otherwise, if @p colIdx is beyond the end of a line then NUL characters are returned (which are
     *  also valid characters within a line). */
    int character(size_t lineIdx, size_t colIdx) const;

    /** Characters for a line.
     *
     *  Returns a pointer to a line's characters.  The array is guaranteed to contain at least those characters that appear in
     *  the specified line. The array is not necessarily NUL-terminated, and if a line contains NUL characters then the array
     *  will have NUL characters.  If @p lineIdx is beyond the end of the file then a null pointer is returned. */
    const char* line(size_t lineIdx) const;

    /** Character index for start of line.
     *
     *  Returns the character index for the first character in the specified line. If @p lineIdx is beyond the end of the file
     *  then the total number of characters is returned. */
    size_t characterIndex(size_t lineIdx) const;

    /** Convert a character index to a line index.
     *
     *  Returns the zero-origin line index that contains the specified character. LF characters (line feeds) are considered to
     *  be at the end of a line.  If @p charIndex is beyond the end of the file then the number of lines is returned. */
    size_t lineIndex(size_t charIndex) const;

    /** Convert a character index to a line and column index.
     *
     *  In error messages, line and column numbers are usually 1-origin, but this function returns zero-origin indexes. */
    std::pair<size_t, size_t> location(size_t charIndex) const;

    /** Determines whether the file ends with line termination.
     *
     *  Determining whether the file ends with line termination can be done without reading the entire file. An empty file is
     *  considered to be not terminated. */
    bool isLastLineTerminated() const;

    /** Determines whether the file is empty.
     *
     *  Returns true if the file is empty without trying to read any data. */
    bool isEmpty() const;

private:
    void cacheLines(size_t nLines) const;
    void cacheCharacters(size_t nCharacters) const;
};

} // namespace
} // namespace

#endif


#ifndef Sawyer_Document_BaseMarkup_H
#define Sawyer_Document_BaseMarkup_H

#include <Sawyer/DocumentMarkup.h>

namespace Sawyer {
namespace Document {

/** Base class for various documentation markup systems. */
class BaseMarkup: public Markup::Grammar {
protected:
    BaseMarkup() {
        init();
    }

public:
    /** Parse input to generate POD. */
    virtual std::string operator()(const std::string&) /*override*/;

    // True if this string contains any non-blank characters.
    static bool hasNonSpace(const std::string&);

    // Remove linefeeds
    static std::string makeOneLine(const std::string&);

    // Left justify a string in a field of width N (or more). String should not contain linefeeds
    static std::string leftJustify(const std::string&, size_t width);

protected:
    // Last thing called before the rendered document is returend
    virtual std::string finalizeDocument(const std::string &s) { return s; }

private:
    void init();
};

} // namespace
} // namespace

#endif

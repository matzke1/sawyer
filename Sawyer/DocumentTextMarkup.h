#ifndef Sawyer_Document_TextMarkup_H
#define Sawyer_Document_TextMarkup_H

#include <Sawyer/DocumentBaseMarkup.h>

namespace Sawyer {
namespace Document {

/** Renders markup as plain text.
 *
 *  This isn't fancy--it's got just enough smarts so a user can read the documentation when a more capable formatter like
 *  perldoc is not available. */
class TextMarkup: public BaseMarkup {
public:
    TextMarkup() { init(); }

private:
    void init();
    std::string finalizeDocument(const std::string &s_);
};

} // namespace
} // namespace

#endif

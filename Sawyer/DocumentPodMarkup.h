#ifndef Sawyer_Document_PodMarkup_H
#define Sawyer_Document_PodMarkup_H

#include <Sawyer/DocumentBaseMarkup.h>

namespace Sawyer {
namespace Document {

class PodMarkup: public BaseMarkup {
public:
    PodMarkup() { init(); }

private:
    void init();
    virtual std::string finalizeDocument(const std::string&s) /*override*/;
};

} // namespace
} // namespace

#endif

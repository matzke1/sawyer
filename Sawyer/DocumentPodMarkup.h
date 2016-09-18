#ifndef Sawyer_Document_PodMarkup_H
#define Sawyer_Document_PodMarkup_H

#include <Sawyer/DocumentBaseMarkup.h>

namespace Sawyer {
namespace Document {

class SAWYER_EXPORT PodMarkup: public BaseMarkup {
public:
    PodMarkup() { init(); }

    virtual void emit(const std::string &doc) /*override*/;

private:
    void init();
    virtual std::string finalizeDocument(const std::string&s) /*override*/;
};

} // namespace
} // namespace

#endif

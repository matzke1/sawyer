#include <Sawyer/DocumentBaseMarkup.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/foreach.hpp>

namespace Sawyer {
namespace Document {

// Function with one required argument
class Fr1: public Markup::Function {
protected:
    Fr1(const std::string &name): Markup::Function(name) {}
public:
    static Ptr instance(const std::string &name) {
        return Ptr(new Fr1(name))->arg("arg1");
    }
    std::string eval(const BaseMarkup::Grammar &grammar, const std::vector<std::string> &args) {
        ASSERT_require(args.size() == 1);
        throw Markup::SyntaxError("function \"" + name() + "\" should have been implemented in a subclass");
    }
};

// Function with two required arguments
class Fr2: public Markup::Function {
protected:
    Fr2(const std::string &name): Markup::Function(name) {}
public:
    static Ptr instance(const std::string &name) {
        return Ptr(new Fr2(name))->arg("arg1")->arg("arg2");
    }
    std::string eval(const BaseMarkup::Grammar &grammar, const std::vector<std::string> &args) {
        ASSERT_require(args.size() == 2);
        throw Markup::SyntaxError("function \"" + name() + "\" should have been implemented in a subclass");
    }
};

void
BaseMarkup::init() {
    with(Fr1::instance("b"));                           // @b{bold text}
    with(Fr1::instance("bullet"));                      // @bullet{body}
    with(Fr1::instance("b"));                           // @c{line oriented code}
    with(Fr2::instance("named"));                       // @named{item}{body}
    with(Fr1::instance("numbered"));                    // @numbered{body}
    with(Fr2::instance("section"));                     // @section{title}{body}
    with(Fr1::instance("v"));                           // @v{variable}
}

std::string
BaseMarkup::operator()(const std::string &s) {
    return finalizeDocument(Markup::Grammar::operator()(s));
}

// class method
bool
BaseMarkup::hasNonSpace(const std::string &s) {
    return !boost::trim_copy(s).empty();
}

std::string
BaseMarkup::makeOneLine(const std::string &s) {
    std::string retval;
    BOOST_FOREACH (char ch, s) {
        if ('\n' != ch && '\r' != ch && '\f' != ch)
            retval += ch;
    }
    return retval;
}

std::string
BaseMarkup::leftJustify(const std::string &s, size_t width) {
    return s.size() >=width ? s : s + std::string(width-s.size(), ' ');
}

} // namespace
} // namespace

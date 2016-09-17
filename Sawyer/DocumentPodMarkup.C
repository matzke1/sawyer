#include <Sawyer/DocumentPodMarkup.h>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>

namespace Sawyer {
namespace Document {

namespace {

// Escape characters that are special to pod
static std::string
podEscape(const std::string &s) {
    std::string retval;
    BOOST_FOREACH (char ch, s) {
        if ('<' == ch) {
            retval += "E<lt>";
        } else if ('>' == ch) {
            retval += "E<gt>";
        } else if ('|' == ch) {
            retval += "E<verbar>";
        } else if ('/' == ch) {
            retval += "E<sol>";
        } else {
            retval += ch;
        }
    }
    return retval;
}

// To override BaseMarkup's @section{title}{body}
class Section: public Markup::Function {
protected:
    Section(const std::string &name): Markup::Function(name) {}

public:
    static Ptr instance(const std::string &name) {
        return Ptr(new Section(name))->arg("title")->arg("body");
    }

    std::string eval(const Markup::Grammar&, const std::vector<std::string> &args) {
        ASSERT_require(args.size() == 2);

        // Do not produce a section if the body would be empty.
        if (!BaseMarkup::hasNonSpace(BaseMarkup::unescape(args[1])))
            return "";

        return "\n\n=head1 " + BaseMarkup::makeOneLine(args[0]) + "\n\n" + args[1] + "\n\n";
    }
};

// To override BaseMarkup's @named{item}{body}
class NamedItem: public Markup::Function {
protected:
    NamedItem(const std::string &name): Markup::Function(name) {}

public:
    static Ptr instance(const std::string &name) {
        return Ptr(new NamedItem(name))->arg("item")->arg("body");
    }

    std::string eval(const Markup::Grammar&, const std::vector<std::string> &args) {
        ASSERT_require(args.size() == 2);
        return ("\n\n"
                "=over\n\n"
                "=item Z<>" + BaseMarkup::makeOneLine(args[0]) + "\n\n" +
                args[1] + "\n\n"
                "=back\n\n");
    }
};

// To override BaseMarkup's @bullet{body} and @numbered{body}
class NumberedItem: public Markup::Function {
    std::string format_;                                // "*" or "1"
protected:
    NumberedItem(const std::string &name, const std::string &format)
        : Markup::Function(name), format_(format) {}

public:
    static Ptr instance(const std::string &name, const std::string &format) {
        return Ptr(new NumberedItem(name, format))->arg("body");
    }

    std::string eval(const Markup::Grammar&, const std::vector<std::string> &args) {
        ASSERT_require(args.size() == 1);
        return ("\n\n"
                "=over\n\n"
                "=item " + format_ + "\n\n" +
                args[0] + "\n\n"
                "=back\n\n");
    }
};

// To override BaseMarkup's @v{variable}
class InlineFormat: public Markup::Function {
    std::string format_;                                // I, B, C, L, E, F, S, X, Z
protected:
    InlineFormat(const std::string &name, const std::string &format)
        : Markup::Function(name), format_(format) {}
public:
    static Ptr instance(const std::string &name, const std::string &format) {
        return Ptr(new InlineFormat(name, format))->arg("body"); 
    }
    std::string eval(const Markup::Grammar&, const std::vector<std::string> &args) {
        ASSERT_require(args.size() == 1);
        return format_ + "<" + podEscape(args[0]) + ">";
    }
};

} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
PodMarkup::init() {
    with(InlineFormat::instance("b", "B"));             // bold
    with(NumberedItem::instance("bullet", "*"));
    with(InlineFormat::instance("c", "C"));             // code
    with(NamedItem::instance("named"));
    with(NumberedItem::instance("numbered", "1"));
    with(Section::instance("section"));
    with(InlineFormat::instance("v", "I"));;            // variable
}

std::string
PodMarkup::finalizeDocument(const std::string &s_) {
    std::string s = "=pod\n\n" + s_ + "\n\n=cut\n";

    // Remove pairs of "=back =over" and spaces at the ends of lines
    {
        boost::regex backOverRe("(^=back\\s*=over)|[ \\t\\r\\f]+$");
        std::ostringstream out(std::ios::out | std::ios::binary);
        std::ostream_iterator<char, char> oi(out);
        boost::regex_replace(oi, s.begin(), s.end(), backOverRe, "");
        s = out.str();
    }

    // Replace lots of blank lines with just one blank line
    {
        boost::regex blankLines("\\n{3,}");
        std::ostringstream out(std::ios::out | std::ios::binary);
        std::ostream_iterator<char, char> oi(out);
        boost::regex_replace(oi, s.begin(), s.end(), blankLines, "\n\n");
        s = out.str();
    }

    return s;
}

} // namespace
} // namespace

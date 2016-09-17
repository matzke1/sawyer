#include <Sawyer/DocumentTextMarkup.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>

namespace Sawyer {
namespace Document {

namespace {

// This text-based markup is similar to Perl POD but be careful: it's not the same:
//
//   Blank lines around "=foo" directives are significant, although the linefeed at the end of "=foo" lines is eaten.
//
//   Consecutive occurrences of =back/=over are removed since they would have no effect. Any white space between these
//   two lines is removed (as well as the linefeeds at the ends of these two lines).
//
//   The =item directive is an implied =over and must therefore end with a =back
//
//   The =item directive is not valid for named lists.  Instead, named list are created by emitting the name, a line break, and
//   an extra level of =over/=back for the body.
//
//   The =line directive means force a line break, otherwise isolated linefeeds are treated as spaces. Two or more consecutive
//   linefeeds will generate a blank line in the output.
//
//   There are no inline markups like I<text> and B<text> since we can't do Italics or bold faces. Perhaps a future version can
//   use ANSI escapes to accomplish this.


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

        return ("\n\n" + BaseMarkup::makeOneLine(args[0]) + "\n\n"
                "=over\n" +
                args[1] + "\n\n"
                "=back\n\n");
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
                "=over\n" +
                BaseMarkup::makeOneLine(args[0]) + "\n"
                "=line\n"
                "=over\n" +
                args[1] + "\n"
                "=back\n"
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
                "=over\n"
                "=item " + format_ + "\n" +
                args[0] + "\n"
                "=back\n"                               // this one is for the =over implied by =item
                "=back\n\n");
    }
};

// To handle variables, etc.
class Surround: public Markup::Function {
    std::string left_, right_;
protected:
    Surround(const std::string &name, const std::string &left, const std::string &right)
        : Markup::Function(name), left_(left), right_(right) {}
public:
    static Ptr instance(const std::string &name, const std::string &left, const std::string &right) {
        return Ptr(new Surround(name, left, right))->arg("body");
    }
    std::string eval(const Markup::Grammar&, const std::vector<std::string> &args) {
        ASSERT_require(args.size() == 1);
        return left_ + args[0] + right_;
    }
};


} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
TextMarkup::init() {
    with(Surround::instance("b", "*", "*"));            // bold
    with(NumberedItem::instance("bullet", "*"));
    with(Markup::Concat::instance("c"));                // code
    with(NamedItem::instance("named"));
    with(NumberedItem::instance("numbered", "1"));
    with(Section::instance("section"));
    with(Surround::instance("v", "<", ">"));;           // variable
}

std::string
TextMarkup::finalizeDocument(const std::string &s_) {
    std::string s = boost::trim_copy(s_);

    // Remove pairs of "=back =over" and spaces at the ends of lines
    {
        boost::regex backOverRe("(^=back\\s*=over[ \t]*)|[ \\t\\r\\f]+$");
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

    std::vector<std::string> lines;
    boost::split(lines, s, boost::is_any_of("\n"));

    // Indent and reflow the text since it tends to not have enough linefeeds.
    Markup::Reflow reflow(80);                          // standard tty width of 80 columns
    reflow.indentationString("    ");                   // four is good breathing room without looking stretched
    static const size_t itemFieldWidth = 3;             // one less than indentation
    std::vector<int> itemNumbers;                       // for numbered lists
    BOOST_FOREACH (const std::string &line, lines) {
        if (boost::starts_with(line, "=line")) {
            reflow.lineBreak();
        } else if (boost::starts_with(line, "=over")) {
            ++reflow;
            itemNumbers.push_back(0);
        } else if (boost::starts_with(line, "=back")) {
            --reflow;
            if (!itemNumbers.empty())
                itemNumbers.pop_back();
        } else if (boost::starts_with(line, "=item")) { // =item is an implied =over
            std::string style = boost::trim_copy(line.substr(5));
            if (style.empty()) {
                style = "*";
            } else if (isdigit(style[0]) && !itemNumbers.empty()) {
                style = boost::lexical_cast<std::string>(++itemNumbers.back()) + ".";
            }
            reflow(BaseMarkup::leftJustify(style, itemFieldWidth) + " ");
            ++reflow;
            itemNumbers.push_back(0);
        } else {
            reflow(line + "\n");
        }
    }
    return reflow.toString();
}

} // namespace
} // namespace

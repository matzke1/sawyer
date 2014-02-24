#include "MarkupRoff.h"
#include "Assert.h"

#include <cerrno>
#include <cstdio>

using namespace Sawyer;

int main(int argc, char *argv[]) {

    // Read the file into an STL string
    ASSERT_require2(argc==2, std::string("usage: ") + argv[0] + " FILENAME");
    FILE *f = fopen(argv[1], "r");
    ASSERT_not_null2(f, std::string(argv[1]) + ": " + strerror(errno));
    char *linebuf = NULL;
    size_t linebufsz = 0;
    std::string source;
    ssize_t nchars;
    while (0 < (nchars = getline(&linebuf, &linebufsz, f)))
        source += std::string(linebuf, nchars);
    
    // Parse the string
    Markup::ParserResult markup = Markup::Parser().parse(source);

    // Set up to generate nroff (man page) output
    Markup::RoffFormatterPtr nroff = Markup::RoffFormatter::instance("test10");
    nroff->version("0.99a", "February 2014");

    // Generate output
    markup.emit(std::cout, nroff);
}

    

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iostream>
#include <Sawyer/CommandLine.h>
#include <Sawyer/SlidingWindow.h>
#include <Sawyer/Histogram.h>

using namespace Sawyer;

enum Algorithm {
    ALGO_NONE,
    ALGO_UNIQUE_SYMBOLS,
    ALGO_UNIQUE_ASCII,
    ALGO_ASCII_RATIO
};

struct Settings {
    size_t symbolSize;                                  // size of symbol (bytes)
    size_t windowSize;                                  // size of sliding window (symbols)
    size_t reportInterval;                              // how often to report statistics (symbols)
    Algorithm algorithm;                                // which algorithm to use
    bool usePercent;                                    // show file position as a percent rather than an address?
    double positionScale;                               // multiply file position by this before emitting
    Settings()
        : symbolSize(1), windowSize(1024), reportInterval(1024), algorithm(ALGO_NONE), usePercent(false),
          positionScale(1.0) {}
};

class Symbol {
    std::vector<unsigned char> bytes_;
public:
    Symbol() {}
    explicit Symbol(const std::vector<unsigned char> &bytes): bytes_(bytes) {}
    bool operator<(const Symbol &other) const {
        ASSERT_require(bytes_.size()==other.bytes_.size());
        for (size_t i=0; i<bytes_.size(); ++i) {
            if (bytes_[i] < other.bytes_[i])
                return true;
            if (bytes_[i] > other.bytes_[i])
                return false;
        }
        return false;
    }
    bool isEmpty() const {
        return bytes_.empty();
    }
};

class Input {
    std::vector<std::string> fileNames_;
    std::ifstream in_;
    size_t at_;
public:
    Input(const std::vector<std::string> fileNames): fileNames_(fileNames), at_(0) {}
    int get() {
        while (1) {
            if (!in_.is_open()) {
                if (at_ < fileNames_.size()) {
                    in_.open(fileNames_[at_].c_str());
                    if (!in_.is_open())
                        throw std::runtime_error("cannot open file: " + fileNames_[at_]);
                } else {
                    return EOF;
                }
            }
            int c = in_.get();
            if (EOF!=c)
                return c;
            in_.close();
            ++at_;
        }
    }

    // FIXME[Robb Matzke 2014-06-24]: this isn't efficient, but it's portable
    size_t size() {
        size_t sz = 0;
        while (EOF!=get())
            ++sz;
        return sz;
    }
};

static CommandLine::ParserResult
parseCommandLine(int argc, char *argv[], Settings &settings) {
    using namespace CommandLine;
    SwitchGroup switches;
    switches.insert(Switch("help", 'h')
                    .action(showHelpAndExit(0))
                    .doc("Show this documentation."));
    switches.insert(Switch("symbol-size")
                    .argument("nbytes", nonNegativeIntegerParser(settings.symbolSize))
                    .doc("Size of each symbol in bytes.  The default is " +
                         boost::lexical_cast<std::string>(settings.symbolSize) + "."));
    switches.insert(Switch("window-size")
                    .argument("nsymbols", nonNegativeIntegerParser(settings.windowSize))
                    .doc("Size of sliding window in terms of symbols.  The default is " +
                         boost::lexical_cast<std::string>(settings.windowSize) + "."));
    switches.insert(Switch("report")
                    .argument("nsymbols", nonNegativeIntegerParser(settings.reportInterval))
                    .doc("Report statistics once every @v{nsymbols} of input.  The default is " +
                         boost::lexical_cast<std::string>(settings.reportInterval) + "."));
    switches.insert(Switch("algorithm", 'A')
                    .argument("algo", enumParser<Algorithm>(settings.algorithm)
                              ->with("unique-symbols", ALGO_UNIQUE_SYMBOLS)
                              ->with("unique-ascii", ALGO_UNIQUE_ASCII)
                              ->with("ascii-ratio", ALGO_ASCII_RATIO))
                    .doc("What algorithm to run. The @v{algo} must be one of the following:\n"
                         "@named{unique-symbols}{Number of unique symbols in the sliding window as a ratio of the total"
                         " number possible within the window.}\n"
                         "@named{unique-ascii}{Number of unique printable ASCII characters (or the NUL character) in the"
                         " sliding windows as a ratio of the total number possible within the window.}\n"
                         "@named{ascii-ratio}{The ratio of printable ASCII characters (or the NUL character) to other"
                         " bytes within the window.}"));
    switches.insert(Switch("percent")
                    .intrinsicValue(true, settings.usePercent)
                    .doc("Show file position as a percent (0 through 100) rather than a byte position."));

    Parser parser;
    parser.purpose("compute statistics over sliding window");
    parser.doc("synopsis", "@prop{programName} -A @v{algo} [@v{switches}] @v{files...}");
    parser.doc("description",
               "Reads the specified files and computes some value as a function of file position.  The value is computed "
               "over a sliding window at each byte of the file.  The size of the window and the reporting interval are "
               "controlled by switches.");
    
    return parser.with(switches).parse(argc, argv).apply();
}

static Symbol
getSymbol(Input &input, size_t nbytes) {
    std::vector<unsigned char> chars;
    for (size_t i=0; i<nbytes; ++i) {
        int c = input.get();
        if (EOF==c)
            return Symbol();
        chars.push_back((unsigned char)c);
    }
    return Symbol(chars);
}

static void
uniqueSymbols(Input &input, Settings &settings) {
    typedef Histogram<Symbol> Statistics;
    Container::SlidingWindow<Symbol, Statistics> window(settings.windowSize);

    // Maximum number of unique symbols per window
    double maxPossible = pow(256.0, (double)settings.symbolSize);
    if (settings.symbolSize>8 || maxPossible > settings.windowSize)
        maxPossible = settings.windowSize;

    size_t offset = 0;
    while (1) {
        Symbol symbol = getSymbol(input, settings.symbolSize);
        if (symbol.isEmpty())
            break;
        window.insert(symbol);
        if (0 == ++offset % settings.reportInterval && window.size()==settings.windowSize)
            std::cout <<(offset*settings.positionScale) <<"\t" <<(window.stats().nUnique()/maxPossible) <<"\n";
    }
}

class AsciiHistogram: public Histogram<unsigned char> {
public:
    typedef Histogram<unsigned char> Super;
    void insert(unsigned char ch) {
        if ('\0'==ch || isprint(ch))
            Super::insert(ch);
    }
    void erase(unsigned char ch) {
        if ('\0'==ch || isprint(ch))
            Super::erase(ch);
    }
};

static void
uniqueAscii(Input &input, Settings &settings) {
    if (settings.symbolSize != 1)
        throw std::runtime_error("unique ASCII must have a symbol size of 1 byte");

    Container::SlidingWindow<unsigned char, AsciiHistogram> window(settings.windowSize);

    // Maximum number of unique symbols per window
    size_t maxPossible = 1;
    for (size_t i=0; i<256; ++i) {
        if (isprint(i))
            ++maxPossible;
    }
    if (maxPossible > settings.windowSize)
        maxPossible = settings.windowSize;

    size_t offset = 0;
    while (1) {
        int c = input.get();
        if (EOF==c)
            break;
        window.insert((unsigned char)c);
        if (0 == ++offset % settings.reportInterval && window.size()==settings.windowSize)
            std::cout <<(offset*settings.positionScale) <<"\t" <<((double)window.stats().nUnique()/maxPossible) <<"\n";
    }
}

static void
asciiRatio(Input &input, Settings &settings) {
    if (settings.symbolSize != 1)
        throw std::runtime_error("unique ASCII must have a symbol size of 1 byte");

    typedef Histogram<bool> Statistics;
    Container::SlidingWindow<unsigned char, Statistics> window(settings.windowSize);
    size_t offset = 0;
    while (1) {
        int c = input.get();
        if (EOF==c)
            break;
        window.insert('\0'==c || isprint(c));
        if (0 == ++offset % settings.reportInterval && window.size()==settings.windowSize)
            std::cout <<(offset*settings.positionScale) <<"\t" <<((double)window.stats().count(true)/window.size()) <<"\n";
    }
}

int
main(int argc, char *argv[]) {
    Settings settings;
    CommandLine::ParserResult cmdline = parseCommandLine(argc, argv, settings);
    if (settings.usePercent) {
        size_t totalBytes = Input(cmdline.unreachedArgs()).size();
        settings.positionScale = 100.0 * settings.symbolSize / totalBytes;
    }
    Input input(cmdline.unreachedArgs());
    switch (settings.algorithm) {
        case ALGO_UNIQUE_ASCII:
            uniqueAscii(input, settings);
            break;
        case ALGO_UNIQUE_SYMBOLS:
            uniqueSymbols(input, settings);
            break;
        case ALGO_ASCII_RATIO:
            asciiRatio(input, settings);
            break;
        case ALGO_NONE:
            ASSERT_not_reachable("no algorithm specified; see --help");
    }
}

#include <Sawyer/Assert.h>
#include <Sawyer/Clexer.h>

#include <boost/algorithm/string/trim.hpp>
#include <iostream>

namespace Sawyer {
namespace Language {
namespace Clexer {

std::string
toString(TokenType tt) {
    switch (tt) {
        case TOK_EOF: return "eof";
        case TOK_LEFT: return "left";
        case TOK_RIGHT: return "right";
        case TOK_CHAR: return "char";
        case TOK_STRING: return "string";
        case TOK_NUMBER: return "number";
        case TOK_WORD: return "word";
        case TOK_OTHER: return "other";
    }
    ASSERT_not_reachable("invalid token type");
}

const Token&
TokenStream::operator[](size_t lookahead) {
    while (lookahead >= tokens_.size()) {
        makeNextToken();
        ASSERT_require(!tokens_.empty());
        if (tokens_.back().type() == TOK_EOF)
            return tokens_.back();
    }
    return tokens_[lookahead];
}

void
TokenStream::consume(size_t n) {
    if (n >= tokens_.size()) {
        tokens_.clear();
    } else {
        tokens_.erase(tokens_.begin(), tokens_.begin()+n);
    }
}

std::string
TokenStream::lexeme(const Token &t) const {
    const char *s = content_.characters(t.begin_);
    return std::string(s, t.end_-t.begin_);
}

std::string
TokenStream::toString(const Token &t) const {
    return Sawyer::Language::Clexer::toString(t.type()) + " " + lexeme(t);
}

std::string
TokenStream::line(const Token &t) const {
    if (t.type() == TOK_EOF)
        return "";
    size_t lineIdx = content_.lineIndex(t.begin_);
    const char *s = content_.lineChars(lineIdx);
    size_t n = content_.nCharacters(lineIdx);
    return std::string(s, n);
}

bool
TokenStream::matches(const Token &token, const char *s2) const {
    size_t n1 = token.end_ - token.begin_;
    size_t n2 = strlen(s2);
    if (n1 != n2)
        return false;
    const char *s1 = content_.characters(token.begin_);
    return 0 == strncmp(s1, s2, n1);
}

void
TokenStream::emit(const std::string &fileName, const Token &token, const std::string &message) const {
    std::pair<size_t, size_t> loc = content_.location(token.begin_);
    std::cerr <<fileName <<":" <<(loc.first+1) <<":" <<(loc.second+1) <<": " <<message <<"\n";
    const char *line = content_.lineChars(loc.first);
    if (line) {
        std::string str(line, content_.nCharacters(loc.first));
        boost::trim_right(str);
        std::cerr <<"        |" <<str <<"|\n";
        std::cerr <<"         " <<std::string(loc.second, ' ') <<"^\n";
    }
}

std::pair<size_t, size_t>
TokenStream::location(const Token &token) const {
    return content_.location(token.begin_);
}

void
TokenStream::scanString() {
    int q = content_.character(at_);
    ASSERT_require('\''==q || '"'==q);
    int c = content_.character(++at_);
    while (EOF != c && c != q) {
        if ('\\' == c)
            ++at_;                                  // skipping next char is sufficient
        c = content_.character(++at_);
    }
    ++at_;                                          // skip closing quote
}

void
TokenStream::makeNextToken() {
    if (!tokens_.empty() && tokens_.back().type() == TOK_EOF)
        return;
    while (isspace(content_.character(at_)))
           ++at_;
    int c = content_.character(at_);
    if (EOF == c) {
        tokens_.push_back(Token(TOK_EOF, at_, at_));
    } else if ('\'' == c || '"' == c) {
        size_t begin = at_;
        scanString();
        tokens_.push_back(Token('"'==c ? TOK_STRING : TOK_CHAR, begin, at_));
    } else if ('/' == c && '/' == content_.character(at_+1)) {
        at_ = content_.characterIndex(content_.lineIndex(at_) + 1);
        makeNextToken();
    } else if ('/' == c && '*' == content_.character(at_+1)) {
        at_ += 2;
        while (EOF != (c = content_.character(at_))) {
            if (content_.character(at_) == '*' && content_.character(at_+1) == '/') {
                at_ = at_ + 2;
                break;
            }
            ++at_;
        }
        makeNextToken();
    } else if (isalpha(c) || c=='_') {
        size_t begin = at_++;
        while (isalnum(c=content_.character(at_)) || '_'==c)
            ++at_;
        tokens_.push_back(Token(TOK_WORD, begin, at_));
    } else if ('('==c || '{'==c || '['==c) {
        ++at_;
        tokens_.push_back(Token(TOK_LEFT, at_-1, at_));
    } else if (')'==c || '}'==c || ']'==c) {
        ++at_;
        tokens_.push_back(Token(TOK_RIGHT, at_-1, at_));
    } else if (isdigit(c) || (('-'==c || '+'==c) && isdigit(content_.character(at_+1)))) {
        size_t begin = at_;
        if (!isdigit(c))
            ++at_;
        if ('0'==content_.character(at_) && 'x'==content_.character(at_+1)) {
            at_ += 2;
            while (isxdigit(content_.character(at_)))
                ++at_;
        } else if ('0'==content_.character(at_)) {
            ++at_;
            while ((c=content_.character(at_)) >= '0' && c <= '7')
                ++at_;
        } else {
            ++at_;
            while (isdigit(content_.character(at_)))
                ++at_;
        }
        tokens_.push_back(Token(TOK_NUMBER, begin, at_));
    } else if ('#' == c) {
        at_ = content_.characterIndex(content_.lineIndex(at_) + 1);
        while (at_>=2 && at_ < content_.nCharacters() && content_.character(at_-2)=='\\' && content_.character(at_-1)=='\n')
            at_ = content_.characterIndex(content_.lineIndex(at_) + 1);
        makeNextToken();
    } else {
        tokens_.push_back(Token(TOK_OTHER, at_, at_+1));
        ++at_;
    }
}

} // namespace
} // namespace
} // namespace

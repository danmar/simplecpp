/*
 * preprocessor library by daniel marjamäki
 */

#ifndef simplecppH
#define simplecppH

#include <istream>
#include <string>
#include <map>
#include <cctype>

namespace simplecpp {

typedef std::string TokenString;

struct Location {
    unsigned int file;
    unsigned int line;
    unsigned int col;
};

class Token {
public:
    Token(const TokenString &str, const Location &location) :
        str(str), location(location), previous(nullptr), next(nullptr)
    {
        flags();
    }

    Token(const Token &tok) :
        str(tok.str), macro(tok.macro), location(tok.location), previous(nullptr), next(nullptr)
    {
        flags();
    }

    void flags() {
        name = (str[0] == '_' || std::isalpha(str[0]));
        comment = (str[0] == '/');
        number = std::isdigit(str[0]);
        op = (str.size() == 1U) ? str[0] : '\0';
    }

    void setstr(const std::string &s) {
        str = s;
        flags();
    }

    char op;
    TokenString str;
    TokenString macro;
    bool comment;
    bool name;
    bool number;
    Location location;
    Token *previous;
    Token *next;
};

class TokenList {
public:
    TokenList();
    TokenList(const TokenList &other);
    ~TokenList();
    void operator=(const TokenList &other);

    void clear();
    void push_back(Token *token);

    void printOut() const;

    Token *begin() {
        return first;
    }

    const Token *cbegin() const {
        return first;
    }

    Token *end() {
        return last;
    }

    const Token *cend() const {
        return last;
    }

    void deleteToken(Token *tok) {
        if (!tok)
            return;
        Token *prev = tok->previous;
        Token *next = tok->next;
        if (prev)
            prev->next = next;
        if (next)
            next->previous = prev;
        if (first == tok)
            first = next;
        if (last == tok)
            last = prev;
        delete tok;
    }
private:
    Token *first;
    Token *last;
};

namespace Preprocessor {
TokenList readfile(std::istream &istr);
TokenList preprocess(const TokenList &rawtokens, const std::map<std::string,std::string> &defines);
}
}

#endif

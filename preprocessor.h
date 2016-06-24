/*
 * preprocessor library by daniel marjamäki
 */

#ifndef preprocessorH
#define preprocessorH

#include <string>
#include <map>
#include <cctype>

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
        str(tok.str), location(tok.location), previous(nullptr), next(nullptr)
    {
        flags();
    }

    void flags() {
        name = (str[0] == '_' || std::isalpha(str[0]));
        comment = (str[0] == '/');
        number = std::isdigit(str[0]);
        op = (str.size() == 1U) ? str[0] : '\0';
    }

    char op;
    TokenString str;
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
private:
    Token *first;
    Token *last;
};

TokenList readfile(std::istream &istr);
TokenList preprocess(const TokenList &rawtokens);



#endif

#ifndef PREPROCESSOR_HEADER_GUARD
#define PREPROCESSOR_HEADER_GUARD

#include <string>

struct Location {
    std::string file;
    unsigned int line;
    unsigned int col;
};

class Token {
public:
    Token(const std::string &str, const Location &location) :
        str(str), location(location), previous(nullptr), next(nullptr)
    {}

    Token(const Token &tok) :
        str(tok.str), location(tok.location), previous(nullptr), next(nullptr)
    {}

    std::string str;
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

    void dump() const;
private:
    Token *first;
    Token *last;
};

TokenList readfile(std::istream &istr, const std::string &filename);



#endif

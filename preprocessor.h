#ifndef PREPROCESSOR_HEADER_GUARD
#define PREPROCESSOR_HEADER_GUARD

#include <string>
#include <map>

struct Location {
    std::string file;
    unsigned int line;
    unsigned int col;
};

class Token {
public:
    Token(unsigned int str, bool isname, const Location &location) :
        str(str), isname(isname), location(location), previous(nullptr), next(nullptr)
    {}

    Token(const Token &tok) :
        str(tok.str), isname(tok.isname), location(tok.location), previous(nullptr), next(nullptr)
    {}

    unsigned int str;
    bool isname;
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

TokenList readfile(std::istream &istr, const std::string &filename, std::map<std::string, unsigned int> *stringlist);
TokenList preprocess(const TokenList &rawtokens);



#endif

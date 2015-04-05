#ifndef PREPROCESSOR_HEADER_GUARD
#define PREPROCESSOR_HEADER_GUARD

#include <string>
#include <map>

extern const unsigned int DEFINE;

struct Location {
    unsigned int file;
    unsigned int line;
    unsigned int col;
};

class Token {
public:
    Token(unsigned int str, const Location &location) :
        str(str), location(location), previous(nullptr), next(nullptr)
    {}

    Token(const Token &tok) :
        str(tok.str), location(tok.location), previous(nullptr), next(nullptr)
    {}

    static unsigned int encode(unsigned int index, bool isname, unsigned char strlen) {
        return index | (isname << 16U) | (strlen << 17U);
    }

    bool isname() const {
        return (str >> 16U) & 1U;
    }

    unsigned char strlen() const {
        return (str >> 17U) & 255U;
    }

    unsigned int str;
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

TokenList readfile(std::istream &istr, std::map<std::string, unsigned int> *stringlist);
TokenList preprocess(const TokenList &rawtokens);



#endif

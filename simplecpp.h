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
    Token(const TokenString &s, const Location &location) :
        str(string), location(location), previous(nullptr), next(nullptr), string(s)
    {
        flags();
    }

    Token(const Token &tok) :
        str(string), macro(tok.macro), location(tok.location), previous(nullptr), next(nullptr), string(tok.str)
    {
        flags();
    }

    void flags() {
        name = (str[0] == '_' || std::isalpha(str[0]));
        comment = (str[0] == '/');
        number = std::isdigit(str[0]) || (str.size() > 1U && str[0] == '-' && std::isdigit(str[1]));
        op = (str.size() == 1U) ? str[0] : '\0';
    }

    void setstr(const std::string &s) {
        string = s;
        flags();
    }

    char op;
    const TokenString &str;
    TokenString macro;
    bool comment;
    bool name;
    bool number;
    Location location;
    Token *previous;
    Token *next;
private:
    TokenString string;
};

class TokenList {
public:
    TokenList();
    TokenList(std::istringstream &istr);
    TokenList(const TokenList &other);
    ~TokenList();
    void operator=(const TokenList &other);

    void clear();
    bool empty() const {
        return !cbegin();
    }
    void push_back(Token *token);

    void dump() const;

    void readfile(std::istream &istr);
    void constFold();

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
    void combineOperators();

    void constFoldUnaryNotPosNeg(Token *tok);
    void constFoldMulDivRem(Token *tok);
    void constFoldAddSub(Token *tok);
    void constFoldComparison(Token *tok);
    void constFoldBitwise(Token *tok);
    void constFoldLogicalOp(Token *tok);
    void constFoldQuestionOp(Token *tok);

    Token *first;
    Token *last;
};

namespace Preprocessor {
TokenList preprocess(const TokenList &rawtokens, const std::map<std::string,std::string> &defines);
}
}

#endif

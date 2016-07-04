/*
 * preprocessor library by daniel marjamäki
 */

#ifndef simplecppH
#define simplecppH

#include <istream>
#include <string>
#include <map>
#include <cctype>
#include <list>

namespace simplecpp {

typedef std::string TokenString;

class Location {
public:
    Location() : line(1U), col(0U) {}

    std::string file;
    unsigned int line;
    unsigned int col;

    Location &operator=(const Location &other) {
        if (this != &other) {
            file = other.file;
            line = other.line;
            col  = other.col;
        }
        return *this;
    }

    void adjust(const std::string &str);
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

struct Output {
    enum Type {
        ERROR, /* error */
        WARNING /* warning */
    } type;
    Location location;
    std::string msg;
};

typedef std::list<struct Output> OutputList;

class TokenList {
public:
    TokenList();
    TokenList(std::istringstream &istr, const std::string &filename=std::string(), OutputList *outputList = 0);
    TokenList(const TokenList &other);
    ~TokenList();
    void operator=(const TokenList &other);

    void clear();
    bool empty() const {
        return !cbegin();
    }
    void push_back(Token *token);

    void dump() const;
    std::string stringify() const;

    void readfile(std::istream &istr, const std::string &filename=std::string(), OutputList *outputList = 0);
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

    std::string lastLine() const;

    Token *first;
    Token *last;
};

struct MacroUsage {
    std::string macroName;
    Location    macroLocation;
    Location    useLocation;
};

typedef std::map<std::string, std::string> Defines;
TokenList preprocess(const TokenList &rawtokens, const Defines &defines, OutputList *outputList = 0, std::list<struct MacroUsage> *macroUsage = 0);
}

#endif


#include <iostream>
#include <fstream>
#include <cctype>
#include <list>
#include <map>
#include <vector>

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

    std::string str;
    Location location;
    Token *previous;
    Token *next;
};

class TokenList {
public:
    TokenList() : first(nullptr), last(nullptr) {}
    TokenList(const TokenList &other) {
        *this = other;
    }
    ~TokenList() {
        clear();
    }
    void operator=(const TokenList &other) {
        if (this == &other)
            return;
        clear();
        for (const Token *tok = other.cbegin(); tok; tok = tok->next)
            push_back(tok->str, tok->location);
    }

    void clear() {
        while (first) {
            Token *next = first->next;
            delete first;
            first = next;
        }
        last = nullptr;
    }

    void push_back(const std::string &str, const Location &location) {
        Token *tok = new Token(str,location);
        if (!first)
            first = tok;
        else
            last->next = tok;
        tok->previous = last;
        last = tok;
    }

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

    void dump() const {
        for (const Token *tok = cbegin(); tok; tok = tok->next) {
            if (tok->previous && tok->previous->location.line != tok->location.line)
                std::cout << std::endl;
            std::cout << ' ' << tok->str;
        }
        std::cout << std::endl;
    }
private:
    Token *first;
    Token *last;
};

TokenList readfile(const std::string &filename)
{
    TokenList tokens;

    std::ifstream fin(filename);

    Location location;
    location.file = filename;
    location.line = 1U;
    location.col  = 0U;
    while (fin.good()) {
        unsigned char ch = (unsigned char)fin.get();
        if (!fin.good())
            break;
        location.col = (ch == '\t') ? ((location.col + 8) & (~7)) : (location.col + 1);

        if (ch == '\r' || ch == '\n') {
            if (ch == '\r' && fin.peek() == '\n')
                fin.get();
            ++location.line;
            location.col = 0;
            continue;
        }

        if (std::isspace(ch))
            continue;

        // number or name
        if (std::isalnum(ch) || ch == '_') {
            std::string currentToken;
            while (fin.good() && (std::isalnum(ch) || ch == '_')) {
                currentToken += ch;
                ch = (unsigned char)fin.get();
            }
            tokens.push_back(currentToken, location);
            location.col += currentToken.size() - 1U;
            fin.unget();
            continue;
        }

        // comment
        if (ch == '/' && fin.peek() == '/') {
            std::string currentToken;
            while (fin.good() && ch != '\r' && ch != '\n') {
                currentToken += ch;
                ch = (unsigned char)fin.get();
            }
            tokens.push_back(currentToken, location);
            location.col += currentToken.size() - 1U;
            fin.unget();
            continue;
        }

        // comment
        if (ch == '/' && fin.peek() == '*') {
            std::string currentToken;
            while (fin.good() && !(currentToken.size() > 2U && ch == '*' && fin.peek() == '/')) {
                currentToken += ch;
                ch = (unsigned char)fin.get();
            }
            tokens.push_back(currentToken, location);
            location.col--;
            fin.unget();
            continue;
        }

        // string / char literal
        if (ch == '\"' || ch == '\'') {
            std::string currentToken;
            do {
                currentToken += ch;
                ch = (unsigned char)fin.get();
                if (fin.good() && ch == '\\') {
                    currentToken += ch;
                    ch = (unsigned char)fin.get();
                    currentToken += ch;
                    ch = (unsigned char)fin.get();
                }
            } while (fin.good() && ch != '\"' && ch != '\'');
            currentToken += ch;
            tokens.push_back(currentToken, location);
            location.col += currentToken.size() - 1U;
            continue;
        }

        tokens.push_back(std::string(1U,ch), location);
    }

    return tokens;
}

class Macro {
public:
    Macro() : nameToken(nullptr) {}

    explicit Macro(const Token *tok) : nameToken(nullptr) {
        if (tok->previous && tok->previous->location.line == tok->location.line)
            throw 1;
        if (tok->str != "#")
            throw 1;
        tok = tok->next;
        if (!tok || tok->str != "define")
            throw 1;
        tok = tok->next;
        if (!tok || (!std::isalpha(tok->str[0]) && tok->str[0] != '_'))
            throw 1;
        parsedef(tok);
    }

    Macro(const Macro &macro) {
        *this = macro;
    }

    void operator=(const Macro &macro) {
        if (this != &macro)
            parsedef(macro.nameToken);
    }

    const Token * expand(TokenList * const output, const Token *tok, const std::map<std::string,Macro> &macros) const {
        if (args.empty()) {
            for (const Token *macro = valueToken; macro != endToken; macro = macro->next)
                output->push_back(macro->str, tok->location);
            return tok->next;
        }

        if (!tok->next || tok->next->str != "(") {
            std::cerr << "error: macro call" << std::endl;
            return tok->next;
        }

        // Parse macro-call
        std::vector<const Token*> parametertokens;
        parametertokens.push_back(tok->next);
        unsigned int par = 0U;
        for (const Token *calltok = tok->next->next; calltok; calltok = calltok->next) {
            if (calltok->str == "(")
                ++par;
            else if (calltok->str == ")") {
                if (par == 0U) {
                    parametertokens.push_back(calltok);
                    break;
                }
                --par;
            }
            else if (par == 0U && calltok->str == ",")
                parametertokens.push_back(calltok);
        }

        if (parametertokens.size() != args.size() + 1U) {
            std::cerr << "error: macro call" << std::endl;
            return tok->next;
        }

        // expand
        for (const Token *macro = valueToken; macro != endToken; macro = macro->next) {
            if (macro->str[0] == '_' || std::isalpha(macro->str[0])) {
                // Handling macro parameter..
                unsigned int par = 0;
                while (par < args.size()) {
                    if (macro->str == args[par])
                        break;
                    par++;
                }
                if (par < args.size()) {
                    for (const Token *partok = parametertokens[par]->next; partok != parametertokens[par+1]; partok = partok->next)
                        output->push_back(partok->str, tok->location);
                    continue;
                }
            }
            output->push_back(macro->str, tok->location);
        }

        return parametertokens[args.size()]->next;
    }

    std::string name() const {
        return nameToken->str;
    }

private:
    void parsedef(const Token *nametoken) {
        nameToken = nametoken;
        if (!nameToken) {
            valueToken = endToken = nullptr;
            args.clear();
            return;
        }

        // function like macro..
        if (nameToken->next &&
                nameToken->next->str == "(" &&
                nameToken->location.line == nameToken->next->location.line &&
                nameToken->next->location.col == nameToken->location.col + nameToken->str.size()) {
            args.clear();
            const Token *argtok = nameToken->next->next;
            while (argtok && argtok->str != ")") {
                if (argtok->str != ",")
                    args.push_back(argtok->str);
                argtok = argtok->next;
            }
            valueToken = argtok->next;
        } else {
            args.clear();
            valueToken = nameToken->next;
        }

        if (valueToken && valueToken->location.line != nameToken->location.line)
            valueToken = nullptr;
        endToken = valueToken;
        while (endToken && endToken->location.line == nameToken->location.line)
            endToken = endToken->next;
    }

    const Token *nameToken;
    std::vector<std::string> args;
    const Token *valueToken;
    const Token *endToken;
};

TokenList expandmacros(const TokenList &rawtokens)
{
    TokenList output;
    std::map<std::string, Macro> macros;
    for (const Token *rawtok = rawtokens.cbegin(); rawtok;) {
        if (rawtok->str == "#") {
            try {
                const Macro &macro = Macro(rawtok);
                macros[macro.name()] = macro;
                const unsigned int line = rawtok->location.line;
                while (rawtok && rawtok->location.line == line)
                    rawtok = rawtok->next;
                continue;
            } catch (...) {
            }
        }

        if (macros.find(rawtok->str) != macros.end()) {
            std::map<std::string,Macro>::const_iterator macro = macros.find(rawtok->str);
            if (macro != macros.end()) {
                rawtok = macro->second.expand(&output,rawtok,macros);
                continue;
            }
        }

        output.push_back(rawtok->str, rawtok->location);
        rawtok = rawtok->next;
    }
    return output;
}

int main(int argc, const char * const argv[]) {
    const TokenList rawtokens = readfile("1.c");
    const TokenList tokens = expandmacros(rawtokens);
    tokens.dump();
    return 0;
}

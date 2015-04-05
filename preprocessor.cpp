
#include "preprocessor.h"

#include <iostream>
#include <cctype>
#include <list>
#include <map>
#include <vector>


TokenList::TokenList() : first(nullptr), last(nullptr) {}

TokenList::TokenList(const TokenList &other) {
    *this = other;
}

TokenList::~TokenList() {
    clear();
}

void TokenList::operator=(const TokenList &other) {
    if (this == &other)
        return;
    clear();
    for (const Token *tok = other.cbegin(); tok; tok = tok->next)
        push_back(new Token(tok->str, tok->location));
}

void TokenList::clear() {
    while (first) {
        Token *next = first->next;
        delete first;
        first = next;
    }
    last = nullptr;
}

void TokenList::push_back(Token *tok) {
    if (!first)
        first = tok;
    else
        last->next = tok;
    tok->previous = last;
    last = tok;
}

TokenList readfile(std::istream &istr, const std::string &filename)
{
    TokenList tokens;
    Location location;
    location.file = filename;
    location.line = 1U;
    location.col  = 0U;
    while (istr.good()) {
        unsigned char ch = (unsigned char)istr.get();
        if (!istr.good())
            break;
        location.col = (ch == '\t') ? ((location.col + 8) & (~7)) : (location.col + 1);

        if (ch == '\r' || ch == '\n') {
            if (ch == '\r' && istr.peek() == '\n')
                istr.get();
            ++location.line;
            location.col = 0;
            continue;
        }

        if (std::isspace(ch))
            continue;

        // number or name
        if (std::isalnum(ch) || ch == '_') {
            std::string currentToken;
            while (istr.good() && (std::isalnum(ch) || ch == '_')) {
                currentToken += ch;
                ch = (unsigned char)istr.get();
            }
            tokens.push_back(new Token(currentToken, location));
            location.col += currentToken.size() - 1U;
            istr.unget();
            continue;
        }

        // comment
        if (ch == '/' && istr.peek() == '/') {
            std::string currentToken;
            while (istr.good() && ch != '\r' && ch != '\n') {
                currentToken += ch;
                ch = (unsigned char)istr.get();
            }
            tokens.push_back(new Token(currentToken, location));
            location.col += currentToken.size() - 1U;
            istr.unget();
            continue;
        }

        // comment
        if (ch == '/' && istr.peek() == '*') {
            std::string currentToken;
            while (istr.good() && !(currentToken.size() > 2U && ch == '*' && istr.peek() == '/')) {
                currentToken += ch;
                ch = (unsigned char)istr.get();
            }
            tokens.push_back(new Token(currentToken, location));
            location.col--;
            istr.unget();
            continue;
        }

        // string / char literal
        if (ch == '\"' || ch == '\'') {
            std::string currentToken;
            do {
                currentToken += ch;
                ch = (unsigned char)istr.get();
                if (istr.good() && ch == '\\') {
                    currentToken += ch;
                    ch = (unsigned char)istr.get();
                    currentToken += ch;
                    ch = (unsigned char)istr.get();
                }
            } while (istr.good() && ch != '\"' && ch != '\'');
            currentToken += ch;
            tokens.push_back(new Token(currentToken, location));
            location.col += currentToken.size() - 1U;
            continue;
        }

        tokens.push_back(new Token(std::string(1U,ch), location));
    }

    return tokens;
}

class Macro {
public:
    Macro() : nameToken(nullptr) {}

    explicit Macro(const Token *tok) : nameToken(nullptr) {
        if (tok->previous && tok->previous->location.line == tok->location.line)
            throw 1;
        if (tok->ch != '#')
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
                output->push_back(new Token(macro->str, tok->location));
            return tok->next;
        }

        if (!tok->next || tok->next->ch != '(') {
            std::cerr << "error: macro call" << std::endl;
            return tok->next;
        }

        // Parse macro-call
        std::vector<const Token*> parametertokens;
        parametertokens.push_back(tok->next);
        unsigned int par = 0U;
        for (const Token *calltok = tok->next->next; calltok; calltok = calltok->next) {
            if (calltok->ch == '(')
                ++par;
            else if (calltok->ch == ')') {
                if (par == 0U) {
                    parametertokens.push_back(calltok);
                    break;
                }
                --par;
            }
            else if (par == 0U && calltok->ch == ',')
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
                        output->push_back(new Token(partok->str, tok->location));
                    continue;
                }
            }
            output->push_back(new Token(macro->str, tok->location));
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
                nameToken->next->ch == '(' &&
                nameToken->location.line == nameToken->next->location.line &&
                nameToken->next->location.col == nameToken->location.col + nameToken->str.size()) {
            args.clear();
            const Token *argtok = nameToken->next->next;
            while (argtok && argtok->ch != ')') {
                if (argtok->ch != ',')
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

TokenList preprocess(const TokenList &rawtokens)
{
    TokenList output;
    std::map<std::string, Macro> macros;
    for (const Token *rawtok = rawtokens.cbegin(); rawtok;) {
        if (rawtok->ch == '#') {
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

        output.push_back(new Token(*rawtok));
        rawtok = rawtok->next;
    }
    return output;
}

/*
 * preprocessor library by daniel marjamäki
 */

#include "simplecpp.h"

#include <cctype>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <sstream>
#include <iostream>
#include <stack>

namespace {
const simplecpp::TokenString DEFINE("define");
const simplecpp::TokenString ERROR("error");
const simplecpp::TokenString WARNING("warning");
const simplecpp::TokenString IF("if");
const simplecpp::TokenString IFDEF("ifdef");
const simplecpp::TokenString IFNDEF("ifndef");
const simplecpp::TokenString DEFINED("defined");
const simplecpp::TokenString ELSE("else");
const simplecpp::TokenString ELIF("elif");
const simplecpp::TokenString ENDIF("endif");
const simplecpp::TokenString UNDEF("undef");

bool sameline(const simplecpp::Token *tok1, const simplecpp::Token *tok2) {
    return (tok1 && tok2 && tok1->location.line == tok2->location.line && tok1->location.file == tok2->location.file);
}
}

void simplecpp::Location::adjust(const std::string &str) {
    if (str.find_first_of("\r\n") == std::string::npos) {
        col += str.size() - 1U;
        return;
    }

    for (unsigned int i = 0U; i < str.size(); ++i) {
        col++;
        if (str[i] == '\n' || str[i] == '\r') {
            col = 0;
            line++;
            if (str[i] == '\r' && (i+1)<str.size() && str[i+1]=='\n')
                ++i;
        }
    }
}

simplecpp::TokenList::TokenList() : first(nullptr), last(nullptr) {}

simplecpp::TokenList::TokenList(std::istringstream &istr, const std::string &filename, OutputList *outputList)
    : first(nullptr), last(nullptr) {
    readfile(istr,filename,outputList);
}

simplecpp::TokenList::TokenList(const TokenList &other) : first(nullptr), last(nullptr) {
    *this = other;
}

simplecpp::TokenList::~TokenList() {
    clear();
}

void simplecpp::TokenList::operator=(const TokenList &other) {
    if (this == &other)
        return;
    clear();
    for (const Token *tok = other.cbegin(); tok; tok = tok->next)
        push_back(new Token(*tok));
}

void simplecpp::TokenList::clear() {
    while (first) {
        Token *next = first->next;
        delete first;
        first = next;
    }
    last = nullptr;
}

void simplecpp::TokenList::push_back(Token *tok) {
    if (!first)
        first = tok;
    else
        last->next = tok;
    tok->previous = last;
    last = tok;
}

void simplecpp::TokenList::dump() const {
    std::cout << stringify();
}

std::string simplecpp::TokenList::stringify() const {
    std::ostringstream ret;
    Location loc;
    for (const Token *tok = cbegin(); tok; tok = tok->next) {
        while (tok->location.line > loc.line) {
            ret << '\n';
            loc.line++;
        }

        if (sameline(tok->previous, tok))
            ret << ' ';

        ret << tok->str;

        loc.adjust(tok->str);
    }

    return ret.str();
}

void simplecpp::TokenList::readfile(std::istream &istr, const std::string &filename, OutputList *outputList)
{
    std::stack<simplecpp::Location> loc;

    unsigned int multiline = 0U;

    const Token *oldLastToken = nullptr;

    Location location;
    location.file = filename;
    location.line = 1U;
    location.col  = 0U;
    while (istr.good()) {
        unsigned char ch = (unsigned char)istr.get();
        if (!istr.good())
            break;
        location.col = (ch == '\t') ? ((location.col + 8U) & (~7U)) : (location.col + 1);

        if (ch == '\r' || ch == '\n') {
            if (ch == '\r' && istr.peek() == '\n')
                istr.get();
            if (cend() && cend()->op == '\\') {
                ++multiline;
                deleteToken(end());
            } else {
                location.line += multiline + 1;
                multiline = 0U;
            }
            location.col = 0;

            if (oldLastToken != cend()) {
                oldLastToken = cend();
                const std::string lastline(lastLine());

                if (lastline == "# file %str%") {
                    loc.push(location);
                    location.file = cend()->str.substr(1U, cend()->str.size() - 2U);
                    location.line = 1U;
                }

                // #endfile
                else if (lastline == "# endfile" && !loc.empty()) {
                    location = loc.top();
                    loc.pop();
                }
            }

            continue;
        }

        if (std::isspace(ch))
            continue;

        TokenString currentToken;

        // number or name
        if (std::isalnum(ch) || ch == '_') {
            while (istr.good() && (std::isalnum(ch) || ch == '_')) {
                currentToken += ch;
                ch = (unsigned char)istr.get();
            }
            istr.unget();
        }

        // comment
        else if (ch == '/' && istr.peek() == '/') {
            while (istr.good() && ch != '\r' && ch != '\n') {
                currentToken += ch;
                ch = (unsigned char)istr.get();
            }
            istr.unget();
        }

        // comment
        else if (ch == '/' && istr.peek() == '*') {
            currentToken = "/*";
            (void)istr.get();
            ch = (unsigned char)istr.get();
            while (istr.good()) {
                currentToken += ch;
                if (currentToken.size() >= 4U && currentToken.substr(currentToken.size() - 2U) == "*/")
                    break;
                ch = (unsigned char)istr.get();
            }
        }

        // string / char literal
        else if (ch == '\"' || ch == '\'') {
            do {
                currentToken += ch;
                ch = (unsigned char)istr.get();
                if (istr.good() && ch == '\\') {
                    currentToken += ch;
                    ch = (unsigned char)istr.get();
                    currentToken += ch;
                    ch = (unsigned char)istr.get();
                } else if (istr.good() && (ch == '\r' || ch == '\n')) {
                    clear();
                    if (outputList) {
                        Output err;
                        err.type = Output::ERROR;
                        err.location = location;
                        err.msg = "invalid " + std::string(currentToken[0] == '\'' ? "char" : "string") + " literal.";
                        outputList->push_back(err);
                    }
                    return;
                }
            } while (istr.good() && ch != '\"' && ch != '\'');
            currentToken += ch;
        }

        else {
            currentToken += ch;
        }

        push_back(new Token(currentToken, location));
        location.adjust(currentToken);
    }

    combineOperators();
}

void simplecpp::TokenList::constFold() {
    while (begin()) {
        // goto last '('
        Token *tok = end();
        while (tok && tok->op != '(')
            tok = tok->previous;

        // no '(', goto first token
        if (!tok)
            tok = begin();

        // Constant fold expression
        constFoldUnaryNotPosNeg(tok);
        constFoldMulDivRem(tok);
        constFoldAddSub(tok);
        constFoldComparison(tok);
        constFoldBitwise(tok);
        constFoldLogicalOp(tok);
        constFoldQuestionOp(tok);

        // If there is no '(' we are done with the constant folding
        if (tok->op != '(')
            break;

        if (!tok->next || !tok->next->next || tok->next->next->op != ')')
            break;

        tok = tok->next;
        deleteToken(tok->previous);
        deleteToken(tok->next);
    }
}

void simplecpp::TokenList::combineOperators() {
    for (Token *tok = begin(); tok; tok = tok->next) {
        if (tok->op == '\0' || !tok->next || tok->next->op == '\0')
            continue;
        if (std::strchr("=!<>", tok->op) && tok->next->op == '=') {
            tok->setstr(tok->str + "=");
            deleteToken(tok->next);
        } else if ((tok->op == '|' || tok->op == '&') && tok->op == tok->next->op) {
            tok->setstr(tok->str + tok->next->str);
            deleteToken(tok->next);
        }
    }
}

void simplecpp::TokenList::constFoldUnaryNotPosNeg(simplecpp::Token *tok) {
    for (; tok && tok->op != ')'; tok = tok->next) {
        if (tok->op == '!' && tok->next && tok->next->number) {
            tok->setstr(tok->next->str == "0" ? "1" : "0");
            deleteToken(tok->next);
        }
        else {
            if (tok->previous && (tok->previous->number || tok->previous->name))
                continue;
            if (!tok->next || !tok->next->number)
                continue;
            switch (tok->op) {
            case '+':
                tok->setstr(tok->next->str);
                deleteToken(tok->next);
                break;
            case '-':
                tok->setstr(tok->op + tok->next->str);
                deleteToken(tok->next);
                break;
            }
        }
    }
}

void simplecpp::TokenList::constFoldMulDivRem(Token *tok) {
    for (; tok && tok->op != ')'; tok = tok->next) {
        if (!tok->previous || !tok->previous->number)
            continue;
        if (!tok->next || !tok->next->number)
            continue;

        long long result;
        if (tok->op == '*')
            result = (std::stoll(tok->previous->str) * std::stoll(tok->next->str));
        else if (tok->op == '/')
            result = (std::stoll(tok->previous->str) / std::stoll(tok->next->str));
        else if (tok->op == '%')
            result = (std::stoll(tok->previous->str) % std::stoll(tok->next->str));
        else
            continue;

        tok->setstr(std::to_string(result));
        deleteToken(tok->previous);
        deleteToken(tok->next);
    }
}

void simplecpp::TokenList::constFoldAddSub(Token *tok) {
    for (; tok && tok->op != ')'; tok = tok->next) {
        if (!tok->previous || !tok->previous->number)
            continue;
        if (!tok->next || !tok->next->number)
            continue;

        long long result;
        if (tok->op == '+')
            result = (std::stoll(tok->previous->str) + std::stoll(tok->next->str));
        else if (tok->op == '-')
            result = (std::stoll(tok->previous->str) - std::stoll(tok->next->str));
        else
            continue;

        tok->setstr(std::to_string(result));
        deleteToken(tok->previous);
        deleteToken(tok->next);
    }
}

void simplecpp::TokenList::constFoldComparison(Token *tok) {
    for (; tok && tok->op != ')'; tok = tok->next) {
        if (!std::strchr("<>=!", tok->str[0]))
            continue;
        if (!tok->previous || !tok->previous->number)
            continue;
        if (!tok->next || !tok->next->number)
            continue;

        int result;
        if (tok->str == "==")
            result = (std::stoll(tok->previous->str) == std::stoll(tok->next->str));
        else if (tok->str == "!=")
            result = (std::stoll(tok->previous->str) != std::stoll(tok->next->str));
        else if (tok->str == ">")
            result = (std::stoll(tok->previous->str) > std::stoll(tok->next->str));
        else if (tok->str == ">=")
            result = (std::stoll(tok->previous->str) >= std::stoll(tok->next->str));
        else if (tok->str == "<")
            result = (std::stoll(tok->previous->str) < std::stoll(tok->next->str));
        else if (tok->str == "<=")
            result = (std::stoll(tok->previous->str) <= std::stoll(tok->next->str));
        else
            continue;

        tok->setstr(std::to_string(result));
        deleteToken(tok->previous);
        deleteToken(tok->next);
    }
}

void simplecpp::TokenList::constFoldBitwise(Token *tok)
{
    Token * const tok1 = tok;
    for (const char *op = "&^|"; *op; op++) {
        for (tok = tok1; tok && tok->op != ')'; tok = tok->next) {
            if (tok->op != *op)
                continue;
            if (!tok->previous || !tok->previous->number)
                continue;
            if (!tok->next || !tok->next->number)
                continue;
            int result;
            if (tok->op == '&')
                result = (std::stoll(tok->previous->str) & std::stoll(tok->next->str));
            else if (tok->op == '^')
                result = (std::stoll(tok->previous->str) ^ std::stoll(tok->next->str));
            else if (tok->op == '|')
                result = (std::stoll(tok->previous->str) | std::stoll(tok->next->str));
            tok->setstr(std::to_string(result));
            deleteToken(tok->previous);
            deleteToken(tok->next);
        }
    }
}

void simplecpp::TokenList::constFoldLogicalOp(Token *tok) {
    for (; tok && tok->op != ')'; tok = tok->next) {
        if (tok->str != "&&" && tok->str != "||")
            continue;
        if (!tok->previous || !tok->previous->number)
            continue;
        if (!tok->next || !tok->next->number)
            continue;

        int result;
        if (tok->str == "||")
            result = (std::stoll(tok->previous->str) || std::stoll(tok->next->str));
        else if (tok->str == "&&")
            result = (std::stoll(tok->previous->str) && std::stoll(tok->next->str));
        else
            continue;

        tok->setstr(std::to_string(result));
        deleteToken(tok->previous);
        deleteToken(tok->next);
    }
}

void simplecpp::TokenList::constFoldQuestionOp(Token *tok) {
    Token * const tok1 = tok;
    for (; tok && tok->op != ')'; tok = tok->next) {
        if (tok->str != "?")
            continue;
        if (!tok->previous || !tok->previous->number)
            continue;
        if (!tok->next)
            continue;
        if (!tok->next->next || tok->next->next->op != ':')
            continue;
        Token * const condTok = tok->previous;
        Token * const trueTok = tok->next;
        Token * const falseTok = trueTok->next->next;
        deleteToken(condTok->next); // ?
        deleteToken(trueTok->next); // :
        deleteToken(condTok->str == "0" ? trueTok : falseTok);
        deleteToken(condTok);
        tok = tok1;
    }
}

std::string simplecpp::TokenList::lastLine() const {
    std::string ret;
    for (const Token *tok = cend(); sameline(tok,cend()); tok = tok->previous) {
        if (tok->comment)
            continue;
        if (!ret.empty())
            ret = ' ' + ret;
        ret = (tok->str[0] == '\"' ? std::string("%str%") : tok->str) + ret;
    }
    return ret;
}

namespace simplecpp {
class Macro {
public:
    Macro() : nameToken(nullptr) {}

    explicit Macro(const Token *tok) : nameToken(nullptr) {
        if (sameline(tok->previous, tok))
            throw std::runtime_error("bad macro syntax");
        if (tok->op != '#')
            throw std::runtime_error("bad macro syntax");
        tok = tok->next;
        if (!tok || tok->str != DEFINE)
            throw std::runtime_error("bad macro syntax");
        tok = tok->next;
        if (!tok || !tok->name)
            throw std::runtime_error("bad macro syntax");
        parseDefine(tok);
    }

    explicit Macro(const std::string &name, const std::string &value) : nameToken(nullptr) {
        const std::string def(name + ' ' + value);
        std::istringstream istr(def);
        tokenListDefine.readfile(istr);
        parseDefine(tokenListDefine.cbegin());
    }

    Macro(const Macro &macro) {
        *this = macro;
    }

    void operator=(const Macro &macro) {
        if (this != &macro) {
            if (macro.tokenListDefine.empty())
                parseDefine(macro.nameToken);
            else {
                tokenListDefine = macro.tokenListDefine;
                parseDefine(tokenListDefine.cbegin());
            }
        }
    }

    const Token * expand(TokenList * const output, const Location &loc, const Token * const nameToken, const std::map<TokenString,Macro> &macros, std::set<TokenString> expandedmacros) const {
        const std::set<TokenString> expandedmacros1(expandedmacros);
        expandedmacros.insert(nameToken->str);

        usageList.push_back(loc);

        if (args.empty()) {
            Token * const token1 = output->end();
            for (const Token *macro = valueToken; macro != endToken;) {
                const std::map<TokenString, Macro>::const_iterator it = macros.find(macro->str);
                if (it != macros.end() && expandedmacros.find(macro->str) == expandedmacros.end()) {
                    macro = it->second.expand(output, loc, macro, macros, expandedmacros);
                } else {
                    output->push_back(newMacroToken(macro->str, loc, false));
                    macro = macro->next;
                }
            }
            setMacroName(output, token1, expandedmacros1);
            return nameToken->next;
        }

        // Parse macro-call
        const std::vector<const Token*> parametertokens(getMacroParameters(nameToken));
        if (parametertokens.size() != args.size() + 1U) {
            throw wrongNumberOfParameters(nameToken->location, name());
        }

        // expand
        for (const Token *tok = valueToken; tok != endToken;) {
            if (tok->op != '#') {
                tok = expandToken(output, loc, tok, macros, expandedmacros1, expandedmacros, parametertokens);
                continue;
            }

            tok = tok->next;
            if (tok->op == '#') {
                // A##B => AB
                Token *A = output->end();
                if (!A)
                    throw invalidHashHash(tok->location, name());
                tok = expandToken(output, loc, tok->next, macros, expandedmacros1, expandedmacros, parametertokens);
                Token *next = A->next;
                if (!next)
                    throw invalidHashHash(tok->location, name());
                A->setstr(A->str + A->next->str);
                A->flags();
                output->deleteToken(A->next);
            } else {
                // #123 => "123"
                TokenList tokenListHash;
                tok = expandToken(&tokenListHash, loc, tok, macros, expandedmacros1, expandedmacros, parametertokens);
                std::string s;
                for (const Token *hashtok = tokenListHash.cbegin(); hashtok; hashtok = hashtok->next)
                    s += hashtok->str;
                output->push_back(newMacroToken('\"' + s + '\"', loc, expandedmacros1.empty()));
            }
        }

        return parametertokens[args.size()]->next;
    }

    const TokenString &name() const {
        return nameToken->str;
    }

    const Location &defineLocation() const {
        return nameToken->location;
    }

    const std::list<Location> &usage() const {
        return usageList;
    }

    struct Error {
        Error(const Location &loc, const std::string &s) : location(loc), what(s) {}
        Location location;
        std::string what;
    };

    struct wrongNumberOfParameters : public Error {
        wrongNumberOfParameters(const Location &loc, const std::string &macroName) : Error(loc, "Syntax error. Wrong number of parameters for macro \'" + macroName + "\'.") {}
    };

    struct invalidHashHash : public Error {
        invalidHashHash(const Location &loc, const std::string &macroName) : Error(loc, "Syntax error. Invalid ## usage when expanding \'" + macroName + "\'.") {}
    };
private:
    Token *newMacroToken(const TokenString &str, const Location &loc, bool rawCode) const {
        Token *tok = new Token(str,loc);
        if (!rawCode)
            tok->macro = nameToken->str;
        return tok;
    }

    void setMacroName(TokenList *output, Token *token1, const std::set<std::string> &expandedmacros1) const {
        if (!expandedmacros1.empty())
            return;
        for (Token *tok = token1 ? token1->next : output->begin(); tok; tok = tok->next) {
            if (!tok->macro.empty())
                tok->macro = nameToken->str;
        }
    }

    void parseDefine(const Token *nametoken) {
        nameToken = nametoken;
        if (!nameToken) {
            valueToken = endToken = nullptr;
            args.clear();
            return;
        }

        // function like macro..
        if (nameToken->next &&
                nameToken->next->op == '(' &&
                sameline(nameToken, nameToken->next) &&
                nameToken->next->location.col == nameToken->location.col + nameToken->str.size()) {
            args.clear();
            const Token *argtok = nameToken->next->next;
            while (argtok && argtok->op != ')') {
                if (argtok->op != ',')
                    args.push_back(argtok->str);
                argtok = argtok->next;
            }
            valueToken = argtok->next;
        } else {
            args.clear();
            valueToken = nameToken->next;
        }

        if (!sameline(valueToken, nameToken))
            valueToken = nullptr;
        endToken = valueToken;
        while (sameline(endToken, nameToken))
            endToken = endToken->next;
    }

    unsigned int getArgNum(const TokenString &str) const {
        unsigned int par = 0;
        while (par < args.size()) {
            if (str == args[par])
                return par;
            par++;
        }
        return ~0U;
    }

    std::vector<const Token *> getMacroParameters(const Token *nameToken) const {
        if (!nameToken->next || nameToken->next->op != '(')
            return std::vector<const Token *>();

        std::vector<const Token *> parametertokens;
        parametertokens.push_back(nameToken->next);
        unsigned int par = 0U;
        for (const Token *tok = nameToken->next->next; tok; tok = tok->next) {
            if (tok->op == '(')
                ++par;
            else if (tok->op == ')') {
                if (par == 0U) {
                    parametertokens.push_back(tok);
                    break;
                }
                --par;
            }
            else if (par == 0U && tok->op == ',')
                parametertokens.push_back(tok);
        }
        return parametertokens;
    }

    const Token *expandToken(TokenList *output, const Location &loc, const Token *tok, const std::map<TokenString,Macro> &macros, std::set<TokenString> expandedmacros1, std::set<TokenString> expandedmacros, const std::vector<const Token*> &parametertokens) const {
        // Not name..
        if (!tok->name) {
            output->push_back(newMacroToken(tok->str, loc, false));
            return tok->next;
        }

        // Not macro parameter..
        const unsigned int par = getArgNum(tok->str);
        if (par >= args.size()) {
            // Macro..
            const std::map<TokenString, Macro>::const_iterator it = macros.find(tok->str);
            if (it != macros.end() && expandedmacros1.find(tok->str) == expandedmacros1.end())
                return it->second.expand(output, loc, tok, macros, expandedmacros);

            output->push_back(newMacroToken(tok->str, loc, false));
            return tok->next;
        }

        // Expand parameter..
        for (const Token *partok = parametertokens[par]->next; partok != parametertokens[par+1];) {
            const std::map<TokenString, Macro>::const_iterator it = macros.find(partok->str);
            if (it != macros.end() && expandedmacros1.find(partok->str) == expandedmacros1.end())
                partok = it->second.expand(output, loc, partok, macros, expandedmacros);
            else {
                output->push_back(newMacroToken(partok->str, loc, expandedmacros1.empty()));
                partok = partok->next;
            }
        }

        return tok->next;
    }

    void setMacro(Token *tok) const {
        while (tok) {
            if (!tok->macro.empty())
                tok->macro = nameToken->str;
            tok = tok->next;
        }
    }

    const Token *nameToken;
    std::vector<TokenString> args;
    const Token *valueToken;
    const Token *endToken;
    TokenList tokenListDefine;
    mutable std::list<Location> usageList;
};
}

namespace {
void simplifySizeof(simplecpp::TokenList &expr) {
    for (simplecpp::Token *tok = expr.begin(); tok; tok = tok->next) {
        if (tok->str != "sizeof")
            continue;
        simplecpp::Token *tok1 = tok->next;
        simplecpp::Token *tok2 = tok1->next;
        if (tok1->op == '(') {
            while (tok2->op != ')')
                tok2 = tok2->next;
            tok2 = tok2->next;
        }

        unsigned int sz = 0;
        for (simplecpp::Token *typeToken = tok1; typeToken != tok2; typeToken = typeToken->next) {
            if (typeToken->str == "char")
                sz = sizeof(char);
            if (typeToken->str == "short")
                sz = sizeof(short);
            if (typeToken->str == "int")
                sz = sizeof(int);
            if (typeToken->str == "long")
                sz = sizeof(long);
            if (typeToken->str == "float")
                sz = sizeof(float);
            if (typeToken->str == "double")
                sz = sizeof(double);
        }

        tok->setstr(std::to_string(sz));

        while (tok->next != tok2)
            expr.deleteToken(tok->next);
    }
}

void simplifyName(simplecpp::TokenList &expr) {
    for (simplecpp::Token *tok = expr.begin(); tok; tok = tok->next) {
        if (tok->name)
            tok->setstr("0");
    }
}

void simplifyNumbers(simplecpp::TokenList &expr) {
    for (simplecpp::Token *tok = expr.begin(); tok; tok = tok->next) {
        if (tok->str.size() == 1U)
            continue;
        if (tok->str.compare(0,2,"0x") == 0)
            tok->setstr(std::to_string(std::stoll(tok->str.substr(2), nullptr, 16)));
        else if (tok->str[0] == '\'')
            tok->setstr(std::to_string((unsigned char)tok->str[1]));
    }
}

int evaluate(simplecpp::TokenList expr) {
    simplifySizeof(expr);
    simplifyName(expr);
    simplifyNumbers(expr);
    expr.constFold();
    // TODO: handle invalid expressions
    return expr.cbegin() && expr.cbegin() == expr.cend() && expr.cbegin()->number ? std::stoi(expr.cbegin()->str) : 0;
}

const simplecpp::Token *gotoNextLine(const simplecpp::Token *tok) {
    const unsigned int line = tok->location.line;
    const std::string &file = tok->location.file;
    while (tok && tok->location.line == line && tok->location.file == file)
        tok = tok->next;
    return tok;
}
}

simplecpp::TokenList simplecpp::preprocess(const simplecpp::TokenList &rawtokens, const Defines &defines, OutputList *outputList, std::list<struct MacroUsage> *macroUsage)
{
    std::map<TokenString, Macro> macros;
    for (std::map<std::string,std::string>::const_iterator it = defines.begin(); it != defines.end(); ++it) {
        const Macro macro(it->first, it->second.empty() ? std::string("1") : it->second);
        macros[macro.name()] = macro;
    }

    // TRUE => code in current #if block should be kept
    // ELSE_IS_TRUE => code in current #if block should be dropped. the code in the #else should be kept.
    // ALWAYS_FALSE => drop all code in #if and #else
    enum IfState { TRUE, ELSE_IS_TRUE, ALWAYS_FALSE };
    std::stack<IfState> ifstates;
    ifstates.push(TRUE);

    TokenList output;
    for (const Token *rawtok = rawtokens.cbegin(); rawtok;) {
        if (rawtok->op == '#' && !sameline(rawtok->previous, rawtok)) {
            rawtok = rawtok->next;
            if (!rawtok || !rawtok->name)
                continue;

            if (ifstates.top() == TRUE && (rawtok->str == ERROR || rawtok->str == WARNING)) {
                if (outputList) {
                    simplecpp::Output err;
                    err.type = rawtok->str == ERROR ? Output::ERROR : Output::WARNING;
                    err.location = rawtok->location;
                    for (const Token *tok = rawtok->next; tok && sameline(rawtok,tok); tok = tok->next) {
                        if (!err.msg.empty() && std::isalnum(tok->str[0]))
                            err.msg += ' ';
                        err.msg += tok->str;
                    }
                    err.msg = '#' + rawtok->str + ' ' + err.msg;
                    outputList->push_back(err);
                }
                return TokenList();
            }

            if (rawtok->str == DEFINE) {
                if (ifstates.top() != TRUE)
                    continue;
                try {
                    const Macro &macro = Macro(rawtok->previous);
                    macros[macro.name()] = macro;
                } catch (const std::runtime_error &) {
                }
            } else if (rawtok->str == IF || rawtok->str == IFDEF || rawtok->str == IFNDEF || rawtok->str == ELIF) {
                bool conditionIsTrue;
                if (ifstates.top() == ALWAYS_FALSE)
                    conditionIsTrue = false;
                else if (rawtok->str == IFDEF)
                    conditionIsTrue = (macros.find(rawtok->next->str) != macros.end());
                else if (rawtok->str == IFNDEF)
                    conditionIsTrue = (macros.find(rawtok->next->str) == macros.end());
                else if (rawtok->str == IF || rawtok->str == ELIF) {
                    TokenList expr;
                    const Token * const endToken = gotoNextLine(rawtok);
                    for (const Token *tok = rawtok->next; tok != endToken; tok = tok->next) {
                        if (!tok->name) {
                            expr.push_back(new Token(*tok));
                            continue;
                        }

                        if (tok->str == DEFINED) {
                            tok = tok->next;
                            const bool par = (tok && tok->op == '(');
                            if (par)
                                tok = tok->next;
                            if (!tok)
                                break;
                            if (macros.find(tok->str) != macros.end())
                                expr.push_back(new Token("1", tok->location));
                            else
                                expr.push_back(new Token("0", tok->location));
                            if (tok && par)
                                tok = tok->next;
                            continue;
                        }

                        const std::map<std::string,Macro>::const_iterator it = macros.find(tok->str);
                        if (it != macros.end()) {
                            TokenList value;
                            std::set<TokenString> expandedmacros;
                            it->second.expand(&value, tok->location, tok, macros, expandedmacros);
                            for (const Token *tok2 = value.cbegin(); tok2; tok2 = tok2->next)
                                expr.push_back(new Token(tok2->str, tok->location));
                        } else {
                            expr.push_back(new Token(*tok));
                        }
                    }
                    conditionIsTrue = evaluate(expr);
                }

                if (rawtok->str != ELIF) {
                    // push a new ifstate..
                    if (ifstates.top() != TRUE)
                        ifstates.push(ALWAYS_FALSE);
                    else
                        ifstates.push(conditionIsTrue ? TRUE : ELSE_IS_TRUE);
                } else if (ifstates.top() == TRUE) {
                    ifstates.top() = ALWAYS_FALSE;
                } else if (ifstates.top() == ELSE_IS_TRUE && conditionIsTrue) {
                    ifstates.top() = TRUE;
                }
            } else if (rawtok->str == ELSE) {
                ifstates.top() = (ifstates.top() == ELSE_IS_TRUE) ? TRUE : ALWAYS_FALSE;
            } else if (rawtok->str == ENDIF) {
                if (ifstates.size() > 1U)
                    ifstates.pop();
            } else if (rawtok->str == UNDEF) {
                if (ifstates.top() == TRUE) {
                    const Token *tok = rawtok->next;
                    while (sameline(rawtok,tok) && tok->comment)
                        tok = tok->next;
                    if (sameline(rawtok, tok))
                        macros.erase(tok->str);
                }
            }
            rawtok = gotoNextLine(rawtok);
            if (!rawtok)
                break;
            continue;
        }

        if (ifstates.top() != TRUE) {
            // drop code
            rawtok = gotoNextLine(rawtok);
            continue;
        }

        if (macros.find(rawtok->str) != macros.end()) {
            std::map<TokenString,Macro>::const_iterator macro = macros.find(rawtok->str);
            if (macro != macros.end()) {
                std::set<TokenString> expandedmacros;
                try {
                    rawtok = macro->second.expand(&output,rawtok->location,rawtok,macros,expandedmacros);
                } catch (const simplecpp::Macro::Error &err) {
                    Output out;
                    out.type = Output::ERROR;
                    out.location = err.location;
                    out.msg = err.what;
                    if (outputList)
                        outputList->push_back(out);
                    return TokenList();
                }
                continue;
            }
        }

        if (!rawtok->comment)
            output.push_back(new Token(*rawtok));
        rawtok = rawtok->next;
    }

    if (macroUsage) {
        for (std::map<TokenString, simplecpp::Macro>::const_iterator macroIt = macros.begin(); macroIt != macros.end(); ++macroIt) {
            const Macro &macro = macroIt->second;
            const std::list<Location> &usage = macro.usage();
            for (std::list<Location>::const_iterator usageIt = usage.begin(); usageIt != usage.end(); ++usageIt) {
                struct MacroUsage mu;
                mu.macroName = macro.name();
                mu.macroLocation = macro.defineLocation();
                mu.useLocation = *usageIt;
                macroUsage->push_back(mu);
            }
        }
    }

    return output;
}

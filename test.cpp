
#include <iostream>
#include <sstream>
#include "simplecpp.h"

#define ASSERT_EQUALS(expected, actual)  assertEquals((expected), (actual), __LINE__);

static int assertEquals(const std::string &expected, const std::string &actual, int line) {
    std::cerr << "line " << line << ": Assertion " << ((expected == actual) ? "success" : "failed") << std::endl;
    if (expected != actual) {
        std::cerr << "------ assertion failed ---------" << std::endl;
        std::cerr << "expected:" << expected << std::endl;
        std::cerr << "actual:" << actual << std::endl;
    }
    return (expected == actual);
}

static std::string stringify(const simplecpp::TokenList &tokens) {
    std::ostringstream out;

    for (const simplecpp::Token *tok = tokens.cbegin(); tok; tok = tok->next) {
        if (tok->previous && tok->previous->location.line != tok->location.line)
            out << '\n';
        else if (tok->previous)
            out << ' ';
        out << tok->str;
    }

    return out.str();
}

static std::string readfile(const char code[]) {
    std::istringstream istr(code);
    return stringify(simplecpp::TokenList(istr));
}

static std::string preprocess(const char code[], const std::map<std::string,std::string> &defines) {
    std::istringstream istr(code);
    return stringify(simplecpp::Preprocessor::preprocess(simplecpp::TokenList(istr),defines));
}

static std::string preprocess(const char code[]) {
    std::map<std::string,std::string> nodefines;
    return preprocess(code,nodefines);
}

static std::string testConstFold(const char code[]) {
    std::istringstream istr(code);
    simplecpp::TokenList expr(istr);
    expr.constFold();
    return stringify(expr);
}

void comment() {
    const char code[] = "// abc";
    ASSERT_EQUALS("// abc",
                  readfile(code));
    ASSERT_EQUALS("// abc",
                  preprocess(code));
}

static void constFold() {
    ASSERT_EQUALS("7", testConstFold("1+2*3"));
    ASSERT_EQUALS("15", testConstFold("1+2*(3+4)"));
}

void define1() {
    const char code[] = "#define A 1+2\n"
                        "a=A+3;";
    ASSERT_EQUALS("# define A 1 + 2\n"
                  "a = A + 3 ;",
                  readfile(code));
    ASSERT_EQUALS("a = 1 + 2 + 3 ;",
                  preprocess(code));
}

void define2() {
    const char code[] = "#define ADD(A,B) A+B\n"
                        "ADD(1+2,3);";
    ASSERT_EQUALS("# define ADD ( A , B ) A + B\n"
                  "ADD ( 1 + 2 , 3 ) ;",
                  readfile(code));
    ASSERT_EQUALS("1 + 2 + 3 ;",
                  preprocess(code));
}

void define3() {
    const char code[] = "#define A   123\n"
                        "#define B   A\n"
                        "A B";
    ASSERT_EQUALS("# define A 123\n"
                  "# define B A\n"
                  "A B",
                  readfile(code));
    ASSERT_EQUALS("123 123",
                  preprocess(code));
}

void define4() {
    const char code[] = "#define A      123\n"
                        "#define B(C)   A\n"
                        "A B(1)";
    ASSERT_EQUALS("# define A 123\n"
                  "# define B ( C ) A\n"
                  "A B ( 1 )",
                  readfile(code));
    ASSERT_EQUALS("123 123",
                  preprocess(code));
}

void define5() {
    const char code[] = "#define add(x,y) x+y\n"
                        "add(add(1,2),3)";
    ASSERT_EQUALS("1 + 2 + 3", preprocess(code));
}

void hash() {
    const char code[] = "#define a(x) #x\n"
                        "a(1)\n"
                        "a(2+3)";
    ASSERT_EQUALS("\"1\"\n"
                  "\"2+3\"", preprocess(code));
}

void hashhash() { // #4703
    const char code[] = "#define MACRO( A, B, C ) class A##B##C##Creator {};\n"
                        "MACRO( B\t, U , G )";
    ASSERT_EQUALS("class BUGCreator { } ;", preprocess(code));
}

void ifdef1() {
    const char code[] = "#ifdef A\n"
                        "1\n"
                        "#else\n"
                        "2\n"
                        "#endif";
    ASSERT_EQUALS("2", preprocess(code));
}

void ifdef2() {
    const char code[] = "#define A\n"
                        "#ifdef A\n"
                        "1\n"
                        "#else\n"
                        "2\n"
                        "#endif";
    ASSERT_EQUALS("1", preprocess(code));
}

void ifA() {
    const char code[] = "#if A==1\n"
                        "X\n"
                        "#endif";
    ASSERT_EQUALS("", preprocess(code));

    std::map<std::string,std::string> defines;
    defines["A"] = "1";
    ASSERT_EQUALS("X", preprocess(code, defines));
}

void ifDefined() {
    const char code[] = "#if defined(A)\n"
                        "X\n"
                        "#endif";
    std::map<std::string, std::string> defs;
    ASSERT_EQUALS("", preprocess(code, defs));
    defs["A"] = "1";
    ASSERT_EQUALS("X", preprocess(code, defs));
}

void ifLogical() {
    const char code[] = "#if defined(A) || defined(B)\n"
                        "X\n"
                        "#endif";
    std::map<std::string, std::string> defs;
    ASSERT_EQUALS("", preprocess(code, defs));
    defs.clear();
    defs["A"] = "1";
    ASSERT_EQUALS("X", preprocess(code, defs));
    defs.clear();
    defs["B"] = "1";
    ASSERT_EQUALS("X", preprocess(code, defs));
}

void ifSizeof() {
    const char code[] = "#if sizeof(unsigned short)==2\n"
                        "X\n"
                        "#else\n"
                        "Y\n"
                        "#endif";
    ASSERT_EQUALS("X", preprocess(code));
}

void tokenMacro1() {
    const char code[] = "#define A 123\n"
                        "A";
    std::map<std::string, std::string> nodefines;
    std::istringstream istr(code);
    const simplecpp::TokenList tokenList(simplecpp::Preprocessor::preprocess(simplecpp::TokenList(istr), nodefines));
    ASSERT_EQUALS("A", tokenList.cend()->macro);
}

void tokenMacro2() {
    const char code[] = "#define ADD(X,Y) X+Y\n"
                        "ADD(1,2)";
    std::map<std::string, std::string> nodefines;
    std::istringstream istr(code);
    const simplecpp::TokenList tokenList(simplecpp::Preprocessor::preprocess(simplecpp::TokenList(istr), nodefines));
    const simplecpp::Token *tok = tokenList.cbegin();
    ASSERT_EQUALS("1", tok->str);
    ASSERT_EQUALS("", tok->macro);
    tok = tok->next;
    ASSERT_EQUALS("+", tok->str);
    ASSERT_EQUALS("ADD", tok->macro);
    tok = tok->next;
    ASSERT_EQUALS("2", tok->str);
    ASSERT_EQUALS("", tok->macro);
}

void tokenMacro3() {
    const char code[] = "#define ADD(X,Y) X+Y\n"
                        "#define FRED  1\n"
                        "ADD(FRED,2)";
    std::map<std::string, std::string> nodefines;
    std::istringstream istr(code);
    const simplecpp::TokenList tokenList(simplecpp::Preprocessor::preprocess(simplecpp::TokenList(istr), nodefines));
    const simplecpp::Token *tok = tokenList.cbegin();
    ASSERT_EQUALS("1", tok->str);
    ASSERT_EQUALS("FRED", tok->macro);
    tok = tok->next;
    ASSERT_EQUALS("+", tok->str);
    ASSERT_EQUALS("ADD", tok->macro);
    tok = tok->next;
    ASSERT_EQUALS("2", tok->str);
    ASSERT_EQUALS("", tok->macro);
}

void tokenMacro4() {
    const char code[] = "#define A B\n"
                        "#define B 1\n"
                        "A";
    std::map<std::string, std::string> nodefines;
    std::istringstream istr(code);
    const simplecpp::TokenList tokenList(simplecpp::Preprocessor::preprocess(simplecpp::TokenList(istr), nodefines));
    const simplecpp::Token *tok = tokenList.cbegin();
    ASSERT_EQUALS("1", tok->str);
    ASSERT_EQUALS("A", tok->macro);
}

int main() {
    comment();
    constFold();
    define1();
    define2();
    define3();
    define4();
    define5();
    hash();
    hashhash();
    ifdef1();
    ifdef2();
    ifA();
    ifDefined();
    ifLogical();
    ifSizeof();
    tokenMacro1();
    tokenMacro2();
    tokenMacro3();
    tokenMacro4();
    return 0;
}


#include <iostream>
#include <sstream>
#include "preprocessor.h"

#define ASSERT_EQUALS(expected, actual)  assertEquals((expected), (actual), __LINE__);

static int assertEquals(const std::string &expected, const std::string &actual, int line) {
    std::cerr << "line " << line << ": Assertion " << ((expected == actual) ? "success" : "failed") << std::endl;
    if (expected != actual)
        std::cerr << "<<<" << actual << ">>>" << std::endl;
    return (expected == actual);
}

static std::string stringify(const TokenList &tokens) {
    std::ostringstream out;

    for (const Token *tok = tokens.cbegin(); tok; tok = tok->next) {
        if (tok->previous && tok->previous->location.line != tok->location.line)
            out << '\n';
        out << ' ' << tok->str;
    }

    return out.str();
}

static std::string readfile(const char code[]) {
    std::istringstream istr(code);
    return stringify(readfile(istr));
}

static std::string preprocess(const char code[]) {
    std::istringstream istr(code);
    return stringify(preprocess(readfile(istr)));
}


void comment() {
    const char code[] = "// abc";
    ASSERT_EQUALS(" // abc",
                  readfile(code));
    ASSERT_EQUALS(" // abc",
                  preprocess(code));
}

void define1() {
    const char code[] = "#define A 1+2\n"
                        "a=A+3;";
    ASSERT_EQUALS(" # define A 1 + 2\n"
                  " a = A + 3 ;",
                  readfile(code));
    ASSERT_EQUALS(" a = 1 + 2 + 3 ;",
                  preprocess(code));
}

void define2() {
    const char code[] = "#define ADD(A,B) A+B\n"
                        "ADD(1+2,3);";
    ASSERT_EQUALS(" # define ADD ( A , B ) A + B\n"
                  " ADD ( 1 + 2 , 3 ) ;",
                  readfile(code));
    ASSERT_EQUALS(" 1 + 2 + 3 ;",
                  preprocess(code));
}

void define3() {
    const char code[] = "#define A   123\n"
                        "#define B   A\n"
                        "A B";
    ASSERT_EQUALS(" # define A 123\n"
                  " # define B A\n"
                  " A B",
                  readfile(code));
    ASSERT_EQUALS(" 123 123",
                  preprocess(code));
}

void define4() {
    const char code[] = "#define A      123\n"
                        "#define B(C)   A\n"
                        "A B(1)";
    ASSERT_EQUALS(" # define A 123\n"
                  " # define B ( C ) A\n"
                  " A B ( 1 )",
                  readfile(code));
    ASSERT_EQUALS(" 123 123",
                  preprocess(code));
}

void ifdef1() {
    const char code[] = "#ifdef A\n"
                        "1\n"
                        "#else\n"
                        "2\n"
                        "#endif";
    ASSERT_EQUALS(" 2", preprocess(code));
}

void ifdef2() {
    const char code[] = "#define A\n"
                        "#ifdef A\n"
                        "1\n"
                        "#else\n"
                        "2\n"
                        "#endif";
    ASSERT_EQUALS(" 1", preprocess(code));
}

int main() {
    comment();
    define1();
    define2();
    define3();
    define4();
    ifdef1();
    ifdef2();
    return 0;
}

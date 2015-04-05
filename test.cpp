
#include <iostream>
#include <sstream>
#include "preprocessor.h"

#define ASSERT_EQUALS(expected, actual)  assertEquals((expected), (actual), __LINE__);

static int assertEquals(const std::string &expected, const std::string &actual, int line) {
    std::cerr << "line " << line << ": Assertion " << ((expected == actual) ? "success" : "failed") << std::endl;
    return (expected == actual);
}

static std::string preprocess(const char code[]) {
    std::istringstream istr(code);
    const TokenList tokens = readfile(istr, "test.cpp");
    std::ostringstream out;
    for (const Token *tok = tokens.cbegin(); tok; tok = tok->next) {
        if (tok->previous && tok->previous->location.line != tok->location.line)
            out << '\n';
        out << ' ' << tok->str;
    }
    return out.str();
}


void define1() {
    const char code[] = "#define A 1+2\n"
                        "a=A+3;";
    ASSERT_EQUALS(" # define A 1 + 2\n"
                  " a = A + 3 ;",
                  preprocess(code));
}


int main() {
    define1();
    return 0;
}

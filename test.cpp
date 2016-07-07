
#include <iostream>
#include <sstream>
#include "simplecpp.h"

#define ASSERT_EQUALS(expected, actual)  assertEquals((expected), (actual), __LINE__);

static int assertEquals(const std::string &expected, const std::string &actual, int line) {
    if (expected != actual) {
        std::cerr << "------ assertion failed ---------" << std::endl;
        std::cerr << "line " << line << std::endl;
        std::cerr << "expected:" << expected << std::endl;
        std::cerr << "actual:" << actual << std::endl;
    }
    return (expected == actual);
}

static int assertEquals(const unsigned int expected, const unsigned int actual, int line) {
    if (expected != actual) {
        std::cerr << "------ assertion failed ---------" << std::endl;
        std::cerr << "line " << line << std::endl;
        std::cerr << "expected:" << expected << std::endl;
        std::cerr << "actual:" << actual << std::endl;
    }
    return (expected == actual);
}

static std::string readfile(const char code[]) {
    std::istringstream istr(code);
    return simplecpp::TokenList(istr).stringify();
}

static std::string preprocess(const char code[], const std::map<std::string,std::string> &defines) {
    std::istringstream istr(code);
    return simplecpp::preprocess(simplecpp::TokenList(istr),defines).stringify();
}

static std::string preprocess(const char code[]) {
    std::map<std::string,std::string> nodefines;
    return preprocess(code,nodefines);
}

static std::string testConstFold(const char code[]) {
    std::istringstream istr(code);
    simplecpp::TokenList expr(istr);
    expr.constFold();
    return expr.stringify();
}

void comment() {
    ASSERT_EQUALS("// abc", readfile("// abc"));
    ASSERT_EQUALS("", preprocess("// abc"));
    ASSERT_EQUALS("/*\n\n*/abc", readfile("/*\n\n*/abc"));
    ASSERT_EQUALS("\n\nabc", preprocess("/*\n\n*/abc"));
    ASSERT_EQUALS("* p = a / * b / * c ;", readfile("*p=a/ *b/ *c;"));
    ASSERT_EQUALS("* p = a / * b / * c ;", preprocess("*p=a/ *b/ *c;"));
}

static void constFold() {
    ASSERT_EQUALS("7", testConstFold("1+2*3"));
    ASSERT_EQUALS("15", testConstFold("1+2*(3+4)"));
    ASSERT_EQUALS("123", testConstFold("+123"));
    ASSERT_EQUALS("1", testConstFold("-123<1"));
    ASSERT_EQUALS("6", testConstFold("14 & 7"));
    ASSERT_EQUALS("29", testConstFold("13 ^ 16"));
    ASSERT_EQUALS("25", testConstFold("24 | 1"));
    ASSERT_EQUALS("2", testConstFold("1?2:3"));
}

void define1() {
    const char code[] = "#define A 1+2\n"
                        "a=A+3;";
    ASSERT_EQUALS("# define A 1 + 2\n"
                  "a = A + 3 ;",
                  readfile(code));
    ASSERT_EQUALS("\na = 1 + 2 + 3 ;",
                  preprocess(code));
}

void define2() {
    const char code[] = "#define ADD(A,B) A+B\n"
                        "ADD(1+2,3);";
    ASSERT_EQUALS("# define ADD ( A , B ) A + B\n"
                  "ADD ( 1 + 2 , 3 ) ;",
                  readfile(code));
    ASSERT_EQUALS("\n1 + 2 + 3 ;",
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
    ASSERT_EQUALS("\n\n123 123",
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
    ASSERT_EQUALS("\n\n123 123",
                  preprocess(code));
}

void define5() {
    const char code[] = "#define add(x,y) x+y\n"
                        "add(add(1,2),3)";
    ASSERT_EQUALS("\n1 + 2 + 3", preprocess(code));
}

void define6() {
    const char code[] = "#define A() 1\n"
                        "A()";
    ASSERT_EQUALS("\n1", preprocess(code));
}

void define_define_1() {
    const char code[] = "#define A(x) (x+1)\n"
                        "#define B    A(\n"
                        "B(i))";
    ASSERT_EQUALS("\n\n( ( i ) + 1 )", preprocess(code));
}

void define_define_2() {
    const char code[] = "#define A(m)    n=m\n"
                        "#define B(x)    A(x)\n"
                        "B(0)";
    ASSERT_EQUALS("\n\nn = 0", preprocess(code));
}

void define_va_args_1() {
    const char code[] = "#define A(fmt...) dostuff(fmt)\n"
                        "A(1,2);";
    ASSERT_EQUALS("\ndostuff ( 1 , 2 ) ;", preprocess(code));
}

void define_va_args_2() {
    const char code[] = "#define A(X,...) X(#__VA_ARGS__)\n"
                        "A(f,123);";
    ASSERT_EQUALS("\nf ( \"123\" ) ;", preprocess(code));
}

void error() {
    std::istringstream istr("#error    hello world! \n");
    std::map<std::string, std::string> defines;
    std::list<simplecpp::Output> output;
    simplecpp::preprocess(simplecpp::TokenList(istr,"test.c"), defines, &output);
    ASSERT_EQUALS(simplecpp::Output::ERROR, output.front().type);
    ASSERT_EQUALS("test.c", output.front().location.file);
    ASSERT_EQUALS(1U, output.front().location.line);
    ASSERT_EQUALS("#error hello world!", output.front().msg);
}

void hash() {
    const char code[] = "#define a(x) #x\n"
                        "a(1)\n"
                        "a(2+3)";
    ASSERT_EQUALS("\n"
                  "\"1\"\n"
                  "\"2+3\"", preprocess(code));
}

void hashhash() { // #4703
    const char code[] = "#define MACRO( A, B, C ) class A##B##C##Creator {};\n"
                        "MACRO( B\t, U , G )";
    ASSERT_EQUALS("\nclass BUGCreator { } ;", preprocess(code));
}

void ifdef1() {
    const char code[] = "#ifdef A\n"
                        "1\n"
                        "#else\n"
                        "2\n"
                        "#endif";
    ASSERT_EQUALS("\n\n\n2", preprocess(code));
}

void ifdef2() {
    const char code[] = "#define A\n"
                        "#ifdef A\n"
                        "1\n"
                        "#else\n"
                        "2\n"
                        "#endif";
    ASSERT_EQUALS("\n\n1", preprocess(code));
}

void ifndef() {
    const char code1[] = "#define A\n"
                         "#ifndef A\n"
                         "1\n"
                         "#endif";
    ASSERT_EQUALS("", preprocess(code1));

    const char code2[] = "#ifndef A\n"
                         "1\n"
                         "#endif";
    ASSERT_EQUALS("\n1", preprocess(code2));
}

void ifA() {
    const char code[] = "#if A==1\n"
                        "X\n"
                        "#endif";
    ASSERT_EQUALS("", preprocess(code));

    std::map<std::string,std::string> defines;
    defines["A"] = "1";
    ASSERT_EQUALS("\nX", preprocess(code, defines));
}

void ifDefined() {
    const char code[] = "#if defined(A)\n"
                        "X\n"
                        "#endif";
    std::map<std::string, std::string> defs;
    ASSERT_EQUALS("", preprocess(code, defs));
    defs["A"] = "1";
    ASSERT_EQUALS("\nX", preprocess(code, defs));
}

void ifLogical() {
    const char code[] = "#if defined(A) || defined(B)\n"
                        "X\n"
                        "#endif";
    std::map<std::string, std::string> defs;
    ASSERT_EQUALS("", preprocess(code, defs));
    defs.clear();
    defs["A"] = "1";
    ASSERT_EQUALS("\nX", preprocess(code, defs));
    defs.clear();
    defs["B"] = "1";
    ASSERT_EQUALS("\nX", preprocess(code, defs));
}

void ifSizeof() {
    const char code[] = "#if sizeof(unsigned short)==2\n"
                        "X\n"
                        "#else\n"
                        "Y\n"
                        "#endif";
    ASSERT_EQUALS("\nX", preprocess(code));
}

void elif() {
    const char code1[] = "#ifndef X\n"
                         "1\n"
                         "#elif 1<2\n"
                         "2\n"
                         "#else\n"
                         "3\n"
                         "#endif";
    ASSERT_EQUALS("\n1", preprocess(code1));

    const char code2[] = "#ifdef X\n"
                         "1\n"
                         "#elif 1<2\n"
                         "2\n"
                         "#else\n"
                         "3\n"
                         "#endif";
    ASSERT_EQUALS("\n\n\n2", preprocess(code2));

    const char code3[] = "#ifdef X\n"
                         "1\n"
                         "#elif 1>2\n"
                         "2\n"
                         "#else\n"
                         "3\n"
                         "#endif";
    ASSERT_EQUALS("\n\n\n\n\n3", preprocess(code3));
}

void locationFile() {
    const char code[] = "#file \"a.h\"\n"
                        "1\n"
                        "#file \"b.h\"\n"
                        "2\n"
                        "#endfile\n"
                        "3\n"
                        "#endfile\n";
    std::istringstream istr(code);
    const simplecpp::TokenList &tokens = simplecpp::TokenList(istr);

    const simplecpp::Token *tok = tokens.cbegin();

    while (tok && tok->str != "1")
        tok = tok->next;
    ASSERT_EQUALS("a.h", tok ? tok->location.file : std::string(""));
    ASSERT_EQUALS(1U, tok ? tok->location.line : 0U);

    while (tok && tok->str != "2")
        tok = tok->next;
    ASSERT_EQUALS("b.h", tok ? tok->location.file : std::string(""));
    ASSERT_EQUALS(1U, tok ? tok->location.line : 0U);

    while (tok && tok->str != "3")
        tok = tok->next;
    ASSERT_EQUALS("a.h", tok ? tok->location.file : std::string(""));
    ASSERT_EQUALS(3U, tok ? tok->location.line : 0U);
}

void multiline() {
    const char code[] = "#define A \\\n"
                        "1\n"
                        "A";
    std::map<std::string, std::string> nodefines;
    std::istringstream istr(code);
    ASSERT_EQUALS("\n\n1", simplecpp::preprocess(simplecpp::TokenList(istr), nodefines).stringify());
}

void increment() {
    ASSERT_EQUALS("; ++ x ;", preprocess(";++x;"));
    ASSERT_EQUALS("; x ++ ;", preprocess(";x++;"));
    ASSERT_EQUALS("1 + + 2", preprocess("1++2"));
}

void tokenMacro1() {
    const char code[] = "#define A 123\n"
                        "A";
    std::map<std::string, std::string> nodefines;
    std::istringstream istr(code);
    const simplecpp::TokenList &tokenList = simplecpp::preprocess(simplecpp::TokenList(istr), nodefines);
    ASSERT_EQUALS("A", tokenList.cend()->macro);
}

void tokenMacro2() {
    const char code[] = "#define ADD(X,Y) X+Y\n"
                        "ADD(1,2)";
    std::map<std::string, std::string> nodefines;
    std::istringstream istr(code);
    const simplecpp::TokenList tokenList(simplecpp::preprocess(simplecpp::TokenList(istr), nodefines));
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
    const simplecpp::TokenList tokenList(simplecpp::preprocess(simplecpp::TokenList(istr), nodefines));
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
    const simplecpp::TokenList tokenList(simplecpp::preprocess(simplecpp::TokenList(istr), nodefines));
    const simplecpp::Token *tok = tokenList.cbegin();
    ASSERT_EQUALS("1", tok->str);
    ASSERT_EQUALS("A", tok->macro);
}

void undef() {
    std::istringstream istr("#define A\n"
                            "#undef A\n"
                            "#ifdef A\n"
                            "123\n"
                            "#endif");
    const std::map<std::string, std::string> nodefines;
    const simplecpp::TokenList tokenList(simplecpp::preprocess(simplecpp::TokenList(istr), nodefines));
    ASSERT_EQUALS("", tokenList.stringify());
}

static void testcase(const std::string &name, void (*f)(), int argc, char **argv)
{
    if (argc == 1)
        f();
    else {
        for (int i = 1; i < argc; i++) {
            if (name == argv[i])
                f();
        }
    }
}

#define TEST_CASE(F)    testcase(#F, F, argc, argv)

int main(int argc, char **argv) {

    TEST_CASE(comment);

    TEST_CASE(constFold);
    TEST_CASE(define1);
    TEST_CASE(define2);
    TEST_CASE(define3);
    TEST_CASE(define4);
    TEST_CASE(define5);
    TEST_CASE(define6);
    TEST_CASE(define_define_1);
    TEST_CASE(define_define_2);
    TEST_CASE(define_va_args_1);
    TEST_CASE(define_va_args_2);

    TEST_CASE(error);

    TEST_CASE(hash);
    TEST_CASE(hashhash);

    TEST_CASE(ifdef1);
    TEST_CASE(ifdef2);
    TEST_CASE(ifndef);
    TEST_CASE(ifA);
    TEST_CASE(ifDefined);
    TEST_CASE(ifLogical);
    TEST_CASE(ifSizeof);
    TEST_CASE(elif);

    TEST_CASE(locationFile);

    TEST_CASE(multiline);

    TEST_CASE(increment);

    TEST_CASE(tokenMacro1);
    TEST_CASE(tokenMacro2);
    TEST_CASE(tokenMacro3);
    TEST_CASE(tokenMacro4);

    TEST_CASE(undef);

    return 0;
}


#include <iostream>
#include <sstream>
#include <vector>
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
    std::vector<std::string> files;
    return simplecpp::TokenList(istr,files).stringify();
}

static std::string preprocess(const char code[], const simplecpp::DUI &dui, simplecpp::OutputList *outputList = NULL) {
    std::istringstream istr(code);
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::TokenList tokens(istr,files);
    tokens.removeComments();
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, tokens, files, filedata, dui, outputList);
    return tokens2.stringify();
}

static std::string preprocess(const char code[]) {
    return preprocess(code, simplecpp::DUI());
}

static std::string toString(const simplecpp::OutputList &outputList) {
    std::ostringstream ostr;
    for (const simplecpp::Output &output : outputList) {
        ostr << "file" << output.location.fileIndex << ',' << output.location.line << ',';

        switch (output.type) {
        case simplecpp::Output::Type::ERROR:
            ostr << "error,";
            break;
        case simplecpp::Output::Type::WARNING:
            ostr << "warning,";
            break;
        case simplecpp::Output::Type::MISSING_INCLUDE:
            ostr << "missing_include,";
            break;
        }

        ostr << output.msg << '\n';
    }
    return ostr.str();
}

static std::string testConstFold(const char code[]) {
    std::istringstream istr(code);
    std::vector<std::string> files;
    simplecpp::TokenList expr(istr, files);
    expr.constFold();
    return expr.stringify();
}

void combineOperators_floatliteral() {
    ASSERT_EQUALS("1.", preprocess("1."));
    ASSERT_EQUALS(".1", preprocess(".1"));
    ASSERT_EQUALS("3.1", preprocess("3.1"));
    ASSERT_EQUALS("1E7", preprocess("1E7"));
    ASSERT_EQUALS("1E-7", preprocess("1E-7"));
    ASSERT_EQUALS("1E+7", preprocess("1E+7"));
}

void combineOperators_increment() {
    ASSERT_EQUALS("; ++ x ;", preprocess(";++x;"));
    ASSERT_EQUALS("; x ++ ;", preprocess(";x++;"));
    ASSERT_EQUALS("1 + + 2", preprocess("1++2"));
}

void comment() {
    ASSERT_EQUALS("// abc", readfile("// abc"));
    ASSERT_EQUALS("", preprocess("// abc"));
    ASSERT_EQUALS("/*\n\n*/abc", readfile("/*\n\n*/abc"));
    ASSERT_EQUALS("\n\nabc", preprocess("/*\n\n*/abc"));
    ASSERT_EQUALS("* p = a / * b / * c ;", readfile("*p=a/ *b/ *c;"));
    ASSERT_EQUALS("* p = a / * b / * c ;", preprocess("*p=a/ *b/ *c;"));
}

void comment_multiline() {
    const char code[] = "#define ABC {// \\\n"
                        "}\n"
                        "void f() ABC\n";
    ASSERT_EQUALS("\n\nvoid f ( ) { }", preprocess(code));
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

void define7() {
    const char code[] = "#define A(X) X+1\n"
                        "A(1 /*23*/)";
    ASSERT_EQUALS("\n1 + 1", preprocess(code));
}

void define8() { // 6.10.3.10
    const char code[] = "#define A(X) \n"
                        "int A[10];";
    ASSERT_EQUALS("\nint A [ 10 ] ;", preprocess(code));
}

void define9() {
    const char code[] = "#define AB ab.AB\n"
                        "AB.CD\n";
    ASSERT_EQUALS("\nab . AB . CD", preprocess(code));
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

void define_define_3() {
    const char code[] = "#define ABC 123\n"
                        "#define A(B) A##B\n"
                        "A(BC)";
    ASSERT_EQUALS("\n\n123", preprocess(code));
}

void define_define_4() {
    const char code[] = "#define FOO1()\n"
                        "#define TEST(FOO) FOO FOO()\n"
                        "TEST(FOO1)";
    ASSERT_EQUALS("\n\nFOO1", preprocess(code));
}

void define_define_5() {
    const char code[] = "#define X() Y\n"
                        "#define Y() X\n"
                        "A: X()()()\n";
    // mcpp outputs "A: X()" and gcc/clang/vc outputs "A: Y"
    ASSERT_EQUALS("\n\nA : Y", preprocess(code)); // <- match the output from gcc/clang/vc
}

void define_define_6() {
    const char code1[] = "#define f(a) a*g\n"
                         "#define g f\n"
                         "a: f(2)(9)\n";
    ASSERT_EQUALS("\n\na : 2 * f ( 9 )", preprocess(code1));

    const char code2[] = "#define f(a) a*g\n"
                         "#define g(a) f(a)\n"
                         "a: f(2)(9)\n";
    ASSERT_EQUALS("\n\na : 2 * 9 * g", preprocess(code2));
}

void define_define_7() {
    const char code[] = "#define f(x) g(x\n"
                        "#define g(x) x()\n"
                        "f(f))\n";
    ASSERT_EQUALS("\n\nf ( )", preprocess(code));
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

void define_va_args_3() { // min number of arguments
    const char code[] = "#define A(x, y, z...) 1\n"
                        "A(1, 2)\n";
    ASSERT_EQUALS("\n1", preprocess(code));
}

void error() {
    std::istringstream istr("#error    hello world! \n");
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::OutputList outputList;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files,"test.c"), files, filedata, simplecpp::DUI(), &outputList);
    ASSERT_EQUALS("file0,1,error,#error hello world!\n", toString(outputList));
}

void hash() {
    const char code[] = "#define a(x) #x\n"
                        "a(1)\n"
                        "a(2+3)";
    ASSERT_EQUALS("\n"
                  "\"1\"\n"
                  "\"2+3\"", preprocess(code));
}

void hashhash1() { // #4703
    const char code[] = "#define MACRO( A, B, C ) class A##B##C##Creator {};\n"
                        "MACRO( B\t, U , G )";
    ASSERT_EQUALS("\nclass BUGCreator { } ;", preprocess(code));
}

void hashhash2() {
    const char code[] = "#define A(x) a##x\n"
                        "#define B 0\n"
                        "A(B)";
    ASSERT_EQUALS("\n\naB", preprocess(code));
}

void hashhash3() {
    const char code[] = "#define A(B) A##B\n"
                        "#define a(B) A(B)\n"
                        "a(A(B))";
    ASSERT_EQUALS("\n\nAAB", preprocess(code));
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

    simplecpp::DUI dui;
    dui.defines.push_back("A=1");
    ASSERT_EQUALS("\nX", preprocess(code, dui));
}

void ifCharLiteral() {
    const char code[] = "#if ('A'==0x41)\n"
                        "123\n"
                        "#endif";
    ASSERT_EQUALS("\n123", preprocess(code));
}

void ifDefined() {
    const char code[] = "#if defined(A)\n"
                        "X\n"
                        "#endif";
    simplecpp::DUI dui;
    ASSERT_EQUALS("", preprocess(code, dui));
    dui.defines.push_back("A=1");
    ASSERT_EQUALS("\nX", preprocess(code, dui));
}

void ifLogical() {
    const char code[] = "#if defined(A) || defined(B)\n"
                        "X\n"
                        "#endif";
    simplecpp::DUI dui;
    ASSERT_EQUALS("", preprocess(code, dui));
    dui.defines.clear();
    dui.defines.push_back("A=1");
    ASSERT_EQUALS("\nX", preprocess(code, dui));
    dui.defines.clear();
    dui.defines.push_back("B=1");
    ASSERT_EQUALS("\nX", preprocess(code, dui));
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

void ifif() {
    // source code from LLVM
    const char code[] = "#if defined(__has_include)\n"
                        "#if __has_include(<sanitizer / coverage_interface.h>)\n"
                        "#endif\n"
                        "#endif\n";
    ASSERT_EQUALS("", preprocess(code));
}

void ifoverflow() {
    // source code from CLANG
    const char code[] = "#if 0x7FFFFFFFFFFFFFFF*2\n"
                        "#endif\n"
                        "#if 0xFFFFFFFFFFFFFFFF*2\n"
                        "#endif\n"
                        "#if 0x7FFFFFFFFFFFFFFF+1\n"
                        "#endif\n"
                        "#if 0xFFFFFFFFFFFFFFFF+1\n"
                        "#endif\n"
                        "#if 0x7FFFFFFFFFFFFFFF--1\n"
                        "#endif\n"
                        "#if 0xFFFFFFFFFFFFFFFF--1\n"
                        "#endif\n"
                        "123";
    (void)preprocess(code);
}

void ifdiv0() {
    const char code[] = "#if 1000/0\n"
                        "#endif\n"
                        "123";
    ASSERT_EQUALS("", preprocess(code));
}

void ifalt() { // using "and", "or", etc
    const char *code;

    code = "#if 1 and 1\n"
           "1\n"
           "#else\n"
           "2\n"
           "#endif\n";
    ASSERT_EQUALS("\n1", preprocess(code));

    code = "#if 1 or 0\n"
           "1\n"
           "#else\n"
           "2\n"
           "#endif\n";
    ASSERT_EQUALS("\n1", preprocess(code));
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
    std::vector<std::string> files;
    const simplecpp::TokenList &tokens = simplecpp::TokenList(istr,files);

    const simplecpp::Token *tok = tokens.cbegin();

    while (tok && tok->str != "1")
        tok = tok->next;
    ASSERT_EQUALS("a.h", tok ? tok->location.file() : "");
    ASSERT_EQUALS(1U, tok ? tok->location.line : 0U);

    while (tok && tok->str != "2")
        tok = tok->next;
    ASSERT_EQUALS("b.h", tok ? tok->location.file() : "");
    ASSERT_EQUALS(1U, tok ? tok->location.line : 0U);

    while (tok && tok->str != "3")
        tok = tok->next;
    ASSERT_EQUALS("a.h", tok ? tok->location.file() : "");
    ASSERT_EQUALS(3U, tok ? tok->location.line : 0U);
}

void missingInclude1() {
    const simplecpp::DUI dui;
    std::istringstream istr("#include \"notexist.h\"\n");
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::OutputList outputList;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files), files, filedata, dui, &outputList);
    ASSERT_EQUALS("file0,1,missing_include,Header not found: \"notexist.h\"\n", toString(outputList));
}

void missingInclude2() {
    const simplecpp::DUI dui;
    std::istringstream istr("#include \"foo.h\"\n"); // this file exists
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    filedata["foo.h"] = 0;
    simplecpp::OutputList outputList;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files), files, filedata, dui, &outputList);
    ASSERT_EQUALS("", toString(outputList));
}

void missingInclude3() {
    const simplecpp::DUI dui;
    std::istringstream istr("#ifdef UNDEFINED\n#include \"notexist.h\"\n#endif\n"); // this file is not included
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::OutputList outputList;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files), files, filedata, dui, &outputList);
    ASSERT_EQUALS("", toString(outputList));
}

void multiline() {
    const char code[] = "#define A \\\n"
                        "1\n"
                        "A";
    const simplecpp::DUI dui;
    std::istringstream istr(code);
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files), files, filedata, dui);
    ASSERT_EQUALS("\n\n1", tokens2.stringify());
}

void include1() {
    const char code[] = "#include \"A.h\"\n";
    ASSERT_EQUALS("# include \"A.h\"", readfile(code));
}

void include2() {
    const char code[] = "#include <A.h>\n";
    ASSERT_EQUALS("# include <A.h>", readfile(code));
}

void readfile_string() {
    const char code[] = "A = \"abc\'def\"";
    ASSERT_EQUALS("A = \"abc\'def\"", readfile(code));
    ASSERT_EQUALS("( \"\\\\\\\\\" )", readfile("(\"\\\\\\\\\")"));
}

void tokenMacro1() {
    const char code[] = "#define A 123\n"
                        "A";
    const simplecpp::DUI dui;
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    std::istringstream istr(code);
    simplecpp::TokenList tokenList(files);
    simplecpp::preprocess(tokenList, simplecpp::TokenList(istr,files), files, filedata, dui);
    ASSERT_EQUALS("A", tokenList.cend()->macro);
}

void tokenMacro2() {
    const char code[] = "#define ADD(X,Y) X+Y\n"
                        "ADD(1,2)";
    const simplecpp::DUI dui;
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    std::istringstream istr(code);
    simplecpp::TokenList tokenList(files);
    simplecpp::preprocess(tokenList, simplecpp::TokenList(istr,files), files, filedata, dui);
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
    const simplecpp::DUI dui;
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    std::istringstream istr(code);
    simplecpp::TokenList tokenList(files);
    simplecpp::preprocess(tokenList, simplecpp::TokenList(istr,files), files, filedata, dui);
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
    const simplecpp::DUI dui;
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    std::istringstream istr(code);
    simplecpp::TokenList tokenList(files);
    simplecpp::preprocess(tokenList, simplecpp::TokenList(istr,files), files, filedata, dui);
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
    const simplecpp::DUI dui;
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::TokenList tokenList(files);
    simplecpp::preprocess(tokenList, simplecpp::TokenList(istr,files), files, filedata, dui);
    ASSERT_EQUALS("", tokenList.stringify());
}

void userdef() {
    std::istringstream istr("#ifdef A\n123\n#endif\n");
    simplecpp::DUI dui;
    dui.defines.push_back("A=1");
    std::vector<std::string> files;
    const simplecpp::TokenList tokens1 = simplecpp::TokenList(istr, files);
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, tokens1, files, filedata, dui);
    ASSERT_EQUALS("\n123", tokens2.stringify());
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

    TEST_CASE(combineOperators_floatliteral);
    TEST_CASE(combineOperators_increment);

    TEST_CASE(comment);
    TEST_CASE(comment_multiline);

    TEST_CASE(constFold);

    TEST_CASE(define1);
    TEST_CASE(define2);
    TEST_CASE(define3);
    TEST_CASE(define4);
    TEST_CASE(define5);
    TEST_CASE(define6);
    TEST_CASE(define7);
    TEST_CASE(define8);
    TEST_CASE(define9);
    TEST_CASE(define_define_1);
    TEST_CASE(define_define_2);
    TEST_CASE(define_define_3);
    TEST_CASE(define_define_4);
    TEST_CASE(define_define_5);
    TEST_CASE(define_define_6);
    TEST_CASE(define_define_7);
    TEST_CASE(define_va_args_1);
    TEST_CASE(define_va_args_2);
    TEST_CASE(define_va_args_3);

    TEST_CASE(error);

    TEST_CASE(hash);
    TEST_CASE(hashhash1);
    TEST_CASE(hashhash2);
    TEST_CASE(hashhash3);

    TEST_CASE(ifdef1);
    TEST_CASE(ifdef2);
    TEST_CASE(ifndef);
    TEST_CASE(ifA);
    TEST_CASE(ifCharLiteral);
    TEST_CASE(ifDefined);
    TEST_CASE(ifLogical);
    TEST_CASE(ifSizeof);
    TEST_CASE(elif);
    TEST_CASE(ifif);
    TEST_CASE(ifoverflow);
    TEST_CASE(ifdiv0);
    TEST_CASE(ifalt); // using "and", "or", etc

    TEST_CASE(locationFile);

    TEST_CASE(missingInclude1);
    TEST_CASE(missingInclude2);
    TEST_CASE(missingInclude3);

    TEST_CASE(multiline);

    TEST_CASE(include1);
    TEST_CASE(include2);

    TEST_CASE(readfile_string);

    TEST_CASE(tokenMacro1);
    TEST_CASE(tokenMacro2);
    TEST_CASE(tokenMacro3);
    TEST_CASE(tokenMacro4);

    TEST_CASE(undef);

    TEST_CASE(userdef);

    return 0;
}

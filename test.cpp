
#include <iostream>
#include <sstream>
#include <vector>
#include "simplecpp.h"

int numberOfFailedAssertions = 0;

#define ASSERT_EQUALS(expected, actual)  (assertEquals((expected), (actual), __LINE__))

static int assertEquals(const std::string &expected, const std::string &actual, int line)
{
    if (expected != actual) {
        numberOfFailedAssertions++;
        std::cerr << "------ assertion failed ---------" << std::endl;
        std::cerr << "line " << line << std::endl;
        std::cerr << "expected:" << expected << std::endl;
        std::cerr << "actual:" << actual << std::endl;
    }
    return (expected == actual);
}

static int assertEquals(const unsigned int &expected, const unsigned int &actual, int line)
{
    return assertEquals(std::to_string(expected), std::to_string(actual), line);
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

#define TEST_CASE(F)    (testcase(#F, F, argc, argv))



static std::string readfile(const char code[], int sz=-1, simplecpp::OutputList *outputList=nullptr)
{
    std::istringstream istr(sz == -1 ? std::string(code) : std::string(code,sz));
    std::vector<std::string> files;
    return simplecpp::TokenList(istr,files,std::string(),outputList).stringify();
}

static std::string preprocess(const char code[], const simplecpp::DUI &dui, simplecpp::OutputList *outputList = NULL)
{
    std::istringstream istr(code);
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::TokenList tokens(istr,files);
    tokens.removeComments();
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, tokens, files, filedata, dui, outputList);
    return tokens2.stringify();
}

static std::string preprocess(const char code[])
{
    return preprocess(code, simplecpp::DUI());
}

static std::string toString(const simplecpp::OutputList &outputList)
{
    std::ostringstream ostr;
    for (const simplecpp::Output &output : outputList) {
        ostr << "file" << output.location.fileIndex << ',' << output.location.line << ',';

        switch (output.type) {
        case simplecpp::Output::Type::ERROR:
            ostr << "#error,";
            break;
        case simplecpp::Output::Type::WARNING:
            ostr << "#warning,";
            break;
        case simplecpp::Output::Type::MISSING_HEADER:
            ostr << "missing_header,";
            break;
        case simplecpp::Output::Type::INCLUDE_NESTED_TOO_DEEPLY:
            ostr << "include_nested_too_deeply,";
            break;
        case simplecpp::Output::Type::SYNTAX_ERROR:
            ostr << "syntax_error,";
            break;
        case simplecpp::Output::Type::PORTABILITY_BACKSLASH:
            ostr << "portability_backslash,";
            break;
        case simplecpp::Output::Type::UNHANDLED_CHAR_ERROR:
            ostr << "unhandled_char_error,";
        }

        ostr << output.msg << '\n';
    }
    return ostr.str();
}

static void backslash()
{
    // <backslash><space><newline> preprocessed differently
    simplecpp::OutputList outputList;

    readfile("//123 \\\n456", -1, &outputList);
    ASSERT_EQUALS("", toString(outputList));
    readfile("//123 \\ \n456", -1, &outputList);
    ASSERT_EQUALS("file0,1,portability_backslash,Combination 'backslash space newline' is not portable.\n", toString(outputList));

    outputList.clear();
    readfile("#define A \\\n123", -1, &outputList);
    ASSERT_EQUALS("", toString(outputList));
    readfile("#define A \\ \n123", -1, &outputList);
    ASSERT_EQUALS("file0,1,portability_backslash,Combination 'backslash space newline' is not portable.\n", toString(outputList));
}

static void builtin()
{
    ASSERT_EQUALS("\"\" 1 0", preprocess("__FILE__ __LINE__ __COUNTER__"));
    ASSERT_EQUALS("\n\n3", preprocess("\n\n__LINE__"));
    ASSERT_EQUALS("\n\n0", preprocess("\n\n__COUNTER__"));
    ASSERT_EQUALS("\n\n0 1", preprocess("\n\n__COUNTER__ __COUNTER__"));

    ASSERT_EQUALS("\n0 + 0", preprocess("#define A(c)  c+c\n"
                                        "A(__COUNTER__)\n"));

    ASSERT_EQUALS("\n0 + 0 + 1", preprocess("#define A(c)  c+c+__COUNTER__\n"
                                            "A(__COUNTER__)\n"));
}

static std::string testConstFold(const char code[])
{
    std::istringstream istr(code);
    std::vector<std::string> files;
    simplecpp::TokenList expr(istr, files);
    try {
        expr.constFold();
    } catch (std::exception &) {
        return "exception";
    }
    return expr.stringify();
}

static void combineOperators_floatliteral()
{
    ASSERT_EQUALS("1.", preprocess("1."));
    ASSERT_EQUALS("1.f", preprocess("1.f"));
    ASSERT_EQUALS(".1", preprocess(".1"));
    ASSERT_EQUALS(".1f", preprocess(".1f"));
    ASSERT_EQUALS("3.1", preprocess("3.1"));
    ASSERT_EQUALS("1E7", preprocess("1E7"));
    ASSERT_EQUALS("1E-7", preprocess("1E-7"));
    ASSERT_EQUALS("1E+7", preprocess("1E+7"));
    ASSERT_EQUALS("0x1E + 7", preprocess("0x1E+7"));
}

static void combineOperators_increment()
{
    ASSERT_EQUALS("; ++ x ;", preprocess(";++x;"));
    ASSERT_EQUALS("; x ++ ;", preprocess(";x++;"));
    ASSERT_EQUALS("1 + + 2", preprocess("1++2"));
}

static void combineOperators_coloncolon()
{
    ASSERT_EQUALS("x ? y : :: z", preprocess("x ? y : ::z"));
}

static void comment()
{
    ASSERT_EQUALS("// abc", readfile("// abc"));
    ASSERT_EQUALS("", preprocess("// abc"));
    ASSERT_EQUALS("/*\n\n*/abc", readfile("/*\n\n*/abc"));
    ASSERT_EQUALS("\n\nabc", preprocess("/*\n\n*/abc"));
    ASSERT_EQUALS("* p = a / * b / * c ;", readfile("*p=a/ *b/ *c;"));
    ASSERT_EQUALS("* p = a / * b / * c ;", preprocess("*p=a/ *b/ *c;"));
}

static void comment_multiline()
{
    const char code[] = "#define ABC {// \\\n"
                        "}\n"
                        "void f() ABC\n";
    ASSERT_EQUALS("\n\nvoid f ( ) { }", preprocess(code));
}


static void constFold()
{
    ASSERT_EQUALS("7", testConstFold("1+2*3"));
    ASSERT_EQUALS("15", testConstFold("1+2*(3+4)"));
    ASSERT_EQUALS("123", testConstFold("+123"));
    ASSERT_EQUALS("1", testConstFold("-123<1"));
    ASSERT_EQUALS("6", testConstFold("14 & 7"));
    ASSERT_EQUALS("29", testConstFold("13 ^ 16"));
    ASSERT_EQUALS("25", testConstFold("24 | 1"));
    ASSERT_EQUALS("2", testConstFold("1?2:3"));
    ASSERT_EQUALS("exception", testConstFold("!1 ? 2 :"));
    ASSERT_EQUALS("exception", testConstFold("?2:3"));
}

static void define1()
{
    const char code[] = "#define A 1+2\n"
                        "a=A+3;";
    ASSERT_EQUALS("# define A 1 + 2\n"
                  "a = A + 3 ;",
                  readfile(code));
    ASSERT_EQUALS("\na = 1 + 2 + 3 ;",
                  preprocess(code));
}

static void define2()
{
    const char code[] = "#define ADD(A,B) A+B\n"
                        "ADD(1+2,3);";
    ASSERT_EQUALS("# define ADD ( A , B ) A + B\n"
                  "ADD ( 1 + 2 , 3 ) ;",
                  readfile(code));
    ASSERT_EQUALS("\n1 + 2 + 3 ;",
                  preprocess(code));
}

static void define3()
{
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

static void define4()
{
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

static void define5()
{
    const char code[] = "#define add(x,y) x+y\n"
                        "add(add(1,2),3)";
    ASSERT_EQUALS("\n1 + 2 + 3", preprocess(code));
}

static void define6()
{
    const char code[] = "#define A() 1\n"
                        "A()";
    ASSERT_EQUALS("\n1", preprocess(code));
}

static void define7()
{
    const char code[] = "#define A(X) X+1\n"
                        "A(1 /*23*/)";
    ASSERT_EQUALS("\n1 + 1", preprocess(code));
}

static void define8()   // 6.10.3.10
{
    const char code[] = "#define A(X) \n"
                        "int A[10];";
    ASSERT_EQUALS("\nint A [ 10 ] ;", preprocess(code));
}

static void define9()
{
    const char code[] = "#define AB ab.AB\n"
                        "AB.CD\n";
    ASSERT_EQUALS("\nab . AB . CD", preprocess(code));
}

static void define_invalid_1()
{
    std::istringstream istr("#define  A(\nB\n");
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::OutputList outputList;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files,"test.c"), files, filedata, simplecpp::DUI(), &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,Failed to parse #define\n", toString(outputList));
}

static void define_invalid_2()
{
    std::istringstream istr("#define\nhas#");
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::OutputList outputList;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files,"test.c"), files, filedata, simplecpp::DUI(), &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,Failed to parse #define\n", toString(outputList));
}

static void define_define_1()
{
    const char code[] = "#define A(x) (x+1)\n"
                        "#define B    A(\n"
                        "B(i))";
    ASSERT_EQUALS("\n\n( ( i ) + 1 )", preprocess(code));
}

static void define_define_2()
{
    const char code[] = "#define A(m)    n=m\n"
                        "#define B(x)    A(x)\n"
                        "B(0)";
    ASSERT_EQUALS("\n\nn = 0", preprocess(code));
}

static void define_define_3()
{
    const char code[] = "#define ABC 123\n"
                        "#define A(B) A##B\n"
                        "A(BC)";
    ASSERT_EQUALS("\n\n123", preprocess(code));
}

static void define_define_4()
{
    const char code[] = "#define FOO1()\n"
                        "#define TEST(FOO) FOO FOO()\n"
                        "TEST(FOO1)";
    ASSERT_EQUALS("\n\nFOO1", preprocess(code));
}

static void define_define_5()
{
    const char code[] = "#define X() Y\n"
                        "#define Y() X\n"
                        "A: X()()()\n";
    // mcpp outputs "A: X()" and gcc/clang/vc outputs "A: Y"
    ASSERT_EQUALS("\n\nA : Y", preprocess(code)); // <- match the output from gcc/clang/vc
}

static void define_define_6()
{
    const char code1[] = "#define f(a) a*g\n"
                         "#define g f\n"
                         "a: f(2)(9)\n";
    ASSERT_EQUALS("\n\na : 2 * f ( 9 )", preprocess(code1));

    const char code2[] = "#define f(a) a*g\n"
                         "#define g(a) f(a)\n"
                         "a: f(2)(9)\n";
    ASSERT_EQUALS("\n\na : 2 * 9 * g", preprocess(code2));
}

static void define_define_7()
{
    const char code[] = "#define f(x) g(x\n"
                        "#define g(x) x()\n"
                        "f(f))\n";
    ASSERT_EQUALS("\n\nf ( )", preprocess(code));
}

static void define_define_8()   // line break in nested macro call
{
    const char code[] = "#define A(X,Y)  ((X)*(Y))\n"
                        "#define B(X,Y)  ((X)+(Y))\n"
                        "B(0,A(255,x+\n"
                        "y))\n";
    ASSERT_EQUALS("\n\n( ( 0 ) + ( ( ( 255 ) * ( x + y ) ) ) )", preprocess(code));
}

static void define_define_9()   // line break in nested macro call
{
    const char code[] = "#define A(X) X\n"
                        "#define B(X) X\n"
                        "A(\nB(dostuff(1,\n2)))\n";
    ASSERT_EQUALS("\n\ndostuff ( 1 , 2 )", preprocess(code));
}

static void define_define_10()
{
    const char code[] = "#define glue(a, b) a ## b\n"
                        "#define xglue(a, b) glue(a, b)\n"
                        "#define AB 1\n"
                        "#define B B 2\n"
                        "xglue(A, B)\n";
    ASSERT_EQUALS("\n\n\n\n1 2", preprocess(code));
}

static void define_define_11()
{
    const char code[] = "#define XY(x, y)   x ## y\n"
                        "#define XY2(x, y)  XY(x, y)\n"
                        "#define PORT       XY2(P, 2)\n"
                        "#define ABC        XY2(PORT, DIR)\n"
                        "ABC;\n";
    ASSERT_EQUALS("\n\n\n\nP2DIR ;", preprocess(code));
}

static void define_define_12()
{
    const char code[] = "#define XY(Z)  Z\n"
                        "#define X(ID)  X##ID(0)\n"
                        "X(Y)\n";
    ASSERT_EQUALS("\n\n0", preprocess(code));
}

static void define_va_args_1()
{
    const char code[] = "#define A(fmt...) dostuff(fmt)\n"
                        "A(1,2);";
    ASSERT_EQUALS("\ndostuff ( 1 , 2 ) ;", preprocess(code));
}

static void define_va_args_2()
{
    const char code[] = "#define A(X,...) X(#__VA_ARGS__)\n"
                        "A(f,123);";
    ASSERT_EQUALS("\nf ( \"123\" ) ;", preprocess(code));
}

static void define_va_args_3()   // min number of arguments
{
    const char code[] = "#define A(x, y, z...) 1\n"
                        "A(1, 2)\n";
    ASSERT_EQUALS("\n1", preprocess(code));
}

static void dollar()
{
    ASSERT_EQUALS("$ab", readfile("$ab"));
    ASSERT_EQUALS("a$b", readfile("a$b"));
}

static void dotDotDot()
{
    ASSERT_EQUALS("1 . . . 2", readfile("1 ... 2"));
}

static void error()
{
    std::istringstream istr("#error    hello world! \n");
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::OutputList outputList;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files,"test.c"), files, filedata, simplecpp::DUI(), &outputList);
    ASSERT_EQUALS("file0,1,#error,#error hello world!\n", toString(outputList));
}

static void garbage()
{
    const simplecpp::DUI dui;
    simplecpp::OutputList outputList;

    outputList.clear();
    preprocess("#ifdef\n", dui, &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,Syntax error in #ifdef\n", toString(outputList));

    outputList.clear();
    preprocess("#define TEST2() A ##\nTEST2()\n", dui, &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'TEST2', Invalid ## usage when expanding 'TEST2'.\n", toString(outputList));

    outputList.clear();
    preprocess("#define CON(a,b)  a##b##\nCON(1,2)\n", dui, &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'CON', Invalid ## usage when expanding 'CON'.\n", toString(outputList));
}

static void garbage_endif()
{
    const simplecpp::DUI dui;
    simplecpp::OutputList outputList;

    outputList.clear();
    preprocess("#elif A<0\n", dui, &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,#elif without #if\n", toString(outputList));

    outputList.clear();
    preprocess("#else\n", dui, &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,#else without #if\n", toString(outputList));

    outputList.clear();
    preprocess("#endif\n", dui, &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,#endif without #if\n", toString(outputList));
}

static void hash()
{
    ASSERT_EQUALS("x = \"1\"", preprocess("x=#__LINE__"));

    const char code[] = "#define a(x) #x\n"
                        "a(1)\n"
                        "a(2+3)";
    ASSERT_EQUALS("\n"
                  "\"1\"\n"
                  "\"2+3\"", preprocess(code));

    ASSERT_EQUALS("\n\"\\\"abc\\\\0\\\"\"", preprocess("#define str(x) #x\nstr(\"abc\\0\")\n"));

    ASSERT_EQUALS("\n\n( \"123\" )",
                  preprocess("#define A(x)  (x)\n"
                             "#define B(x)  A(#x)\n"
                             "B(123)"));
}

static void hashhash1()   // #4703
{
    const char code[] = "#define MACRO( A, B, C ) class A##B##C##Creator {};\n"
                        "MACRO( B\t, U , G )";
    ASSERT_EQUALS("\nclass BUGCreator { } ;", preprocess(code));
}

static void hashhash2()
{
    const char code[] = "#define A(x) a##x\n"
                        "#define B 0\n"
                        "A(B)";
    ASSERT_EQUALS("\n\naB", preprocess(code));
}

static void hashhash3()
{
    const char code[] = "#define A(B) A##B\n"
                        "#define a(B) A(B)\n"
                        "a(A(B))";
    ASSERT_EQUALS("\n\nAAB", preprocess(code));
}

static void hashhash4()    // nonstandard gcc/clang extension for empty varargs
{
    const char *code;

    code = "#define A(x,y...)  a(x,##y)\n"
           "A(1)\n";
    ASSERT_EQUALS("\na ( 1 )", preprocess(code));

    code = "#define A(x, ...)   a(x, ## __VA_ARGS__)\n"
           "#define B(x, ...)   A(x, ## __VA_ARGS__)\n"
           "B(1);";
    ASSERT_EQUALS("\n\na ( 1 ) ;", preprocess(code));
}

static void hashhash5()
{
    ASSERT_EQUALS("x1", preprocess("x##__LINE__"));
}

static void hashhash6()
{
    const char *code;

    code = "#define A(X, ...) LOG(X, ##__VA_ARGS__)\n"
           "A(1,(int)2)";
    ASSERT_EQUALS("\nLOG ( 1 , ( int ) 2 )", preprocess(code));

    code = "#define A(X, ...) LOG(X, ##__VA_ARGS__)\n"
           "#define B(X, ...) A(X, ##__VA_ARGS__)\n"
           "#define C(X, ...) B(X, ##__VA_ARGS__)\n"
           "C(1,(int)2)";
    ASSERT_EQUALS("\n\n\nLOG ( 1 , ( int ) 2 )", preprocess(code));
}

static void hashhash7()   // # ## #  (C standard; 6.10.3.3.p4)
{
    const char *code;

    code = "#define hash_hash # ## #\n"
           "x hash_hash y";
    ASSERT_EQUALS("\nx ## y", preprocess(code));
}

static void hashhash8()
{
    const char code[] = "#define a(xy)    x##y = xy\n"
                        "a(123);";
    ASSERT_EQUALS("\nxy = 123 ;", preprocess(code));
}

static void ifdef1()
{
    const char code[] = "#ifdef A\n"
                        "1\n"
                        "#else\n"
                        "2\n"
                        "#endif";
    ASSERT_EQUALS("\n\n\n2", preprocess(code));
}

static void ifdef2()
{
    const char code[] = "#define A\n"
                        "#ifdef A\n"
                        "1\n"
                        "#else\n"
                        "2\n"
                        "#endif";
    ASSERT_EQUALS("\n\n1", preprocess(code));
}

static void ifndef()
{
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

static void ifA()
{
    const char code[] = "#if A==1\n"
                        "X\n"
                        "#endif";
    ASSERT_EQUALS("", preprocess(code));

    simplecpp::DUI dui;
    dui.defines.push_back("A=1");
    ASSERT_EQUALS("\nX", preprocess(code, dui));
}

static void ifCharLiteral()
{
    const char code[] = "#if ('A'==0x41)\n"
                        "123\n"
                        "#endif";
    ASSERT_EQUALS("\n123", preprocess(code));
}

static void ifDefined()
{
    const char code[] = "#if defined(A)\n"
                        "X\n"
                        "#endif";
    simplecpp::DUI dui;
    ASSERT_EQUALS("", preprocess(code, dui));
    dui.defines.push_back("A=1");
    ASSERT_EQUALS("\nX", preprocess(code, dui));
}

static void ifDefinedNoPar()
{
    const char code[] = "#if defined A\n"
                        "X\n"
                        "#endif";
    simplecpp::DUI dui;
    ASSERT_EQUALS("", preprocess(code, dui));
    dui.defines.push_back("A=1");
    ASSERT_EQUALS("\nX", preprocess(code, dui));
}

static void ifDefinedNested()
{
    const char code[] = "#define FOODEF defined(FOO)\n"
                        "#if FOODEF\n"
                        "X\n"
                        "#endif";
    simplecpp::DUI dui;
    ASSERT_EQUALS("", preprocess(code, dui));
    dui.defines.push_back("FOO=1");
    ASSERT_EQUALS("\n\nX", preprocess(code, dui));
}

static void ifDefinedNestedNoPar()
{
    const char code[] = "#define FOODEF defined FOO\n"
                        "#if FOODEF\n"
                        "X\n"
                        "#endif";
    simplecpp::DUI dui;
    ASSERT_EQUALS("", preprocess(code, dui));
    dui.defines.push_back("FOO=1");
    ASSERT_EQUALS("\n\nX", preprocess(code, dui));
}

static void ifDefinedInvalid1()   // #50 - invalid unterminated defined
{
    const char code[] = "#if defined(A";
    simplecpp::DUI dui;
    simplecpp::OutputList outputList;
    std::vector<std::string> files;
    simplecpp::TokenList tokens2(files);
    std::istringstream istr(code);
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files), files, filedata, dui, &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,failed to evaluate #if condition\n", toString(outputList));
}

static void ifDefinedInvalid2()
{
    const char code[] = "#if defined";
    simplecpp::DUI dui;
    simplecpp::OutputList outputList;
    std::vector<std::string> files;
    simplecpp::TokenList tokens2(files);
    std::istringstream istr(code);
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files), files, filedata, dui, &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,failed to evaluate #if condition\n", toString(outputList));
}

static void ifLogical()
{
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

static void ifSizeof()
{
    const char code[] = "#if sizeof(unsigned short)==2\n"
                        "X\n"
                        "#else\n"
                        "Y\n"
                        "#endif";
    ASSERT_EQUALS("\nX", preprocess(code));
}

static void elif()
{
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

static void ifif()
{
    // source code from LLVM
    const char code[] = "#if defined(__has_include)\n"
                        "#if __has_include(<sanitizer / coverage_interface.h>)\n"
                        "#endif\n"
                        "#endif\n";
    ASSERT_EQUALS("", preprocess(code));
}

static void ifoverflow()
{
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

static void ifdiv0()
{
    const char code[] = "#if 1000/0\n"
                        "#endif\n"
                        "123";
    ASSERT_EQUALS("", preprocess(code));
}

static void ifalt()   // using "and", "or", etc
{
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

static void missingHeader1()
{
    const simplecpp::DUI dui;
    std::istringstream istr("#include \"notexist.h\"\n");
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::OutputList outputList;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files), files, filedata, dui, &outputList);
    ASSERT_EQUALS("file0,1,missing_header,Header not found: \"notexist.h\"\n", toString(outputList));
}

static void missingHeader2()
{
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

static void missingHeader3()
{
    const simplecpp::DUI dui;
    std::istringstream istr("#ifdef UNDEFINED\n#include \"notexist.h\"\n#endif\n"); // this file is not included
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::OutputList outputList;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files), files, filedata, dui, &outputList);
    ASSERT_EQUALS("", toString(outputList));
}

static void nestedInclude()
{
    std::istringstream istr("#include \"test.h\"\n");
    std::vector<std::string> files;
    simplecpp::TokenList rawtokens(istr,files,"test.h");
    std::map<std::string, simplecpp::TokenList*> filedata;
    filedata["test.h"] = &rawtokens;

    const simplecpp::DUI dui;
    simplecpp::OutputList outputList;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, rawtokens, files, filedata, dui, &outputList);

    ASSERT_EQUALS("file0,1,include_nested_too_deeply,#include nested too deeply\n", toString(outputList));
}

static void multiline1()
{
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

static void multiline2()
{
    const char code[] = "#define A /*\\\n"
                        "*/1\n"
                        "A";
    const simplecpp::DUI dui;
    std::istringstream istr(code);
    std::vector<std::string> files;
    simplecpp::TokenList rawtokens(istr,files);
    ASSERT_EQUALS("# define A /**/ 1\n\nA", rawtokens.stringify());
    rawtokens.removeComments();
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, rawtokens, files, filedata, dui);
    ASSERT_EQUALS("\n\n1", tokens2.stringify());
}

static void multiline3()   // #28 - macro with multiline comment
{
    const char code[] = "#define A /*\\\n"
                        "           */ 1\n"
                        "A";
    const simplecpp::DUI dui;
    std::istringstream istr(code);
    std::vector<std::string> files;
    simplecpp::TokenList rawtokens(istr,files);
    ASSERT_EQUALS("# define A /*           */ 1\n\nA", rawtokens.stringify());
    rawtokens.removeComments();
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, rawtokens, files, filedata, dui);
    ASSERT_EQUALS("\n\n1", tokens2.stringify());
}

static void multiline4()   // #28 - macro with multiline comment
{
    const char code[] = "#define A \\\n"
                        "          /*\\\n"
                        "           */ 1\n"
                        "A";
    const simplecpp::DUI dui;
    std::istringstream istr(code);
    std::vector<std::string> files;
    simplecpp::TokenList rawtokens(istr,files);
    ASSERT_EQUALS("# define A /*           */ 1\n\n\nA", rawtokens.stringify());
    rawtokens.removeComments();
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, rawtokens, files, filedata, dui);
    ASSERT_EQUALS("\n\n\n1", tokens2.stringify());
}

static void multiline5()   // column
{
    const char code[] = "#define A\\\n"
                        "(";
    const simplecpp::DUI dui;
    std::istringstream istr(code);
    std::vector<std::string> files;
    simplecpp::TokenList rawtokens(istr,files);
    ASSERT_EQUALS("# define A (", rawtokens.stringify());
    ASSERT_EQUALS(11, rawtokens.back()->location.col);
}

static void include1()
{
    const char code[] = "#include \"A.h\"\n";
    ASSERT_EQUALS("# include \"A.h\"", readfile(code));
}

static void include2()
{
    const char code[] = "#include <A.h>\n";
    ASSERT_EQUALS("# include <A.h>", readfile(code));
}

static void include3()   // #16 - crash when expanding macro from header
{
    const char code_c[] = "#include \"A.h\"\n"
                          "glue(1,2,3,4)\n" ;
    const char code_h[] = "#define glue(a,b,c,d) a##b##c##d\n";

    std::vector<std::string> files;

    std::istringstream istr_c(code_c);
    simplecpp::TokenList rawtokens_c(istr_c, files, "A.c");

    std::istringstream istr_h(code_h);
    simplecpp::TokenList rawtokens_h(istr_h, files, "A.h");

    ASSERT_EQUALS(2U, files.size());
    ASSERT_EQUALS("A.c", files[0]);
    ASSERT_EQUALS("A.h", files[1]);

    std::map<std::string, simplecpp::TokenList *> filedata;
    filedata["A.c"] = &rawtokens_c;
    filedata["A.h"] = &rawtokens_h;

    simplecpp::TokenList out(files);
    simplecpp::preprocess(out, rawtokens_c, files, filedata, simplecpp::DUI());

    ASSERT_EQUALS("\n1234", out.stringify());
}


static void include4()   // #27 - -include
{
    const char code_c[] = "X\n" ;
    const char code_h[] = "#define X 123\n";

    std::vector<std::string> files;

    std::istringstream istr_c(code_c);
    simplecpp::TokenList rawtokens_c(istr_c, files, "27.c");

    std::istringstream istr_h(code_h);
    simplecpp::TokenList rawtokens_h(istr_h, files, "27.h");

    ASSERT_EQUALS(2U, files.size());
    ASSERT_EQUALS("27.c", files[0]);
    ASSERT_EQUALS("27.h", files[1]);

    std::map<std::string, simplecpp::TokenList *> filedata;
    filedata["27.c"] = &rawtokens_c;
    filedata["27.h"] = &rawtokens_h;

    simplecpp::TokenList out(files);
    simplecpp::DUI dui;
    dui.includes.push_back("27.h");
    simplecpp::preprocess(out, rawtokens_c, files, filedata, dui);

    ASSERT_EQUALS("123", out.stringify());
}

static void include5()    // #3 - handle #include MACRO
{
    const char code_c[] = "#define A \"3.h\"\n#include A\n";
    const char code_h[] = "123\n";

    std::vector<std::string> files;
    std::istringstream istr_c(code_c);
    simplecpp::TokenList rawtokens_c(istr_c, files, "3.c");
    std::istringstream istr_h(code_h);
    simplecpp::TokenList rawtokens_h(istr_h, files, "3.h");

    std::map<std::string, simplecpp::TokenList *> filedata;
    filedata["3.c"] = &rawtokens_c;
    filedata["3.h"] = &rawtokens_h;

    simplecpp::TokenList out(files);
    simplecpp::DUI dui;
    simplecpp::preprocess(out, rawtokens_c, files, filedata, dui);

    ASSERT_EQUALS("\n#line 1 \"3.h\"\n123", out.stringify());
}

static void include6()   // #57 - incomplete macro  #include MACRO(,)
{
    const char code[] = "#define MACRO(X,Y) X##Y\n#include MACRO(,)\n";

    std::vector<std::string> files;
    std::istringstream istr(code);
    simplecpp::TokenList rawtokens(istr, files, "57.c");

    std::map<std::string, simplecpp::TokenList *> filedata;
    filedata["57.c"] = &rawtokens;

    simplecpp::TokenList out(files);
    simplecpp::DUI dui;
    simplecpp::preprocess(out, rawtokens, files, filedata, dui);
}

static void include7()    // #3 - handle #include MACRO with <>
{
    const char code_c[] = "#define A <3.h>\n#include A\n";
    const char code_h[] = "123\n";

    std::vector<std::string> files;
    std::istringstream istr_c(code_c);
    simplecpp::TokenList rawtokens_c(istr_c, files, "3.c");
    std::istringstream istr_h(code_h);
    simplecpp::TokenList rawtokens_h(istr_h, files, "3.h");

    std::map<std::string, simplecpp::TokenList *> filedata;
    filedata["3.c"] = &rawtokens_c;
    filedata["3.h"] = &rawtokens_h;

    simplecpp::TokenList out(files);
    simplecpp::DUI dui;
    simplecpp::preprocess(out, rawtokens_c, files, filedata, dui);

    ASSERT_EQUALS("\n#line 1 \"3.h\"\n123", out.stringify());
}

static void readfile_nullbyte()
{
    const char code[] = "ab\0cd";
    simplecpp::OutputList outputList;
    ASSERT_EQUALS("ab cd", readfile(code,sizeof(code), &outputList));
    ASSERT_EQUALS(true, outputList.empty()); // should warning be written?
}

static void readfile_string()
{
    const char code[] = "A = \"abc\'def\"";
    ASSERT_EQUALS("A = \"abc\'def\"", readfile(code));
    ASSERT_EQUALS("( \"\\\\\\\\\" )", readfile("(\"\\\\\\\\\")"));
    ASSERT_EQUALS("x = \"a  b\"\n;", readfile("x=\"a\\\n  b\";"));
    ASSERT_EQUALS("x = \"a  b\"\n;", readfile("x=\"a\\\r\n  b\";"));
}

static void readfile_rawstring()
{
    ASSERT_EQUALS("A = \"abc\\\\\\\\def\"", readfile("A = R\"(abc\\\\def)\""));
    ASSERT_EQUALS("A = \"abc\\\\\\\\def\"", readfile("A = R\"x(abc\\\\def)x\""));
    ASSERT_EQUALS("A = \"\"", readfile("A = R\"()\""));
    ASSERT_EQUALS("A = \"\\\\\"", readfile("A = R\"(\\)\""));
    ASSERT_EQUALS("A = \"\\\"\"", readfile("A = R\"(\")\""));
    ASSERT_EQUALS("A = \"abc\"", readfile("A = R\"\"\"(abc)\"\"\""));
    ASSERT_EQUALS("A = \"a\nb\nc\";", readfile("A = R\"foo(a\nb\nc)foo\";"));
    ASSERT_EQUALS("A = L \"abc\"", readfile("A = LR\"(abc)\""));
    ASSERT_EQUALS("A = u \"abc\"", readfile("A = uR\"(abc)\""));
    ASSERT_EQUALS("A = U \"abc\"", readfile("A = UR\"(abc)\""));
    ASSERT_EQUALS("A = u8 \"abc\"", readfile("A = u8R\"(abc)\""));
}

static void readfile_cpp14_number()
{
    ASSERT_EQUALS("A = 12345 ;", readfile("A = 12\'345;"));
}

static void readfile_unhandled_chars()
{
    simplecpp::OutputList outputList;
    readfile("// 你好世界", -1, &outputList);
    ASSERT_EQUALS("", toString(outputList));
    readfile("s=\"你好世界\"", -1, &outputList);
    ASSERT_EQUALS("", toString(outputList));
    readfile("int 你好世界=0;", -1, &outputList);
    ASSERT_EQUALS("file0,1,unhandled_char_error,The code contains unhandled character(s) (character code=228). Neither unicode nor extended ascii is supported.\n", toString(outputList));
}

static void stringify1()
{
    const char code_c[] = "#include \"A.h\"\n"
                          "#include \"A.h\"\n";
    const char code_h[] = "1\n2";

    std::vector<std::string> files;

    std::istringstream istr_c(code_c);
    simplecpp::TokenList rawtokens_c(istr_c, files, "A.c");

    std::istringstream istr_h(code_h);
    simplecpp::TokenList rawtokens_h(istr_h, files, "A.h");

    ASSERT_EQUALS(2U, files.size());
    ASSERT_EQUALS("A.c", files[0]);
    ASSERT_EQUALS("A.h", files[1]);

    std::map<std::string, simplecpp::TokenList *> filedata;
    filedata["A.c"] = &rawtokens_c;
    filedata["A.h"] = &rawtokens_h;

    simplecpp::TokenList out(files);
    simplecpp::preprocess(out, rawtokens_c, files, filedata, simplecpp::DUI());

    ASSERT_EQUALS("\n#line 1 \"A.h\"\n1\n2\n#line 1 \"A.h\"\n1\n2", out.stringify());
}

static void tokenMacro1()
{
    const char code[] = "#define A 123\n"
                        "A";
    const simplecpp::DUI dui;
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    std::istringstream istr(code);
    simplecpp::TokenList tokenList(files);
    simplecpp::preprocess(tokenList, simplecpp::TokenList(istr,files), files, filedata, dui);
    ASSERT_EQUALS("A", tokenList.cback()->macro);
}

static void tokenMacro2()
{
    const char code[] = "#define ADD(X,Y) X+Y\n"
                        "ADD(1,2)";
    const simplecpp::DUI dui;
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    std::istringstream istr(code);
    simplecpp::TokenList tokenList(files);
    simplecpp::preprocess(tokenList, simplecpp::TokenList(istr,files), files, filedata, dui);
    const simplecpp::Token *tok = tokenList.cfront();
    ASSERT_EQUALS("1", tok->str);
    ASSERT_EQUALS("", tok->macro);
    tok = tok->next;
    ASSERT_EQUALS("+", tok->str);
    ASSERT_EQUALS("ADD", tok->macro);
    tok = tok->next;
    ASSERT_EQUALS("2", tok->str);
    ASSERT_EQUALS("", tok->macro);
}

static void tokenMacro3()
{
    const char code[] = "#define ADD(X,Y) X+Y\n"
                        "#define FRED  1\n"
                        "ADD(FRED,2)";
    const simplecpp::DUI dui;
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    std::istringstream istr(code);
    simplecpp::TokenList tokenList(files);
    simplecpp::preprocess(tokenList, simplecpp::TokenList(istr,files), files, filedata, dui);
    const simplecpp::Token *tok = tokenList.cfront();
    ASSERT_EQUALS("1", tok->str);
    ASSERT_EQUALS("FRED", tok->macro);
    tok = tok->next;
    ASSERT_EQUALS("+", tok->str);
    ASSERT_EQUALS("ADD", tok->macro);
    tok = tok->next;
    ASSERT_EQUALS("2", tok->str);
    ASSERT_EQUALS("", tok->macro);
}

static void tokenMacro4()
{
    const char code[] = "#define A B\n"
                        "#define B 1\n"
                        "A";
    const simplecpp::DUI dui;
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    std::istringstream istr(code);
    simplecpp::TokenList tokenList(files);
    simplecpp::preprocess(tokenList, simplecpp::TokenList(istr,files), files, filedata, dui);
    const simplecpp::Token *tok = tokenList.cfront();
    ASSERT_EQUALS("1", tok->str);
    ASSERT_EQUALS("A", tok->macro);
}

static void undef()
{
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

static void userdef()
{
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

static void utf8()
{
    ASSERT_EQUALS("123", readfile("\xEF\xBB\xBF 123"));
}

static void unicode()
{
    ASSERT_EQUALS("12", readfile("\xFE\xFF\x00\x31\x00\x32", 6));
    ASSERT_EQUALS("12", readfile("\xFF\xFE\x31\x00\x32\x00", 6));
    ASSERT_EQUALS("\n//1", readfile("\xff\xfe\x0d\x00\x0a\x00\x2f\x00\x2f\x00\x31\x00\x0d\x00\x0a\x00",16));
}

static void warning()
{
    std::istringstream istr("#warning MSG\n1");
    std::vector<std::string> files;
    std::map<std::string, simplecpp::TokenList*> filedata;
    simplecpp::OutputList outputList;
    simplecpp::TokenList tokens2(files);
    simplecpp::preprocess(tokens2, simplecpp::TokenList(istr,files,"test.c"), files, filedata, simplecpp::DUI(), &outputList);
    ASSERT_EQUALS("\n1", tokens2.stringify());
    ASSERT_EQUALS("file0,1,#warning,#warning MSG\n", toString(outputList));
}

static void simplifyPath()
{
    ASSERT_EQUALS("1.c", simplecpp::simplifyPath("./1.c"));
    ASSERT_EQUALS("1.c", simplecpp::simplifyPath("././1.c"));
    ASSERT_EQUALS("/1.c", simplecpp::simplifyPath("/./1.c"));
    ASSERT_EQUALS("/1.c", simplecpp::simplifyPath("/././1.c"));
    ASSERT_EQUALS("trailing_dot./1.c", simplecpp::simplifyPath("trailing_dot./1.c"));

    ASSERT_EQUALS("1.c", simplecpp::simplifyPath("a/../1.c"));
    ASSERT_EQUALS("1.c", simplecpp::simplifyPath("a/b/../../1.c"));
    ASSERT_EQUALS("a/1.c", simplecpp::simplifyPath("a/b/../1.c"));
    ASSERT_EQUALS("/a/1.c", simplecpp::simplifyPath("/a/b/../1.c"));
    ASSERT_EQUALS("/a/1.c", simplecpp::simplifyPath("/a/b/c/../../1.c"));
    ASSERT_EQUALS("/a/1.c", simplecpp::simplifyPath("/a/b/c/../.././1.c"));

    ASSERT_EQUALS("../1.c", simplecpp::simplifyPath("../1.c"));
    ASSERT_EQUALS("../1.c", simplecpp::simplifyPath("../a/../1.c"));
    ASSERT_EQUALS("/../1.c", simplecpp::simplifyPath("/../1.c"));
    ASSERT_EQUALS("/../1.c", simplecpp::simplifyPath("/../a/../1.c"));

    ASSERT_EQUALS("a/..b/1.c", simplecpp::simplifyPath("a/..b/1.c"));
    ASSERT_EQUALS("../../1.c", simplecpp::simplifyPath("../../1.c"));
    ASSERT_EQUALS("../../../1.c", simplecpp::simplifyPath("../../../1.c"));
    ASSERT_EQUALS("../../../1.c", simplecpp::simplifyPath("../../../a/../1.c"));
    ASSERT_EQUALS("../../1.c", simplecpp::simplifyPath("a/../../../1.c"));
}

// tests transferred from cppcheck
// https://github.com/danmar/cppcheck/blob/d3e79b71b5ec6e641ca3e516cfced623b27988af/test/testpath.cpp#L43
static void simplifyPath_cppcheck()
{
    ASSERT_EQUALS("index.h", simplecpp::simplifyPath("index.h"));
    ASSERT_EQUALS("index.h", simplecpp::simplifyPath("./index.h"));
    ASSERT_EQUALS("index.h", simplecpp::simplifyPath(".//index.h"));
    ASSERT_EQUALS("index.h", simplecpp::simplifyPath(".///index.h"));
    ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/index.h"));
    ASSERT_EQUALS("/path/", simplecpp::simplifyPath("/path/"));
    ASSERT_EQUALS("/", simplecpp::simplifyPath("/"));
    ASSERT_EQUALS("/", simplecpp::simplifyPath("/."));
    ASSERT_EQUALS("/", simplecpp::simplifyPath("/./"));
    ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/./index.h"));
    ASSERT_EQUALS("/", simplecpp::simplifyPath("/.//"));
    ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/.//index.h"));
    ASSERT_EQUALS("../index.h", simplecpp::simplifyPath("../index.h"));
    ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path/../index.h"));
    ASSERT_EQUALS("index.h", simplecpp::simplifyPath("./path/../index.h"));
    ASSERT_EQUALS("index.h", simplecpp::simplifyPath("path/../index.h"));
    ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path//../index.h"));
    ASSERT_EQUALS("index.h", simplecpp::simplifyPath("./path//../index.h"));
    ASSERT_EQUALS("index.h", simplecpp::simplifyPath("path//../index.h"));
    ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path/..//index.h"));
    ASSERT_EQUALS("index.h", simplecpp::simplifyPath("./path/..//index.h"));
    ASSERT_EQUALS("index.h", simplecpp::simplifyPath("path/..//index.h"));
    ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path//..//index.h"));
    ASSERT_EQUALS("index.h", simplecpp::simplifyPath("./path//..//index.h"));
    ASSERT_EQUALS("index.h", simplecpp::simplifyPath("path//..//index.h"));
    ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path/../other/../index.h"));
    ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path/../other///././../index.h"));
    ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path/../other/././..///index.h"));
    ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path/../other///././..///index.h"));
    ASSERT_EQUALS("../path/index.h", simplecpp::simplifyPath("../path/other/../index.h"));
    ASSERT_EQUALS("a/index.h", simplecpp::simplifyPath("a/../a/index.h"));
    ASSERT_EQUALS(".", simplecpp::simplifyPath("a/.."));
    ASSERT_EQUALS(".", simplecpp::simplifyPath("./a/.."));
    ASSERT_EQUALS("../../src/test.cpp", simplecpp::simplifyPath("../../src/test.cpp"));
    ASSERT_EQUALS("../../../src/test.cpp", simplecpp::simplifyPath("../../../src/test.cpp"));
    ASSERT_EQUALS("src/test.cpp", simplecpp::simplifyPath(".//src/test.cpp"));
    ASSERT_EQUALS("src/test.cpp", simplecpp::simplifyPath(".///src/test.cpp"));
    ASSERT_EQUALS("test.cpp", simplecpp::simplifyPath("./././././test.cpp"));
    ASSERT_EQUALS("src/", simplecpp::simplifyPath("src/abc/.."));
    ASSERT_EQUALS("src/", simplecpp::simplifyPath("src/abc/../"));

    // Handling of UNC paths on Windows
    ASSERT_EQUALS("//src/test.cpp", simplecpp::simplifyPath("//src/test.cpp"));
    ASSERT_EQUALS("//src/test.cpp", simplecpp::simplifyPath("///src/test.cpp"));
}

static void simplifyPath_New()
{
    ASSERT_EQUALS("", simplecpp::simplifyPath(""));
    ASSERT_EQUALS("/", simplecpp::simplifyPath("/"));
    ASSERT_EQUALS("//", simplecpp::simplifyPath("//"));
    ASSERT_EQUALS("//", simplecpp::simplifyPath("///"));
    ASSERT_EQUALS("/", simplecpp::simplifyPath("\\"));
}

static void preprocessSizeOf()
{
    simplecpp::OutputList outputList;

    preprocess("#if 3 > sizeof", simplecpp::DUI(), &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,failed to evaluate #if condition, missing sizeof argument\n", toString(outputList));

    outputList.clear();

    preprocess("#if 3 > sizeof A", simplecpp::DUI(), &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,failed to evaluate #if condition, missing sizeof argument\n", toString(outputList));

    outputList.clear();

    preprocess("#if 3 > sizeof(int", simplecpp::DUI(), &outputList);
    ASSERT_EQUALS("file0,1,syntax_error,failed to evaluate #if condition, invalid sizeof expression\n", toString(outputList));
}

int main(int argc, char **argv)
{
    TEST_CASE(backslash);

    TEST_CASE(builtin);

    TEST_CASE(combineOperators_floatliteral);
    TEST_CASE(combineOperators_increment);
    TEST_CASE(combineOperators_coloncolon);

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
    TEST_CASE(define_invalid_1);
    TEST_CASE(define_invalid_2);
    TEST_CASE(define_define_1);
    TEST_CASE(define_define_2);
    TEST_CASE(define_define_3);
    TEST_CASE(define_define_4);
    TEST_CASE(define_define_5);
    TEST_CASE(define_define_6);
    TEST_CASE(define_define_7);
    TEST_CASE(define_define_8); // line break in nested macro call
    TEST_CASE(define_define_9); // line break in nested macro call
    TEST_CASE(define_define_10);
    TEST_CASE(define_define_11);
    TEST_CASE(define_define_12); // expand result of ##
    TEST_CASE(define_va_args_1);
    TEST_CASE(define_va_args_2);
    TEST_CASE(define_va_args_3);

    TEST_CASE(dollar);

    TEST_CASE(dotDotDot); // ...

    TEST_CASE(error);

    TEST_CASE(garbage);
    TEST_CASE(garbage_endif);

    TEST_CASE(hash);
    TEST_CASE(hashhash1);
    TEST_CASE(hashhash2);
    TEST_CASE(hashhash3);
    TEST_CASE(hashhash4);
    TEST_CASE(hashhash5);
    TEST_CASE(hashhash6);
    TEST_CASE(hashhash7); // # ## #  (C standard; 6.10.3.3.p4)
    TEST_CASE(hashhash8);

    TEST_CASE(ifdef1);
    TEST_CASE(ifdef2);
    TEST_CASE(ifndef);
    TEST_CASE(ifA);
    TEST_CASE(ifCharLiteral);
    TEST_CASE(ifDefined);
    TEST_CASE(ifDefinedNoPar);
    TEST_CASE(ifDefinedNested);
    TEST_CASE(ifDefinedNestedNoPar);
    TEST_CASE(ifDefinedInvalid1);
    TEST_CASE(ifDefinedInvalid2);
    TEST_CASE(ifLogical);
    TEST_CASE(ifSizeof);
    TEST_CASE(elif);
    TEST_CASE(ifif);
    TEST_CASE(ifoverflow);
    TEST_CASE(ifdiv0);
    TEST_CASE(ifalt); // using "and", "or", etc

    TEST_CASE(missingHeader1);
    TEST_CASE(missingHeader2);
    TEST_CASE(missingHeader3);
    TEST_CASE(nestedInclude);

    TEST_CASE(include1);
    TEST_CASE(include2);
    TEST_CASE(include3);
    TEST_CASE(include4); // -include
    TEST_CASE(include5); // #include MACRO
    TEST_CASE(include6); // invalid code: #include MACRO(,)
    TEST_CASE(include7); // #include MACRO with <>

    TEST_CASE(multiline1);
    TEST_CASE(multiline2);
    TEST_CASE(multiline3);
    TEST_CASE(multiline4);
    TEST_CASE(multiline5); // column

    TEST_CASE(readfile_nullbyte);
    TEST_CASE(readfile_string);
    TEST_CASE(readfile_rawstring);
    TEST_CASE(readfile_cpp14_number);
    TEST_CASE(readfile_unhandled_chars);

    TEST_CASE(stringify1);

    TEST_CASE(tokenMacro1);
    TEST_CASE(tokenMacro2);
    TEST_CASE(tokenMacro3);
    TEST_CASE(tokenMacro4);

    TEST_CASE(undef);

    TEST_CASE(userdef);

    // utf/unicode
    TEST_CASE(utf8);
    TEST_CASE(unicode);

    TEST_CASE(warning);

    // utility functions.
    TEST_CASE(simplifyPath);
    TEST_CASE(simplifyPath_cppcheck);
    TEST_CASE(simplifyPath_New);

    TEST_CASE(preprocessSizeOf);

    return numberOfFailedAssertions > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

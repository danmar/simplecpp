
#include "simplecpp.h"

#include <fstream>
#include <iostream>
#include <cstring>

int main(int argc, char **argv)
{
    const char *filename = NULL;

    // Settings..
    simplecpp::DUI dui;
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (*arg == '-') {
            char c = arg[1];
            if (c != 'D' && c != 'U' && c != 'I' && c != 'i')
                continue;  // Ignored
            const char *value = arg[2] ? (argv[i] + 2) : argv[++i];
            switch (c) {
            case 'D': // define symbol
                dui.defines.push_back(value);
                break;
            case 'U': // undefine symbol
                dui.undefined.insert(value);
                break;
            case 'I': // include path
                dui.includePaths.push_back(value);
                break;
            case 'i':
                if (std::strncmp(arg, "-include=",9)==0)
                    dui.includes.push_back(arg+9);
                break;
            };
        } else {
            filename = arg;
        }
    }

    if (!filename) {
        std::cout << "Syntax:" << std::endl;
        std::cout << "simplecpp [options] filename" << std::endl;
        std::cout << "  -DNAME          Define NAME." << std::endl;
        std::cout << "  -IPATH          Include path." << std::endl;
        std::cout << "  -include=FILE   Include FILE." << std::endl;
        std::cout << "  -UNAME          Undefine NAME." << std::endl;
        std::exit(0);
    }

    // Perform preprocessing
    simplecpp::OutputList outputList;
    std::vector<std::string> files;
    std::ifstream f(filename);
    simplecpp::TokenList rawtokens(f,files,filename,&outputList);
    rawtokens.removeComments();
    std::map<std::string, simplecpp::TokenList*> included = simplecpp::load(rawtokens, files, dui, &outputList);
    for (std::pair<std::string, simplecpp::TokenList *> i : included)
        i.second->removeComments();
    simplecpp::TokenList outputTokens(files);
    simplecpp::preprocess(outputTokens, rawtokens, files, included, dui, &outputList);

    // Output
    std::cout << outputTokens.stringify() << std::endl;
    for (const simplecpp::Output &output : outputList) {
        std::cerr << output.location.file() << ':' << output.location.line << ": ";
        switch (output.type) {
        case simplecpp::Output::ERROR:
            std::cerr << "#error: ";
            break;
        case simplecpp::Output::WARNING:
            std::cerr << "#warning: ";
            break;
        case simplecpp::Output::MISSING_HEADER:
            std::cerr << "missing header: ";
            break;
        case simplecpp::Output::INCLUDE_NESTED_TOO_DEEPLY:
            std::cerr << "include nested too deeply: ";
            break;
        case simplecpp::Output::SYNTAX_ERROR:
            std::cerr << "syntax error: ";
            break;
        case simplecpp::Output::PORTABILITY_BACKSLASH:
            std::cerr << "portability: ";
            break;
        case simplecpp::Output::UNHANDLED_CHAR_ERROR:
            std::cerr << "unhandled char error: ";
            break;
        }
        std::cerr << output.msg << std::endl;
    }

    // cleanup included tokenlists
    simplecpp::cleanup(included);

    return 0;
}

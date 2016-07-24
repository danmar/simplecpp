
#include "simplecpp.h"

#include <fstream>
#include <iostream>


int main(int argc, char **argv) {
    const char *filename = NULL;

    // Settings..
    struct simplecpp::DUI dui;
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (*arg == '-') {
            char c = arg[1];
            if (c != 'D' && c != 'U' && c != 'I')
                continue;  // Ignored
            const char *value = arg[2] ? (argv[i] + 2) : argv[++i];
            switch (c) {
            case 'D':
                dui.defines.push_back(value);
                break;
            case 'U':
                dui.undefined.insert(value);
                break;
            case 'I':
                dui.includePaths.push_back(value);
                break;
            };
        } else {
            filename = arg;
        }
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
    simplecpp::TokenList output(files);
    simplecpp::preprocess(output, rawtokens, files, included, dui, &outputList);

    // Output
    std::cout << output.stringify() << std::endl;
    for (const simplecpp::Output &output : outputList) {
        std::cerr << output.location.file() << ':' << output.location.line << ": ";
        switch (output.type) {
        case simplecpp::Output::ERROR:
            std::cerr << "error: ";
            break;
        case simplecpp::Output::WARNING:
            std::cerr << "warning: ";
            break;
        case simplecpp::Output::MISSING_INCLUDE:
            std::cerr << "missing include: ";
            break;
        }
        std::cerr << output.msg << std::endl;
    }

    return 0;
}

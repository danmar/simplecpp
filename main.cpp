/*
 * simplecpp - A simple and high-fidelity C/C++ preprocessor library
 * Copyright (C) 2016-2022 Daniel Marjamäki.
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "simplecpp.h"

#include <fstream>
#include <iostream>
#include <cstring>

int main(int argc, char **argv)
{
    bool error = false;
    const char *filename = nullptr;
    bool use_istream = false;

    // Settings..
    simplecpp::DUI dui;
    bool quiet = false;
    bool error_only = false;
    for (int i = 1; i < argc; i++) {
        const char * const arg = argv[i];
        if (*arg == '-') {
            bool found = false;
            const char c = arg[1];
            // TODO: treats every additional parameter as value requiring the options to be specified after the filename
            const char * const value = arg[2] ? (argv[i] + 2) : argv[++i];
            switch (c) {
            case 'D': // define symbol
                dui.defines.push_back(value);
                found = true;
                break;
            case 'U': // undefine symbol
                dui.undefined.insert(value);
                found = true;
                break;
            case 'I': // include path
                dui.includePaths.push_back(value);
                found = true;
                break;
            case 'i':
                if (std::strncmp(arg, "-include=",9)==0) {
                    dui.includes.push_back(arg+9);
                    found = true;
                }
                else if (std::strncmp(arg, "-is",3)==0) {
                    use_istream = true;
                    found = true;
                }
                break;
            case 's':
                if (std::strncmp(arg, "-std=",5)==0) {
                    dui.std = arg + 5;
                    found = true;
                }
                break;
            case 'q':
                quiet = true;
                found = true;
                break;
            case 'e':
                error_only = true;
                found = true;
                break;
            }
            if (!found) {
                std::cout << "error: option '" << arg << "' is unknown." << std::endl;
                error = true;
            }
        } else if (filename) {
            std::cout << "error: multiple filenames specified" << std::endl;
            std::exit(1);
        } else {
            filename = arg;
        }
    }

    if (error)
        std::exit(1);

    if (quiet && error_only) {
        std::cout << "error: -e cannot be used in conjunction with -q" << std::endl;
        std::exit(1);
    }

    if (!filename) {
        std::cout << "Syntax:" << std::endl;
        std::cout << "simplecpp filename [options]" << std::endl;
        std::cout << "  -DNAME          Define NAME." << std::endl;
        std::cout << "  -IPATH          Include path." << std::endl;
        std::cout << "  -include=FILE   Include FILE." << std::endl;
        std::cout << "  -UNAME          Undefine NAME." << std::endl;
        std::cout << "  -std=STD        Specify standard." << std::endl;
        std::cout << "  -q              Quiet mode (no output)." << std::endl;
        std::cout << "  -is             Use std::istream interface." << std::endl;
        std::cout << "  -e              Output errors only." << std::endl;
        std::exit(0);
    }

    // Perform preprocessing
    simplecpp::OutputList outputList;
    std::vector<std::string> files;
    simplecpp::TokenList *rawtokens;
    if (use_istream) {
        std::ifstream f(filename);
        if (!f.is_open()) {
            std::cout << "error: could not open file '" << filename << "'" << std::endl;
            std::exit(1);
        }
        rawtokens = new simplecpp::TokenList(f, files,filename,&outputList);
    }
    else {
        rawtokens = new simplecpp::TokenList(filename,files,&outputList);
    }
    rawtokens->removeComments();
    std::map<std::string, simplecpp::TokenList*> included = simplecpp::load(*rawtokens, files, dui, &outputList);
    for (std::pair<std::string, simplecpp::TokenList *> i : included)
        i.second->removeComments();
    simplecpp::TokenList outputTokens(files);
    simplecpp::preprocess(outputTokens, *rawtokens, files, included, dui, &outputList);
    delete rawtokens;
    rawtokens = nullptr;

    // Output
    if (!quiet) {
        if (!error_only)
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
            case simplecpp::Output::EXPLICIT_INCLUDE_NOT_FOUND:
                std::cerr << "explicit include not found: ";
                break;
            }
            std::cerr << output.msg << std::endl;
        }
    }

    // cleanup included tokenlists
    simplecpp::cleanup(included);

    return 0;
}

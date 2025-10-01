/*
 * simplecpp - A simple and high-fidelity C/C++ preprocessor library
 * Copyright (C) 2016-2023 simplecpp team
 */

#include "simplecpp.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <string>
#include <utility>
#include <vector>

static bool isDir(const std::string& path)
{
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) == -1)
        return false;

    return (file_stat.st_mode & S_IFMT) == S_IFDIR;
}

int main(int argc, char **argv)
{
    bool error = false;
    const char *filename = nullptr;
    bool use_istream = false;
    bool fail_on_error = false;
    bool linenrs = false;

    // Settings..
    simplecpp::DUI dui;
    dui.removeComments = true;
    bool quiet = false;
    bool error_only = false;
    for (int i = 1; i < argc; i++) {
        const char * const arg = argv[i];
        if (*arg == '-') {
            bool found = false;
            const char c = arg[1];
            switch (c) {
            case 'D': { // define symbol
                found = true;
                const char * const value = arg[2] ? (argv[i] + 2) : argv[++i];
                if (!value) {
                    std::cout << "error: option -D with no value." << std::endl;
                    error = true;
                    break;
                }
                dui.defines.push_back(value);
                break;
            }
            case 'U': { // undefine symbol
                found = true;
                const char * const value = arg[2] ? (argv[i] + 2) : argv[++i];
                if (!value) {
                    std::cout << "error: option -U with no value." << std::endl;
                    error = true;
                    break;
                }
                dui.undefined.insert(value);
                break;
            }
            case 'I': { // include path
                found = true;
                const char * const value = arg[2] ? (arg + 2) : argv[++i];
                if (!value) {
                    std::cout << "error: option -I with no value." << std::endl;
                    error = true;
                    break;
                }
                dui.includePaths.push_back(value);
                break;
            }
            case 'i':
                if (std::strncmp(arg, "-include=",9)==0) {
                    found = true;
                    std::string value = arg + 9;
                    if (value.empty()) {
                        std::cout << "error: option -include with no value." << std::endl;
                        error = true;
                        break;
                    }
                    dui.includes.push_back(std::move(value));
                } else if (std::strncmp(arg, "-is",3)==0) {
                    found = true;
                    use_istream = true;
                }
                break;
            case 's':
                if (std::strncmp(arg, "-std=",5)==0) {
                    found = true;
                    std::string value = arg + 5;
                    if (value.empty()) {
                        std::cout << "error: option -std with no value." << std::endl;
                        error = true;
                        break;
                    }
                    dui.std = std::move(value);
                }
                break;
            case 'q':
                found = true;
                quiet = true;
                break;
            case 'e':
                found = true;
                error_only = true;
                break;
            case 'f':
                found = true;
                fail_on_error = true;
                break;
            case 'l':
                linenrs = true;
                found = true;
                break;
            }
            if (!found) {
                std::cout << "error: option '" << arg << "' is unknown." << std::endl;
                error = true;
            }
        } else if (filename) {
            std::cout << "error: multiple filenames specified" << std::endl;
            return 1;
        } else {
            filename = arg;
        }
    }

    if (error)
        return 1;

    if (quiet && error_only) {
        std::cout << "error: -e cannot be used in conjunction with -q" << std::endl;
        return 1;
    }

    if (!filename) {
        std::cout << "Syntax:" << std::endl;
        std::cout << "simplecpp [options] filename" << std::endl;
        std::cout << "  -DNAME          Define NAME." << std::endl;
        std::cout << "  -IPATH          Include path." << std::endl;
        std::cout << "  -include=FILE   Include FILE." << std::endl;
        std::cout << "  -UNAME          Undefine NAME." << std::endl;
        std::cout << "  -std=STD        Specify standard." << std::endl;
        std::cout << "  -q              Quiet mode (no output)." << std::endl;
        std::cout << "  -is             Use std::istream interface." << std::endl;
        std::cout << "  -e              Output errors only." << std::endl;
        std::cout << "  -f              Fail when errors were encountered (exitcode 1)." << std::endl;
        std::cout << "  -l              Print lines numbers." << std::endl;
        return 0;
    }

    // TODO: move this logic into simplecpp
    bool inp_missing = false;

    for (const std::string& inc : dui.includes) {
        std::ifstream f(inc);
        if (!f.is_open() || isDir(inc)) {
            inp_missing = true;
            std::cout << "error: could not open include '" << inc << "'" << std::endl;
        }
    }

    for (const std::string& inc : dui.includePaths) {
        if (!isDir(inc)) {
            inp_missing = true;
            std::cout << "error: could not find include path '" << inc << "'" << std::endl;
        }
    }

    std::ifstream f(filename);
    if (!f.is_open() || isDir(filename)) {
        inp_missing = true;
        std::cout << "error: could not open file '" << filename << "'" << std::endl;
    }

    if (inp_missing)
        return 1;

    // Perform preprocessing
    simplecpp::OutputList outputList;
    std::vector<std::string> files;
    simplecpp::TokenList outputTokens(files);
    {
        simplecpp::TokenList *rawtokens;
        if (use_istream) {
            rawtokens = new simplecpp::TokenList(f, files,filename,&outputList);
        } else {
            f.close();
            rawtokens = new simplecpp::TokenList(filename,files,&outputList);
        }
        rawtokens->removeComments();
        simplecpp::FileDataCache filedata;
        simplecpp::preprocess(outputTokens, *rawtokens, files, filedata, dui, &outputList);
        simplecpp::cleanup(filedata);
        delete rawtokens;
    }

    // Output
    if (!quiet) {
        if (!error_only)
            std::cout << outputTokens.stringify(linenrs) << std::endl;

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
            case simplecpp::Output::FILE_NOT_FOUND:
                std::cerr << "file not found: ";
                break;
            case simplecpp::Output::DUI_ERROR:
                std::cerr << "dui error: ";
                break;
            }
            std::cerr << output.msg << std::endl;
        }
    }

    if (fail_on_error && !outputList.empty())
        return 1;

    return 0;
}

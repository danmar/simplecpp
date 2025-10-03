/*
 * simplecpp - A simple and high-fidelity C/C++ preprocessor library
 * Copyright (C) 2016-2024 simplecpp team
 */

#include "simplecpp.h"

#include <cstdint>

#ifdef NO_FUZZ
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#endif

static void doProcess(const uint8_t *data, size_t dataSize)
{
    simplecpp::OutputList outputList;
    std::vector<std::string> files;
    simplecpp::TokenList rawtokens(data, dataSize, files, "test.cpp", &outputList);

    simplecpp::TokenList outputTokens(files);
    simplecpp::FileDataCache filedata;
    simplecpp::DUI dui;
    dui.includePaths = { "/usr/include" };
    std::list<simplecpp::MacroUsage> macroUsage;
    std::list<simplecpp::IfCond> ifCond;
    simplecpp::preprocess(outputTokens, rawtokens, files, filedata, dui, &outputList, &macroUsage, &ifCond);

    simplecpp::cleanup(filedata);
}

#ifndef NO_FUZZ
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t dataSize);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t dataSize)
{
    doProcess(data, dataSize);
    return 0;
}
#else
int main(int argc, char * argv[])
{
    if (argc < 2 || argc > 3)
        return EXIT_FAILURE;

    std::ifstream f(argv[1]);
    if (!f.is_open())
        return EXIT_FAILURE;

    std::ostringstream oss;
    oss << f.rdbuf();

    if (!f.good())
        return EXIT_FAILURE;

    const int cnt = (argc == 3) ? std::stoi(argv[2]) : 1;

    const std::string code = oss.str();
    for (int i = 0; i < cnt; ++i)
        doProcess(reinterpret_cast<const uint8_t*>(code.data()), code.size());

    return EXIT_SUCCESS;
}
#endif

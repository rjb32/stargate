#include <stdlib.h>
#include <iostream>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include "VerilogDriver.h"

#include "FatalException.h"

using namespace stargate;
using namespace argparse;

constexpr const char* SGCPARSE_NAME = "sgcparse";

static void parseDefine(VerilogDriver* drv, const std::string& spec) {
    const size_t eq = spec.find('=');
    if (eq == std::string::npos) {
        drv->defineMacro(spec, "");
    } else {
        drv->defineMacro(spec.substr(0, eq), spec.substr(eq + 1));
    }
}

int main(int argc, char** argv) {
    ArgumentParser argParser(SGCPARSE_NAME);

    std::vector<std::string> inputs;
    std::vector<std::string> includeDirs;
    std::vector<std::string> defines;

    argParser.add_argument("files")
        .nargs(argparse::nargs_pattern::at_least_one)
        .metavar("file.v")
        .help("Verilog source file(s) to parse")
        .store_into(inputs);

    argParser.add_argument("-I", "--include")
        .append()
        .default_value(std::vector<std::string>{})
        .metavar("dir")
        .help("Add a directory to the `include search path")
        .store_into(includeDirs);

    argParser.add_argument("-D", "--define")
        .append()
        .default_value(std::vector<std::string>{})
        .metavar("NAME[=BODY]")
        .help("Predefine a Verilog macro")
        .store_into(defines);

    argParser.add_argument("-E", "--preprocess-only")
        .nargs(0)
        .default_value(false)
        .implicit_value(true)
        .help("Run only the preprocessor and emit the result to stdout");

    argParser.add_argument("--trace")
        .nargs(0)
        .default_value(false)
        .implicit_value(true)
        .help("Enable bison's debug trace");

    try {
        argParser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        spdlog::error("{}", err.what());
        std::cerr << argParser;
        return EXIT_FAILURE;
    }

    const bool isTrace = argParser.get<bool>("--trace");
    const bool preprocessOnly = argParser.get<bool>("--preprocess-only");

    int failureCount = 0;

    try {
        for (const auto& path : inputs) {
            VerilogDriver drv;
            drv.setTrace(isTrace);
            for (const auto& dir : includeDirs) {
                drv.addIncludeDir(dir);
            }
            for (const auto& d : defines) {
                parseDefine(&drv, d);
            }

            int rc = 0;
            if (preprocessOnly) {
                std::string processed;
                rc = drv.preprocessFile(path, &processed);
                if (rc == 0) {
                    std::cout << processed;
                }
            } else {
                rc = drv.parseFile(path);
            }

            for (const auto& msg : drv.errors()) {
                std::cerr << path << ": " << msg << std::endl;
            }

            if (rc != 0) {
                spdlog::error("parse failed: {}", path);
                failureCount++;
            } else if (!preprocessOnly) {
                spdlog::info("parse ok: {}", path);
            }
        }
    } catch (const FatalException& e) {
        spdlog::error("{}", e.what());
        return EXIT_FAILURE;
    }

    return failureCount == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

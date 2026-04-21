#include <stdlib.h>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include "SGCDist.h"

#include "FatalException.h"

using namespace stargate;
using namespace argparse;

constexpr const char* SGCDIST_NAME = "sgcdist";

int main(int argc, char** argv) {
    ArgumentParser argParser(SGCDIST_NAME);
    std::string commandScriptPath;
    std::string distribConfigPath;

    argParser.add_argument("command_script")
        .metavar("command.sh")
        .help("Path to the command script to execute")
        .store_into(commandScriptPath);

    argParser.add_argument("-config")
        .nargs(1)
        .default_value("")
        .metavar("distrib.toml")
        .help("Path to the distribution config file")
        .store_into(distribConfigPath);

    try {
        argParser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        spdlog::error("{}", err.what());
        return EXIT_FAILURE;
    }

    try {
        SGCDist sgcdist;
        sgcdist.setCommandScriptPath(commandScriptPath);
        sgcdist.setDistribConfigPath(distribConfigPath);
        return sgcdist.exec();

    } catch (const FatalException& e) {
        spdlog::error("{}", e.what());
        return EXIT_FAILURE;
    }
}

#include <stdlib.h>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include "CoreConfig.h"

using namespace stargate;
using namespace argparse;

constexpr const char* STARGATE_NAME = "stargate";

int main(int argc, char** argv) {
    CoreConfig config;

    ArgumentParser argParser(STARGATE_NAME);

    argParser.add_argument("-c", "--config")
    .metavar("stargate.yaml")
    .action([&](const std::string& value) {
        config.setConfigPath(value);
    })
    .help("Path to the config file");

    try {
        argParser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        spdlog::error("{}", err.what());
        return EXIT_FAILURE;
    }

    config.readConfig();

    return EXIT_SUCCESS;
}

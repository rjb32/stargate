#include <stdlib.h>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include "Stargate.h"

#include "StargateConfig.h"
#include "CoreConfig.h"

using namespace stargate;
using namespace argparse;

constexpr const char* STARGATE_NAME = "stargate";

int main(int argc, char** argv) {
    StargateConfig stargateConfig;
    CoreConfig coreConfig;

    ArgumentParser argParser(STARGATE_NAME);

    argParser.add_argument("-c", "--config")
    .metavar("stargate.yaml")
    .action([&](const std::string& value) {
        coreConfig.setConfigPath(value);
    })
    .help("Path to the config file");

    argParser.add_argument("-o", "-out_dir")
    .metavar("stargate.out")
    .action([&](const std::string& value) {
        stargateConfig.setStargateDir(value);
    })
    .help("Path to the output directory");

    try {
        argParser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        spdlog::error("{}", err.what());
        return EXIT_FAILURE;
    }

    Stargate stargate(stargateConfig);
    stargate.run();

    return EXIT_SUCCESS;
}

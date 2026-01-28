#include <stdlib.h>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include "Stargate.h"

#include "StargateConfig.h"
#include "ProjectConfig.h"

#include "FatalException.h"
#include "FileUtils.h"

using namespace stargate;
using namespace argparse;

constexpr const char* STARGATE_NAME = "stargate";

int main(int argc, char** argv) {
    // Parse arguments
    ArgumentParser argParser(STARGATE_NAME);
    std::string configFilePath;
    std::string outDirPath;
    bool isVerbose = false;

    argParser.add_argument("-c", "-config")
        .nargs(1)
        .default_value("")
        .metavar("stargate.toml")
        .help("Path to the config file")
        .store_into(configFilePath);

    argParser.add_argument("-o", "-out_dir")
        .nargs(1)
        .default_value("")
        .metavar("stargate.out")
        .help("Path to the output directory")
        .store_into(outDirPath);

    argParser.add_argument("--verbose")
        .nargs(0)
        .help("Set stargate into verbose mode")
        .store_into(isVerbose);

    // Add clean subcommand
    ArgumentParser cleanParser("clean");
    cleanParser.add_description("Remove the stargate output directory");
    std::string cleanOutDirPath;
    cleanParser.add_argument("-o", "-out_dir")
        .nargs(1)
        .default_value("")
        .metavar("sgc.out")
        .help("Path to the output directory to clean")
        .store_into(cleanOutDirPath);
    argParser.add_subparser(cleanParser);

    try {
        argParser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        spdlog::error("{}", err.what());
        return EXIT_FAILURE;
    }


    try {
        StargateConfig stargateConfig;
        ProjectConfig projectConfig;

        Stargate stargate(stargateConfig);

        if (!outDirPath.empty()) {
            stargateConfig.setStargateDir(outDirPath);
        }

        if (argParser.is_subcommand_used("clean")) {
            stargate.clean();
            return EXIT_SUCCESS;
        }

        // Read project config
        if (!configFilePath.empty()) {
            projectConfig.setConfigPath(configFilePath);
        }

        projectConfig.setVerbose(isVerbose);
        projectConfig.readConfig();

        stargate.run(&projectConfig);

    } catch (const FatalException& e) {
        spdlog::error("{}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

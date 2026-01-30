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
constexpr const char* DEFAULT_TARGET = "default";

int main(int argc, char** argv) {
    // Parse arguments
    ArgumentParser argParser(STARGATE_NAME);
    std::string configFilePath;
    std::string outDirPath;
    std::string targetName = DEFAULT_TARGET;
    std::string taskName;
    std::string startTaskName;
    std::string endTaskName;
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

    argParser.add_argument("-target")
        .nargs(1)
        .default_value(DEFAULT_TARGET)
        .metavar("target")
        .help("Target name (default: default)")
        .store_into(targetName);

    argParser.add_argument("-task")
        .nargs(1)
        .default_value("")
        .metavar("task")
        .help("Execute a single task")
        .store_into(taskName);

    argParser.add_argument("-start_task")
        .nargs(1)
        .default_value("")
        .metavar("task")
        .help("Start execution from this task")
        .store_into(startTaskName);

    argParser.add_argument("-end_task")
        .nargs(1)
        .default_value("")
        .metavar("task")
        .help("End execution at this task (inclusive)")
        .store_into(endTaskName);

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

    // Add build subcommand
    ArgumentParser buildParser("build");
    buildParser.add_description("Execute the build section of the target's flow");
    std::string buildConfigFilePath;
    std::string buildOutDirPath;
    std::string buildTargetName = DEFAULT_TARGET;
    buildParser.add_argument("-c", "-config")
        .nargs(1)
        .default_value("")
        .metavar("stargate.toml")
        .help("Path to the config file")
        .store_into(buildConfigFilePath);
    buildParser.add_argument("-o", "-out_dir")
        .nargs(1)
        .default_value("")
        .metavar("sgc.out")
        .help("Path to the output directory")
        .store_into(buildOutDirPath);
    buildParser.add_argument("-target")
        .nargs(1)
        .default_value(DEFAULT_TARGET)
        .metavar("target")
        .help("Target name (default: default)")
        .store_into(buildTargetName);
    argParser.add_subparser(buildParser);

    // Add run subcommand
    ArgumentParser runParser("run");
    runParser.add_description("Execute the run section of the target's flow");
    std::string runConfigFilePath;
    std::string runOutDirPath;
    std::string runTargetName = DEFAULT_TARGET;
    runParser.add_argument("-c", "-config")
        .nargs(1)
        .default_value("")
        .metavar("stargate.toml")
        .help("Path to the config file")
        .store_into(runConfigFilePath);
    runParser.add_argument("-o", "-out_dir")
        .nargs(1)
        .default_value("")
        .metavar("sgc.out")
        .help("Path to the output directory")
        .store_into(runOutDirPath);
    runParser.add_argument("-target")
        .nargs(1)
        .default_value(DEFAULT_TARGET)
        .metavar("target")
        .help("Target name (default: default)")
        .store_into(runTargetName);
    argParser.add_subparser(runParser);

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
        stargate.init();

        // Handle clean subcommand
        if (argParser.is_subcommand_used("clean")) {
            if (!cleanOutDirPath.empty()) {
                stargateConfig.setStargateDir(cleanOutDirPath);
            }
            stargate.clean();
            return EXIT_SUCCESS;
        }

        // Handle build subcommand
        if (argParser.is_subcommand_used("build")) {
            if (!buildOutDirPath.empty()) {
                stargateConfig.setStargateDir(buildOutDirPath);
            }
            if (!buildConfigFilePath.empty()) {
                projectConfig.setConfigPath(buildConfigFilePath);
            }
            projectConfig.setVerbose(isVerbose);
            projectConfig.readConfig();
            stargate.build(&projectConfig, buildTargetName);
            return EXIT_SUCCESS;
        }

        // Handle run subcommand
        if (argParser.is_subcommand_used("run")) {
            if (!runOutDirPath.empty()) {
                stargateConfig.setStargateDir(runOutDirPath);
            }
            if (!runConfigFilePath.empty()) {
                projectConfig.setConfigPath(runConfigFilePath);
            }
            projectConfig.setVerbose(isVerbose);
            projectConfig.readConfig();
            stargate.runFlow(&projectConfig, runTargetName);
            return EXIT_SUCCESS;
        }

        // Handle main command options
        if (!outDirPath.empty()) {
            stargateConfig.setStargateDir(outDirPath);
        }

        if (!configFilePath.empty()) {
            projectConfig.setConfigPath(configFilePath);
        }

        projectConfig.setVerbose(isVerbose);
        projectConfig.readConfig();

        // Handle task execution options
        if (!taskName.empty()) {
            stargate.executeTask(&projectConfig, targetName, taskName);
            return EXIT_SUCCESS;
        }

        if (!startTaskName.empty() || !endTaskName.empty()) {
            if (startTaskName.empty() || endTaskName.empty()) {
                spdlog::error("-start_task and -end_task must be used together");
                return EXIT_FAILURE;
            }
            stargate.executeTaskRange(&projectConfig,
                                       targetName,
                                       startTaskName,
                                       endTaskName);
            return EXIT_SUCCESS;
        }

        // Default: run the standard flow
        stargate.run(&projectConfig);

    } catch (const FatalException& e) {
        spdlog::error("{}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

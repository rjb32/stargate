#include <stdlib.h>
#include <iostream>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include "Stargate.h"

#include "StargateConfig.h"
#include "ProjectConfig.h"

#include "DistribFlow.h"
#include "FatalException.h"

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
    argParser.add_subparser(cleanParser);

    // Add build subcommand
    ArgumentParser buildParser("build");
    buildParser.add_description("Execute the build section of the target's flow");
    argParser.add_subparser(buildParser);

    // Add run subcommand
    ArgumentParser runParser("run");
    runParser.add_description("Execute the run section of the target's flow");
    argParser.add_subparser(runParser);

    // Add infra subcommand
    ArgumentParser infraParser("infra");
    infraParser.add_description("Manage the distrib flow's infrastructure");
    infraParser.add_argument("action")
        .metavar("action")
        .help("Infra action to perform "
              "(init, ls, start, stop, destroy, gui)");
    infraParser.add_argument("subaction")
        .metavar("subaction")
        .nargs(argparse::nargs_pattern::optional)
        .default_value(std::string(""))
        .help("Sub-action for gui: start, stop. "
              "If omitted, gui opens the DCV browser session.");
    infraParser.add_argument("--dry")
        .nargs(0)
        .default_value(false)
        .implicit_value(true)
        .help("Validate config and log planned actions without provisioning");
    infraParser.add_argument("-y", "--yes")
        .nargs(0)
        .default_value(false)
        .implicit_value(true)
        .help("Assume yes to all infra confirmation prompts");
    argParser.add_subparser(infraParser);

    try {
        argParser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        spdlog::error("{}", err.what());
        return EXIT_FAILURE;
    }

    try {
        StargateConfig stargateConfig;
        ProjectConfig projectConfig;

        if (!outDirPath.empty()) {
            stargateConfig.setStargateDir(outDirPath);
        }

        Stargate stargate(stargateConfig);
        stargate.init();

        // Handle clean subcommand
        if (argParser.is_subcommand_used("clean")) {
            stargate.clean();
            return EXIT_SUCCESS;
        }

        // Check if a command that needs the project config is requested
        const bool hasBuild = argParser.is_subcommand_used("build");
        const bool hasRun = argParser.is_subcommand_used("run");
        const bool hasInfra = argParser.is_subcommand_used("infra");
        const bool hasTask = !taskName.empty();
        const bool hasTaskRange = !startTaskName.empty() || !endTaskName.empty();

        if (!hasBuild && !hasRun && !hasInfra && !hasTask && !hasTaskRange) {
            std::cout << argParser << std::endl;
            return EXIT_SUCCESS;
        }

        // Setup project config for commands that need it
        if (!configFilePath.empty()) {
            projectConfig.setConfigPath(configFilePath);
        }

        projectConfig.setVerbose(isVerbose);
        projectConfig.readConfig();

        // Handle build subcommand
        if (hasBuild) {
            stargate.runSection(&projectConfig, targetName, "build");
            return EXIT_SUCCESS;
        }

        // Handle run subcommand
        if (hasRun) {
            stargate.runSection(&projectConfig, targetName, "run");
            return EXIT_SUCCESS;
        }

        // Handle infra subcommand
        if (hasInfra) {
            const std::string& action = infraParser.get<std::string>("action");
            const std::string& subaction = infraParser.get<std::string>("subaction");
            const bool dryMode = infraParser.get<bool>("--dry");
            const bool yesMode = infraParser.get<bool>("--yes");
            setInfraYesMode(yesMode);

            if (action != "gui" && !subaction.empty()) {
                spdlog::error("Sub-action '{}' is only valid with 'gui'",
                              subaction);
                return EXIT_FAILURE;
            } else if (action == "init") {
                stargate.infraInit(&projectConfig, dryMode);
                return EXIT_SUCCESS;
            } else if (action == "ls") {
                stargate.infraLs(&projectConfig);
                return EXIT_SUCCESS;
            } else if (action == "start") {
                stargate.infraStart(&projectConfig);
                return EXIT_SUCCESS;
            } else if (action == "stop") {
                stargate.infraStop(&projectConfig);
                return EXIT_SUCCESS;
            } else if (action == "destroy") {
                stargate.infraDestroy(&projectConfig);
                return EXIT_SUCCESS;
            } else if (action == "gui") {
                GUIAction guiAction = GUIAction::OPEN;
                if (subaction == "start") {
                    guiAction = GUIAction::START;
                } else if (subaction == "stop") {
                    guiAction = GUIAction::STOP;
                } else if (!subaction.empty()) {
                    spdlog::error("Unknown gui sub-action: {}", subaction);
                    return EXIT_FAILURE;
                }
                stargate.infraGui(&projectConfig, guiAction);
                return EXIT_SUCCESS;
            } else {
                spdlog::error("Unknown infra action: {}", action);
                return EXIT_FAILURE;
            }
        }

        // Handle task execution options
        if (hasTask) {
            stargate.executeTask(&projectConfig, targetName, taskName);
            return EXIT_SUCCESS;
        }

        if (startTaskName.empty() || endTaskName.empty()) {
            spdlog::error("-start_task and -end_task must be used together");
            return EXIT_FAILURE;
        }

        stargate.executeTaskRange(&projectConfig,
                                  targetName,
                                  startTaskName,
                                  endTaskName);
    } catch (const FatalException& e) {
        spdlog::error("{}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

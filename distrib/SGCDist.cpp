#include "SGCDist.h"

#include <filesystem>

#include "DistribConfig.h"
#include "DistribFlow.h"
#include "DistribFlowManager.h"

#include "Command.h"
#include "CommandExecutor.h"
#include "Panic.h"

using namespace stargate;

namespace {

constexpr const char* BASH_BINARY = "/bin/bash";
constexpr const char* COMMAND_LOG_NAME = "command.log";

}

SGCDist::SGCDist() {
    _flowManager = std::make_unique<DistribFlowManager>();
    _flowManager->init();
}

SGCDist::~SGCDist() {
}

int SGCDist::exec() {
    namespace fs = std::filesystem;

    if (_commandScriptPath.empty()) {
        panic("SGCDist requires a command script path");
    }
    if (!fs::exists(_commandScriptPath)) {
        panic("Command script not found: {}", _commandScriptPath);
    }

    if (_distribConfigPath.empty()) {
        return execLocal();
    }

    if (!fs::exists(_distribConfigPath)) {
        panic("Distrib config not found: {}", _distribConfigPath);
    }

    return execFlow();
}

int SGCDist::execLocal() {
    namespace fs = std::filesystem;

    const fs::path scriptPath(_commandScriptPath);
    const std::string logPath =
        (scriptPath.parent_path() / COMMAND_LOG_NAME).string();

    Command command;
    command.setName(BASH_BINARY);
    command.addArg(_commandScriptPath);
    command.setLogPath(logPath);

    CommandExecutor executor;
    return executor.exec(&command);
}

int SGCDist::execFlow() {
    DistribConfig config;
    config.load(_distribConfigPath);

    const std::string& flowName = config.getFlowName();
    if (flowName.empty()) {
        panic("Distrib config {} has no 'flow' key", _distribConfigPath);
    }

    DistribFlow* flow = _flowManager->getFlow(flowName);
    if (!flow) {
        panic("Unknown distrib flow '{}'", flowName);
    }

    return flow->runCommand(_commandScriptPath);
}

#include "SGCDist.h"

#include <filesystem>

#include "Command.h"
#include "CommandExecutor.h"
#include "Panic.h"

using namespace stargate;

namespace {

constexpr const char* BASH_BINARY = "/bin/bash";
constexpr const char* COMMAND_LOG_NAME = "command.log";

}

SGCDist::SGCDist() {
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
    if (!_distribConfigPath.empty() && !fs::exists(_distribConfigPath)) {
        panic("Distrib config not found: {}", _distribConfigPath);
    }

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

#include "DistribExecutor.h"

#include "DistribConfig.h"

#include "Command.h"
#include "CommandExecutor.h"
#include "Panic.h"

using namespace stargate;

namespace {

const std::string COMMAND_SCRIPT_NAME = "command.sh";
const std::string DISTRIB_SCRIPT_NAME = "distrib.sh";
const std::string DISTRIB_CONFIG_NAME = "distrib.toml";
const std::string SGCDIST_BINARY_NAME = "sgcdist";

std::string joinPath(const std::string& dir, const std::string& name) {
    if (dir.empty()) {
        return name;
    }
    if (dir.back() == '/') {
        return dir + name;
    }
    return dir + "/" + name;
}

}

DistribExecutor::DistribExecutor(const DistribConfig* config,
                                 const std::string& currentDir)
    : _distribConfig(config),
    _currentDir(currentDir)
{
    if (!_distribConfig) {
        panic("DistribExecutor requires a distrib config");
    }
    if (_currentDir.empty()) {
        panic("DistribExecutor requires a current directory");
    }
}

DistribExecutor::~DistribExecutor() {
}

int DistribExecutor::exec(const Command* command) {
    const std::string commandScriptPath = joinPath(_currentDir, COMMAND_SCRIPT_NAME);
    const std::string distribScriptPath = joinPath(_currentDir, DISTRIB_SCRIPT_NAME);
    const std::string distribConfigPath = joinPath(_currentDir, DISTRIB_CONFIG_NAME);

    CommandExecutor executor;
    executor.writeScript(command, commandScriptPath);

    _distribConfig->save(distribConfigPath);

    Command distribCommand;
    distribCommand.setName(SGCDIST_BINARY_NAME);
    distribCommand.addArg(commandScriptPath);
    distribCommand.addArg("-config");
    distribCommand.addArg(distribConfigPath);
    distribCommand.setScriptPath(distribScriptPath);

    return executor.exec(&distribCommand);
}

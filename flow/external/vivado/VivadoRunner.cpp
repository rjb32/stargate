#include "VivadoRunner.h"

#include <spdlog/spdlog.h>

#include "FlowManager.h"

#include "DistribConfig.h"
#include "DistribExecutor.h"

#include "Command.h"
#include "Panic.h"

using namespace stargate;

static const std::string VIVADO_BINARY = "vivado";
static const std::string LOG_EXTENSION = ".log";
static const std::string JOURNAL_EXTENSION = ".jou";

VivadoRunner::VivadoRunner(const FlowManager* manager,
                           const std::string& workingDir)
    : _manager(manager),
    _workingDir(workingDir)
{
}

VivadoRunner::~VivadoRunner() {
}

int VivadoRunner::runTcl(const std::string& tclPath,
                         const std::string& logBaseName) {
    if (!_manager) {
        panic("VivadoRunner requires a flow manager");
    }

    const DistribConfig* distribConfig = _manager->getDistribConfig();
    if (!distribConfig) {
        panic("VivadoRunner requires a distrib config on the flow manager");
    }

    const std::string logPath =
        _workingDir + "/" + logBaseName + LOG_EXTENSION;
    const std::string journalPath =
        _workingDir + "/" + logBaseName + JOURNAL_EXTENSION;

    Command command;
    command.setName(VIVADO_BINARY);
    command.addArg("-mode");
    command.addArg("batch");
    command.addArg("-source");
    command.addArg(tclPath);
    command.addArg("-log");
    command.addArg(logPath);
    command.addArg("-journal");
    command.addArg(journalPath);

    DistribExecutor executor;
    executor.setCurrentDir(_workingDir);
    executor.setDistribConfig(distribConfig);

    spdlog::info("Running vivado on {}", tclPath);
    return executor.exec(&command);
}

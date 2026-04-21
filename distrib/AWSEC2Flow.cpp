#include "AWSEC2Flow.h"

#include <spdlog/spdlog.h>

#include "DistribConfig.h"
#include "DistribFlowManager.h"

#include "Command.h"
#include "CommandExecutor.h"
#include "Panic.h"

using namespace stargate;

namespace {

constexpr const char* BASH_BINARY = "/bin/bash";

}

AWSEC2Flow::AWSEC2Flow()
    : DistribFlow()
{
}

AWSEC2Flow* AWSEC2Flow::create(DistribFlowManager* manager) {
    AWSEC2Flow* flow = new AWSEC2Flow();
    manager->addFlow(flow);
    return flow;
}

void AWSEC2Flow::init(const DistribConfig* config) {
    if (!config) {
        panic("AWSEC2Flow::init requires a distrib config");
    }

    spdlog::info("AWSEC2 infra init: validating distrib config");
    spdlog::info("AWSEC2 infra init: choosing or creating VPC");
    spdlog::info("AWSEC2 infra init: choosing or creating subnets");
}

int AWSEC2Flow::runCommand(const std::string& commandScriptPath) {
    spdlog::info("AWSEC2 flow: running command script {}", commandScriptPath);

    Command command;
    command.setName(BASH_BINARY);
    command.addArg(commandScriptPath);

    CommandExecutor executor;
    return executor.exec(&command);
}

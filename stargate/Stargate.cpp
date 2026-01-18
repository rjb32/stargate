#include "Stargate.h"

#include <spdlog/spdlog.h>

#include "StargateConfig.h"

#include "FileUtils.h"

using namespace stargate;

Stargate::Stargate(const StargateConfig& config)
    : _config(config)
{
}

Stargate::~Stargate() {
}

void Stargate::run() {
    createOutputDir();
}

void Stargate::createOutputDir() {
    const auto& stargateDir = _config.getStargateDir();

    // Empty output directory if it exists, create otherwise
    if (FileUtils::exists(stargateDir)) {
        FileUtils::removeDirectory(stargateDir);
    }

    FileUtils::createDirectory(stargateDir);

    spdlog::info("Using stargate output directory {}", stargateDir);
}

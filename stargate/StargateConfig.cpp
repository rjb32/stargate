#include "StargateConfig.h"

#include <filesystem>

#define DEFAULT_STARGATE_DIR "sgc.out"

using namespace stargate;

StargateConfig::StargateConfig()
{
    _stargateDir = std::filesystem::current_path()/DEFAULT_STARGATE_DIR;
}

StargateConfig::~StargateConfig() {
}

void StargateConfig::setStargateDir(const std::string& stargateDir) {
    _stargateDir = std::filesystem::absolute(stargateDir);
}

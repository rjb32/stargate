#include "CoreConfig.h"

#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>

using namespace stargate;

CoreConfig::CoreConfig()
{
}

CoreConfig::~CoreConfig() {
}

bool CoreConfig::readConfig(const std::string& configPath) {
    try {
        YAML::Node config = YAML::LoadFile(configPath);
        (void)config;
    } catch (const YAML::Exception& e) {
        spdlog::error("Error loading config file: {}", e.what());
        return false;
    }
    
    return true;
}
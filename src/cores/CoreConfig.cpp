#include "CoreConfig.h"

#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>

using namespace stargate;

CoreConfig::CoreConfig()
{
}

CoreConfig::~CoreConfig() {
}

bool CoreConfig::readConfig() {
    try {
        YAML::Node config = YAML::LoadFile(_configPath);
        (void)config;
    } catch (const YAML::Exception& e) {
        spdlog::error("Error loading config file: {}", e.what());
        return false;
    }
    
    return true;
}
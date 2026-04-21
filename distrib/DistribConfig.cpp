#include "DistribConfig.h"

#include <fstream>

#include <toml++/toml.hpp>

#include "FileUtils.h"
#include "Panic.h"

using namespace stargate;

namespace {

constexpr const char* FLOW_KEY = "flow";

}

DistribConfig::DistribConfig() {
}

DistribConfig::~DistribConfig() {
}

void DistribConfig::load(const std::string& path) {
    if (!FileUtils::exists(path)) {
        panic("Distrib config file does not exist: {}", path);
    }

    toml::table table;
    try {
        table = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        panic("Error loading distrib config file: {}", e.what());
    }

    loadFromTable(table);
}

void DistribConfig::loadFromTable(const toml::table& table) {
    for (const auto& [key, value] : table) {
        if (key == FLOW_KEY) {
            if (const auto& flowName = value.value<std::string>()) {
                _flowName = *flowName;
            }
        } else {
            panic("Unknown key '{}' in distrib section", key.str());
        }
    }
}

void DistribConfig::save(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open()) {
        panic("Failed to open distrib config file for writing: {}", path);
    }

    out << FLOW_KEY << " = \"" << _flowName << "\"\n";
}

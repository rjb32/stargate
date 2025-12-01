#include "ProjectConfig.h"

#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>

#include "FileUtils.h"
#include "Panic.h"

#define CONFIG_DEFAULT_PATH "stargate.yaml"

using namespace stargate;

ProjectConfig::ProjectConfig()
{
}

ProjectConfig::~ProjectConfig() {
}

void ProjectConfig::readConfig() {
    if (_configPath.empty()) {
        _configPath = CONFIG_DEFAULT_PATH;
        _configPath = FileUtils::absolute(_configPath);
    }

    if (!FileUtils::exists(_configPath)) {
        panic("Project config file {} does not exist", _configPath);
    }

    try {
        spdlog::info("Reading project config file: {}", _configPath);
        YAML::Node config = YAML::LoadFile(_configPath);
        parseConfig(config);
    } catch (const YAML::Exception& e) {
        spdlog::error("Error loading config file: {}", e.what());
    }

    if (_verbose) {
        dumpConfig();
    }
}

void ProjectConfig::dumpConfig() {
    for (const auto& filesetEntry : _filesets) {
        const FileSet& fileset = filesetEntry.second;

        spdlog::info("==== File set: {}", fileset.getName());
        for (const auto& file : fileset.patterns()) {
            spdlog::info("  Pattern: {}", file);
        }
    }

    fmt::print("\n");
}

void ProjectConfig::parseConfig(const YAML::Node& config) {
    for (const auto& pair : config) {
        const std::string& key = pair.first.as<std::string>();
        const YAML::Node& node = pair.second;
        if (key == "filesets") {
            parseFilesets(node);
        } else {
            panic("Unknown section in core config: {}", key);
        }
    }
}

void ProjectConfig::parseFilesets(const YAML::Node& filesets) {
    for (const auto& pair : filesets) {
        const std::string& filesetName = pair.first.as<std::string>();
        const YAML::Node& files = pair.second;
        if (files.IsNull()) {
            continue;
        }

        FileSet& fileset = _filesets[filesetName];
        if (fileset.getName().empty()) {
            fileset.setName(filesetName);
        }

        for (const auto& file : files) {
            fileset.addFilePattern(file.as<std::string>());
        }
    }
}

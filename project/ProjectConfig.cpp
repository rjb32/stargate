#include "ProjectConfig.h"

#include <filesystem>
#include <fstream>

#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>

#include "ProjectTarget.h"
#include "FileSet.h"

#include "FileUtils.h"
#include "Panic.h"

#define CONFIG_DEFAULT_PATH "stargate.yaml"

using namespace stargate;

ProjectConfig::ProjectConfig()
{
}

ProjectConfig::~ProjectConfig() {
    for (auto& [name, target] : _targets) {
        delete target;
    }

    for (auto& [name, fileset] : _filesets) {
        delete fileset;
    }
}

void ProjectConfig::addTarget(ProjectTarget* target) {
    _targets.emplace(target->getName(), target);
}

FileSet* ProjectConfig::getFileSet(const std::string& name) const {
    const auto it = _filesets.find(name);
    if (it == _filesets.end()) {
        return nullptr;
    }

    return it->second;
}

void ProjectConfig::readConfig() {
    if (_configPath.empty()) {
        _configPath = CONFIG_DEFAULT_PATH;
        _configPath = FileUtils::absolute(_configPath);
    }

    if (!FileUtils::exists(_configPath)) {
        panic("Project config file {} does not exist", _configPath);
    }

    if (!FileUtils::isFile(_configPath)) {
        panic("Project config path {} is not a file", _configPath);
    }

    try {
        spdlog::info("Reading project config file: {}", _configPath);
        YAML::Node config = YAML::LoadFile(_configPath);
        parseConfig(config);
    } catch (const YAML::Exception& e) {
        panic("Error loading config file: {}", e.what());
    }

    if (_verbose) {
        dumpConfig();
    }
}

void ProjectConfig::dumpConfig() {
    for (auto& [name, fileset] : _filesets) {
        spdlog::info("==== File set: {}", fileset->getName());
        for (const auto& file : fileset->patterns()) {
            spdlog::info("  Pattern: {}", file);
        }
    }

    for (auto& [name, target] : _targets) {
        spdlog::info("==== Target: {}", target->getName());
    }

    fmt::print("\n");
}

void ProjectConfig::parseConfig(const YAML::Node& config) {
    for (const auto& pair : config) {
        const std::string& key = pair.first.as<std::string>();
        const YAML::Node& node = pair.second;
        if (key == "filesets") {
            parseFilesets(node);
        } else if (key == "targets") {
            parseTargets(node);
        } else {
            panic("Unknown section in core config: {}", key);
        }
    }
}

void ProjectConfig::parseFilesets(const YAML::Node& filesets) {
    for (auto& pair : filesets) {
        const std::string& filesetName = pair.first.as<std::string>();

        const YAML::Node& files = pair.second;
        if (files.IsNull()) {
            continue;
        }

        FileSet* fileset = getFileSet(filesetName);
        if (!fileset) {
            fileset = new FileSet(filesetName);
            _filesets[filesetName] = fileset;
        }

        for (const auto& file : files) {
            fileset->addFilePattern(file.as<std::string>());
        }
    }
}

void ProjectConfig::parseTargets(const YAML::Node& targets) {
    for (auto& pair : targets) {
        const std::string& targetName = pair.first.as<std::string>();
        ProjectTarget* targetObj = ProjectTarget::create(this, targetName);

        const YAML::Node& target = pair.second;
        if (target.IsNull()) {
            continue;
        }

        for (auto& section : target) {
            const std::string& sectionName = section.first.as<std::string>();
            if (sectionName == "filesets") {
                const auto& filesets = section.second;
                if (filesets.IsNull()) {
                    continue;
                }

                for (auto& fileset : filesets) {
                    const std::string& filesetName = fileset.as<std::string>();
                    FileSet* filesetObj = getFileSet(filesetName);
                    targetObj->addFileSet(filesetObj);
                }
            } else {
                panic("Invalid section '{}' in target {}", sectionName, targetName);
            }
        }
    }
}

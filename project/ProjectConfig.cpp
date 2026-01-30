#include "ProjectConfig.h"

#include <filesystem>
#include <fstream>

#include <toml++/toml.hpp>
#include <spdlog/spdlog.h>

#include "ProjectTarget.h"
#include "FileSet.h"

#include "FileUtils.h"
#include "Panic.h"

#define CONFIG_DEFAULT_PATH "stargate.toml"

using namespace stargate;

ProjectConfig::ProjectConfig()
{
}

ProjectConfig::~ProjectConfig() {
    for (const auto& [name, target] : _targets) {
        delete target;
    }

    for (const auto& [name, fileset] : _filesets) {
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

ProjectTarget* ProjectConfig::getTarget(const std::string& name) const {
    const auto it = _targets.find(name);
    if (it == _targets.end()) {
        return nullptr;
    }

    return it->second;
}

void ProjectConfig::readConfig() {
    if (_configPath.empty()) {
        _configPath = CONFIG_DEFAULT_PATH;
        FileUtils::absolute(_configPath, _configPath);
    }

    if (!FileUtils::exists(_configPath)) {
        panic("Project config file {} does not exist", _configPath);
    }

    if (!FileUtils::isFile(_configPath)) {
        panic("Project config path {} is not a file", _configPath);
    }

    try {
        spdlog::info("Reading project config file: {}", _configPath);
        toml::table config = toml::parse_file(_configPath);
        parseConfig(config);
    } catch (const toml::parse_error& e) {
        panic("Error loading config file: {}", e.what());
    }

    if (_verbose) {
        dumpConfig();
    }
}

void ProjectConfig::dumpConfig() const {
    for (const auto& [name, fileset] : _filesets) {
        spdlog::info("==== File set: {}", fileset->getName());
        for (const auto& file : fileset->patterns()) {
            spdlog::info("  Pattern: {}", file);
        }
    }

    for (const auto& [name, target] : _targets) {
        spdlog::info("==== Target: {}", target->getName());
    }

    fmt::print("\n");
}

void ProjectConfig::parseConfig(const toml::table& config) {
    for (const auto& [key, value] : config) {
        if (key == "filesets") {
            if (const toml::table* filesets = value.as_table()) {
                parseFilesets(*filesets);
            }
        } else if (key == "targets") {
            if (const toml::table* targets = value.as_table()) {
                parseTargets(*targets);
            }
        } else {
            panic("Unknown section in core config: {}", key.str());
        }
    }
}

void ProjectConfig::parseFilesets(const toml::table& filesets) {
    for (const auto& [key, value] : filesets) {
        const toml::array* files = value.as_array();
        if (!files) {
            continue;
        }

        std::string filesetName(key.str());
        FileSet* fileset = getFileSet(filesetName);
        if (!fileset) {
            fileset = new FileSet(filesetName);
            _filesets[filesetName] = fileset;
        }

        for (const auto& file : *files) {
            if (const auto& pattern = file.value<std::string>()) {
                fileset->addFilePattern(*pattern);
            }
        }
    }
}

void ProjectConfig::parseTargetProperty(ProjectTarget* target,
                                        std::string_view key,
                                        const toml::node& value) {
    if (key == "filesets") {
        const toml::array* filesetList = value.as_array();
        if (!filesetList) {
            return;
        }

        for (const auto& fileset : *filesetList) {
            if (const auto& filesetName = fileset.value<std::string>()) {
                FileSet* filesetObj = getFileSet(*filesetName);
                target->addFileSet(filesetObj);
            }
        }
    } else if (key == "flow") {
        if (const auto& flowName = value.value<std::string>()) {
            target->setFlowName(*flowName);
        }
    } else {
        panic("Invalid section '{}' in target {}", key, target->getName());
    }
}

void ProjectConfig::parseTargets(const toml::table& targets) {
    ProjectTarget* defaultTarget = nullptr;

    for (const auto& [key, value] : targets) {
        if (value.is_table()) {
            std::string targetName(key.str());
            ProjectTarget* targetObj = ProjectTarget::create(this, targetName);

            for (const auto& [sectionKey, sectionValue] : *value.as_table()) {
                parseTargetProperty(targetObj, sectionKey.str(), sectionValue);
            }
        } else {
            if (!defaultTarget) {
                defaultTarget = ProjectTarget::create(this, "default");
            }
            parseTargetProperty(defaultTarget, key.str(), value);
        }
    }
}

#pragma once

#include <string>
#include <map>
#include <ranges>

#include "FileSet.h"

namespace YAML {
class Node;
}

namespace stargate {

class ProjectConfig {
public:
    ProjectConfig();
    ~ProjectConfig();

    const std::string& getConfigPath() const { return _configPath; }

    void setConfigPath(const std::string& configPath) { _configPath = configPath; }

    void readConfig();

    auto filesets() const { return std::views::values(_filesets); }

private:
    bool _verbose {true};
    std::string _configPath;
    std::map<std::string, FileSet> _filesets;

    void parseConfig(const YAML::Node& config);
    void parseFilesets(const YAML::Node& filesets);
    void dumpConfig();
};

}

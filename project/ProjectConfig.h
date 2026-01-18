#pragma once

#include <string>
#include <map>
#include <ranges>

namespace YAML {
class Node;
}

namespace stargate {

class FileSet;
class ProjectTarget;

class ProjectConfig {
public:
    friend ProjectTarget;

    explicit ProjectConfig();
    ~ProjectConfig();

    bool isVerbose() const { return _verbose; }
    void setVerbose(bool verbose) { _verbose = verbose; }

    const std::string& getConfigPath() const { return _configPath; }

    auto filesets() const { return std::views::values(_filesets); }
    
    FileSet* getFileSet(const std::string& name) const;

    void setConfigPath(const std::string& configPath) { _configPath = configPath; }

    void readConfig();

private:
    bool _verbose {false};
    std::string _configPath;
    std::map<std::string, FileSet*> _filesets;
    std::map<std::string, ProjectTarget*> _targets;

    void addTarget(ProjectTarget* target);
    void parseConfig(const YAML::Node& config);
    void parseFilesets(const YAML::Node& filesets);
    void parseTargets(const YAML::Node& targets);
    void dumpConfig();
};

}

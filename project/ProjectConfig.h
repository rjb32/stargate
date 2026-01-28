#pragma once

#include <string>
#include <map>
#include <ranges>

namespace toml {
inline namespace v3 {
class node;
class table;
}
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

    auto targets() const { return std::views::values(_targets); }

    void setConfigPath(const std::string& configPath) { _configPath = configPath; }

    void readConfig();

private:
    bool _verbose {false};
    std::string _configPath;
    std::map<std::string, FileSet*> _filesets;
    std::map<std::string, ProjectTarget*> _targets;

    void addTarget(ProjectTarget* target);
    void parseConfig(const toml::table& config);
    void parseFilesets(const toml::table& filesets);
    void parseTargets(const toml::table& targets);
    void parseTargetProperty(ProjectTarget* target,
                             std::string_view key,
                             const toml::node& value);
    void dumpConfig();
};

}

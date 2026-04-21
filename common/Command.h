#pragma once

#include <string>
#include <utility>
#include <vector>

namespace stargate {

class Command {
public:
    using Args = std::vector<std::string>;
    using EnvVar = std::pair<std::string, std::string>;
    using EnvVars = std::vector<EnvVar>;
    using PathEntries = std::vector<std::string>;

    Command();
    ~Command();

    void setName(const std::string& name) { _name = name; }
    void setArgs(const Args& args) { _args = args; }
    void addArg(const std::string& arg) { _args.push_back(arg); }

    void setLogPath(const std::string& path) { _logPath = path; }
    void setScriptPath(const std::string& path) { _scriptPath = path; }

    void addEnvVar(const std::string& name, const std::string& value);
    void addExecPath(const std::string& path);

    const std::string& getName() const { return _name; }
    const std::string& getLogPath() const { return _logPath; }
    const std::string& getScriptPath() const { return _scriptPath; }

    const Args& args() const { return _args; }
    const EnvVars& envVars() const { return _envVars; }
    const PathEntries& pathEntries() const { return _pathEntries; }

private:
    std::string _name;
    Args _args;

    std::string _logPath;
    std::string _scriptPath;

    EnvVars _envVars;
    PathEntries _pathEntries;
};

}

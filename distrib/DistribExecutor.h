#pragma once

#include <string>

namespace stargate {

class Command;
class DistribConfig;

class DistribExecutor {
public:
    DistribExecutor();
    ~DistribExecutor();

    void setCurrentDir(const std::string& currentDir) {
        _currentDir = currentDir;
    }

    const std::string& getCurrentDir() const { return _currentDir; }

    void setDistribConfig(const DistribConfig* config) {
        _distribConfig = config;
    }

    const DistribConfig* getDistribConfig() const { return _distribConfig; }

    int exec(const Command* command);

private:
    std::string _currentDir;
    const DistribConfig* _distribConfig {nullptr};

    void writeDistribConfig(const std::string& path) const;
};

}

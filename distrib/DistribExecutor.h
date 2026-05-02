#pragma once

#include <string>

namespace stargate {

class Command;
class DistribConfig;

class DistribExecutor {
public:
    DistribExecutor(const DistribConfig* config, const std::string& currentDir);
    ~DistribExecutor();

    const DistribConfig* getDistribConfig() const { return _distribConfig; }
    const std::string& getCurrentDir() const { return _currentDir; }

    int exec(const Command* command);

private:
    const DistribConfig* _distribConfig {nullptr};
    std::string _currentDir;

    void writeDistribConfig(const std::string& path) const;
};

}

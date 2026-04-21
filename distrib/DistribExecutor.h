#pragma once

#include <string>

namespace stargate {

class Command;

class DistribExecutor {
public:
    DistribExecutor();
    ~DistribExecutor();

    void setCurrentDir(const std::string& currentDir) {
        _currentDir = currentDir;
    }

    const std::string& getCurrentDir() const { return _currentDir; }

    int exec(const Command* command);

private:
    std::string _currentDir;
};

}

#pragma once

#include <string>

namespace stargate {

class SGCDist {
public:
    SGCDist();
    ~SGCDist();

    void setCommandScriptPath(const std::string& path) {
        _commandScriptPath = path;
    }

    void setDistribConfigPath(const std::string& path) {
        _distribConfigPath = path;
    }

    const std::string& getCommandScriptPath() const {
        return _commandScriptPath;
    }

    const std::string& getDistribConfigPath() const {
        return _distribConfigPath;
    }

    int exec();

private:
    std::string _commandScriptPath;
    std::string _distribConfigPath;
};

}

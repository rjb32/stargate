#pragma once

#include <string>

namespace stargate {

class CoreConfig {
public:
    CoreConfig();
    ~CoreConfig();

    const std::string& getConfigPath() const { return _configPath; }

    void setConfigPath(const std::string& configPath) { _configPath = configPath; }

    bool readConfig();

private:
    std::string _configPath;
};

}
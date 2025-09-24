#pragma once

#include <string>

namespace stargate {

class CoreConfig {
public:
    CoreConfig();
    ~CoreConfig();

    bool readConfig(const std::string& configPath);
};

}
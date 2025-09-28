#pragma once

#include <string>

namespace stargate {

class StargateConfig {
public:
    StargateConfig();
    ~StargateConfig();

    const std::string& getStargateDir() const { return _stargateDir; }

    void setStargateDir(const std::string& stargateDir);

private:
    std::string _stargateDir;
};

}
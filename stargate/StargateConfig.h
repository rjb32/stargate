#pragma once

#include <string>

namespace stargate {

class StargateConfig {
public:
    StargateConfig();
    ~StargateConfig();

    const std::string& getStargateDir() const { return _stargateDir; }

    void setStargateDir(const std::string& stargateDir);

    bool getVerbose() const { return _verbose; }
    void setVerbose(bool verbose) { _verbose = verbose; }

private:
    std::string _stargateDir;
    bool _verbose {false};
};

}

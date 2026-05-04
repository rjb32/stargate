#pragma once

#include <memory>
#include <string>

namespace toml {
inline namespace v3 {
class table;
}
}

namespace stargate {

class AWSEC2Config;

class DistribConfig {
public:
    DistribConfig();
    ~DistribConfig();

    void load(const std::string& path);
    void loadFromTable(const toml::table& table);
    void save(const std::string& path) const;

    const std::string& getFlowName() const { return _flowName; }
    void setFlowName(const std::string& flowName) { _flowName = flowName; }

    const AWSEC2Config* getAWSEC2Config() const { return _awsec2Config.get(); }
    AWSEC2Config* getAWSEC2Config() { return _awsec2Config.get(); }

private:
    std::string _flowName;
    std::unique_ptr<AWSEC2Config> _awsec2Config;
};

}

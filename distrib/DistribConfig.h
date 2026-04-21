#pragma once

#include <string>

namespace toml {
inline namespace v3 {
class table;
}
}

namespace stargate {

class DistribConfig {
public:
    DistribConfig();
    ~DistribConfig();

    void load(const std::string& path);
    void loadFromTable(const toml::table& table);
    void save(const std::string& path) const;

    const std::string& getFlowName() const { return _flowName; }
    void setFlowName(const std::string& flowName) { _flowName = flowName; }

private:
    std::string _flowName;
};

}

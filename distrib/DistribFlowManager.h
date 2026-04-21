#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

namespace stargate {

class DistribFlow;
class AWSEC2Flow;

class DistribFlowManager {
public:
    using Flows = std::vector<DistribFlow*>;
    using FlowNameMap = std::unordered_map<std::string_view, DistribFlow*>;

    DistribFlowManager();
    ~DistribFlowManager();

    void init();

    const Flows& flows() const { return _flows; }

    DistribFlow* getFlow(std::string_view name) const;

private:
    friend DistribFlow;
    friend AWSEC2Flow;

    void addFlow(DistribFlow* flow);

    Flows _flows;
    FlowNameMap _flowNameMap;
};

}

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace stargate {

class Flow;
class VivadoFlow;

class FlowManager {
public:
    using Flows = std::vector<Flow*>;
    using FlowNameMap = std::unordered_map<std::string_view, Flow*>;

    FlowManager();
    ~FlowManager();

    void init();

    const Flows& flows() const { return _flows; }
    Flow* getFlow(std::string_view name) const;

    void setOutputDir(const std::string& outputDir);
    const std::string& getOutputDir() const { return _outputDir; }

private:
    friend Flow;
    friend VivadoFlow;

    void addFlow(Flow* flow);

    Flows _flows;
    FlowNameMap _flowNameMap;
    std::string _outputDir;
};

}

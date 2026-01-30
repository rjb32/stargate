#include "FlowManager.h"

#include "Flow.h"
#include "VivadoFlow.h"

using namespace stargate;

FlowManager::FlowManager()
{
}

FlowManager::~FlowManager() {
    for (Flow* flow : _flows) {
        delete flow;
    }
}

void FlowManager::init() {
    VivadoFlow::create(this);
}

void FlowManager::addFlow(Flow* flow) {
    flow->_manager = this;
    _flows.push_back(flow);
    _flowNameMap[flow->getName()] = flow;
}

Flow* FlowManager::getFlow(std::string_view name) const {
    const auto it = _flowNameMap.find(name);
    if (it == _flowNameMap.end()) {
        return nullptr;
    }

    return it->second;
}

void FlowManager::setOutputDir(const std::string& outputDir) {
    _outputDir = outputDir;
}

#include "DistribFlowManager.h"

#include "DistribFlow.h"
#include "AWSEC2Flow.h"

using namespace stargate;

DistribFlowManager::DistribFlowManager() {
}

DistribFlowManager::~DistribFlowManager() {
    for (DistribFlow* flow : _flows) {
        delete flow;
    }
}

void DistribFlowManager::init() {
    AWSEC2Flow::create(this);
}

void DistribFlowManager::addFlow(DistribFlow* flow) {
    flow->_manager = this;
    _flows.push_back(flow);
    _flowNameMap[flow->getName()] = flow;
}

DistribFlow* DistribFlowManager::getFlow(std::string_view name) const {
    const auto it = _flowNameMap.find(name);
    if (it == _flowNameMap.end()) {
        return nullptr;
    }

    return it->second;
}

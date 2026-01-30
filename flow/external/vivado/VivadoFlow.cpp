#include "VivadoFlow.h"

#include "FlowManager.h"
#include "FlowSection.h"

#include "VivadoSynthTask.h"
#include "VivadoImplTask.h"
#include "VivadoBitstreamTask.h"

using namespace stargate;

VivadoFlow::VivadoFlow()
    : Flow()
{
}

VivadoFlow* VivadoFlow::create(FlowManager* manager) {
    VivadoFlow* flow = new VivadoFlow();
    manager->addFlow(flow);
    flow->initSections();
    return flow;
}

void VivadoFlow::initSections() {
    FlowSection* buildSection = FlowSection::create(this, "build");

    VivadoSynthTask::create(buildSection);
    VivadoImplTask::create(buildSection);
    VivadoBitstreamTask::create(buildSection);
}

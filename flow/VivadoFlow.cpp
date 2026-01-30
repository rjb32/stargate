#include "VivadoFlow.h"

#include <spdlog/spdlog.h>

#include "FlowManager.h"
#include "FlowSection.h"

using namespace stargate;

// VivadoFlow implementation

VivadoFlow::VivadoFlow()
    : Flow()
{
}

VivadoFlow* VivadoFlow::create(FlowManager* manager) {
    VivadoFlow* flow = new VivadoFlow();
    manager->addFlow(flow);
    flow->initializeSections();
    return flow;
}

void VivadoFlow::initializeSections() {
    FlowSection* buildSection = FlowSection::create(this, "build");

    VivadoSynthTask::create(buildSection);
    VivadoImplTask::create(buildSection);
    VivadoBitstreamTask::create(buildSection);
}

// VivadoSynthTask implementation

VivadoSynthTask::VivadoSynthTask(FlowSection* parent)
    : FlowTask(parent)
{
}

VivadoSynthTask* VivadoSynthTask::create(FlowSection* parent) {
    VivadoSynthTask* task = new VivadoSynthTask(parent);
    parent->addTask(task);
    return task;
}

void VivadoSynthTask::execute() {
    spdlog::info("Executing Vivado synthesis task");
    writeStatus(TaskStatus::Success, 0, "");
}

// VivadoImplTask implementation

VivadoImplTask::VivadoImplTask(FlowSection* parent)
    : FlowTask(parent)
{
}

VivadoImplTask* VivadoImplTask::create(FlowSection* parent) {
    VivadoImplTask* task = new VivadoImplTask(parent);
    parent->addTask(task);
    return task;
}

void VivadoImplTask::execute() {
    spdlog::info("Executing Vivado implementation task");
    writeStatus(TaskStatus::Success, 0, "");
}

// VivadoBitstreamTask implementation

VivadoBitstreamTask::VivadoBitstreamTask(FlowSection* parent)
    : FlowTask(parent)
{
}

VivadoBitstreamTask* VivadoBitstreamTask::create(FlowSection* parent) {
    VivadoBitstreamTask* task = new VivadoBitstreamTask(parent);
    parent->addTask(task);
    return task;
}

void VivadoBitstreamTask::execute() {
    spdlog::info("Executing Vivado bitstream generation task");
    writeStatus(TaskStatus::Success, 0, "");
}

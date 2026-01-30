#include "VivadoImplTask.h"

#include <spdlog/spdlog.h>

#include "FlowSection.h"

using namespace stargate;

VivadoImplTask::VivadoImplTask(FlowSection* parent)
    : FlowTask(parent)
{
}

VivadoImplTask* VivadoImplTask::create(FlowSection* parent) {
    VivadoImplTask* task = new VivadoImplTask(parent);
    task->registerTask();
    return task;
}

void VivadoImplTask::execute() {
    spdlog::info("Executing Vivado implementation task");
    writeStatus(TaskStatus::Status::Success, 0, "");
}

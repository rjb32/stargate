#include "VivadoSynthTask.h"

#include <spdlog/spdlog.h>

#include "FlowSection.h"

using namespace stargate;

VivadoSynthTask::VivadoSynthTask(FlowSection* parent)
    : FlowTask(parent)
{
}

VivadoSynthTask* VivadoSynthTask::create(FlowSection* parent) {
    VivadoSynthTask* task = new VivadoSynthTask(parent);
    task->registerTask();
    return task;
}

void VivadoSynthTask::execute() {
    spdlog::info("Executing Vivado synthesis task");
    writeStatus(TaskStatus::Status::Success, 0, "");
}

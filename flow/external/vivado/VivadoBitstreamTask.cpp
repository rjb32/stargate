#include "VivadoBitstreamTask.h"

#include <spdlog/spdlog.h>

#include "FlowSection.h"

using namespace stargate;

VivadoBitstreamTask::VivadoBitstreamTask(FlowSection* parent)
    : FlowTask(parent)
{
}

VivadoBitstreamTask* VivadoBitstreamTask::create(FlowSection* parent) {
    VivadoBitstreamTask* task = new VivadoBitstreamTask(parent);
    task->registerTask();
    return task;
}

void VivadoBitstreamTask::execute() {
    spdlog::info("Executing Vivado bitstream generation task");
    writeStatus(TaskStatus::Status::Success, 0, "");
}

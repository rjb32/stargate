#include "VivadoBitstreamTask.h"

#include <spdlog/spdlog.h>

#include "FlowSection.h"
#include "Flow.h"
#include "FlowManager.h"

#include "VivadoTCLGenerator.h"
#include "VivadoRunner.h"

#include "FileUtils.h"
#include "Panic.h"

using namespace stargate;

static const std::string BITSTREAM_TCL_NAME = "bitstream.tcl";
static const std::string BITSTREAM_LOG_BASE = "bitstream";

VivadoBitstreamTask::VivadoBitstreamTask(FlowSection* parent)
    : FlowTask(parent)
{
}

VivadoBitstreamTask* VivadoBitstreamTask::create(FlowSection* parent) {
    VivadoBitstreamTask* task = new VivadoBitstreamTask(parent);
    task->registerTask();
    return task;
}

void VivadoBitstreamTask::execute(const ProjectTarget* target) {
    spdlog::info("Vivado bitstream task");

    if (!target) {
        panic("Vivado bitstream task requires a target");
    }

    const FlowManager* manager = getParent()->getParent()->getManager();

    std::string outputDir;
    getOutputDir(outputDir);

    if (!FileUtils::exists(outputDir)) {
        FileUtils::createDirectory(outputDir);
    }

    VivadoTCLGenerator generator(manager, target);
    generator.writeBitstreamTcl(outputDir);

    const std::string tclPath = outputDir + "/" + BITSTREAM_TCL_NAME;

    VivadoRunner runner(manager, outputDir);
    const int exitCode = runner.runTcl(tclPath, BITSTREAM_LOG_BASE);

    const TaskStatus::Status status = (exitCode == 0)
        ? TaskStatus::Status::Success
        : TaskStatus::Status::Failed;
    writeStatus(status, exitCode, "");
}

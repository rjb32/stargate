#include "VivadoImplTask.h"

#include <spdlog/spdlog.h>

#include "FlowSection.h"
#include "Flow.h"
#include "FlowManager.h"

#include "VivadoTCLGenerator.h"
#include "VivadoRunner.h"

#include "FileUtils.h"
#include "Panic.h"

using namespace stargate;

static const std::string IMPL_TCL_NAME = "impl.tcl";
static const std::string IMPL_LOG_BASE = "impl";

VivadoImplTask::VivadoImplTask(FlowSection* parent)
    : FlowTask(parent)
{
}

VivadoImplTask* VivadoImplTask::create(FlowSection* parent) {
    VivadoImplTask* task = new VivadoImplTask(parent);
    task->registerTask();
    return task;
}

void VivadoImplTask::execute(const ProjectTarget* target) {
    spdlog::info("Vivado implementation task");

    if (!target) {
        panic("Vivado impl task requires a target");
    }

    const FlowManager* manager = getParent()->getParent()->getManager();

    std::string outputDir;
    getOutputDir(outputDir);

    if (!FileUtils::exists(outputDir)) {
        FileUtils::createDirectory(outputDir);
    }

    VivadoTCLGenerator generator(manager, target);
    generator.writeImplTcl(outputDir);

    const std::string tclPath = outputDir + "/" + IMPL_TCL_NAME;

    VivadoRunner runner(manager, outputDir);
    const int exitCode = runner.runTcl(tclPath, IMPL_LOG_BASE);

    const TaskStatus::Status status = (exitCode == 0)
        ? TaskStatus::Status::Success
        : TaskStatus::Status::Failed;
    writeStatus(status, exitCode, "");
}

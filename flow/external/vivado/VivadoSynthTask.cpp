#include "VivadoSynthTask.h"

#include <spdlog/spdlog.h>

#include "FlowSection.h"
#include "Flow.h"
#include "FlowManager.h"

#include "VivadoTCLGenerator.h"
#include "VivadoRunner.h"

#include "FileUtils.h"
#include "Panic.h"

using namespace stargate;

static const std::string SYNTH_TCL_NAME = "synth.tcl";
static const std::string SYNTH_LOG_BASE = "synth";

VivadoSynthTask::VivadoSynthTask(FlowSection* parent)
    : FlowTask(parent)
{
}

VivadoSynthTask* VivadoSynthTask::create(FlowSection* parent) {
    VivadoSynthTask* task = new VivadoSynthTask(parent);
    task->registerTask();
    return task;
}

void VivadoSynthTask::execute(const ProjectTarget* target) {
    spdlog::info("Vivado synthesis task");

    const FlowManager* manager = getParent()->getParent()->getManager();

    std::string outputDir;
    getOutputDir(outputDir);

    if (!FileUtils::exists(outputDir)) {
        FileUtils::createDirectory(outputDir);
    }

    VivadoTCLGenerator generator(manager, target);
    generator.writeSynthTcl(outputDir);

    const std::string tclPath = outputDir + "/" + SYNTH_TCL_NAME;

    VivadoRunner runner(manager, outputDir);
    const int exitCode = runner.runTcl(tclPath, SYNTH_LOG_BASE);

    const TaskStatus::Status status = (exitCode == 0)
        ? TaskStatus::Status::Success
        : TaskStatus::Status::Failed;
    writeStatus(status, exitCode, "");
}

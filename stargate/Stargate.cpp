#include "Stargate.h"

#include <filesystem>
#include <fstream>

#include <spdlog/spdlog.h>

#include "ProjectConfig.h"
#include "StargateConfig.h"
#include "ProjectTarget.h"

#include "FlowManager.h"
#include "Flow.h"
#include "FlowSection.h"
#include "FlowTask.h"

#include "FileUtils.h"
#include "FileSetCollector.h"
#include "Panic.h"

using namespace stargate;

Stargate::Stargate(const StargateConfig& config)
    : _config(config)
{
    _flowManager = std::make_unique<FlowManager>();
}

Stargate::~Stargate() {
}

void Stargate::init() {
    _flowManager->init();
}

void Stargate::run(const ProjectConfig* projConfig) {
    createOutputDir();
    writeTargets(projConfig);
}

void Stargate::build(const ProjectConfig* projectConfig, const std::string& targetName) {
    createOutputDir();
    writeTargets(projectConfig);

    const ProjectTarget* target = projectConfig->getTarget(targetName);
    if (!target) {
        panic("Target '{}' not found", targetName);
    }

    Flow* flow = getTargetFlow(target);
    FlowSection* buildSection = flow->getBuildSection();
    if (!buildSection) {
        panic("Flow '{}' does not have a build section", flow->getName());
    }

    executeSection(buildSection);
}

void Stargate::runFlow(const ProjectConfig* projectConfig, const std::string& targetName) {
    createOutputDir();
    writeTargets(projectConfig);

    const ProjectTarget* target = projectConfig->getTarget(targetName);
    if (!target) {
        panic("Target '{}' not found", targetName);
    }

    Flow* flow = getTargetFlow(target);
    FlowSection* runSection = flow->getRunSection();
    if (!runSection) {
        panic("Flow '{}' does not have a run section", flow->getName());
    }

    executeSection(runSection);
}

void Stargate::executeTask(const ProjectConfig* projectConfig,
                           const std::string& targetName,
                           const std::string& taskName) {
    createOutputDir();
    writeTargets(projectConfig);

    const ProjectTarget* target = projectConfig->getTarget(targetName);
    if (!target) {
        panic("Target '{}' not found", targetName);
    }

    Flow* flow = getTargetFlow(target);

    FlowTask* task = nullptr;
    FlowSection* taskSection = nullptr;
    size_t taskIdx = 0;

    for (FlowSection* section : flow->sections()) {
        size_t idx = 0;
        for (FlowTask* t : section->tasks()) {
            if (t->getName() == taskName) {
                task = t;
                taskSection = section;
                taskIdx = idx;
                break;
            }
            idx++;
        }
        if (task) {
            break;
        }
    }

    if (!task) {
        panic("Task '{}' not found in flow '{}'", taskName, flow->getName());
    }

    if (!checkTaskDependencies(taskSection, taskIdx)) {
        panic("Task '{}' has unmet dependencies", taskName);
    }

    task->execute();
}

void Stargate::executeTaskRange(const ProjectConfig* projectConfig,
                                const std::string& targetName,
                                const std::string& startTaskName,
                                const std::string& endTaskName) {
    createOutputDir();
    writeTargets(projectConfig);

    const ProjectTarget* target = projectConfig->getTarget(targetName);
    if (!target) {
        panic("Target '{}' not found", targetName);
    }

    Flow* flow = getTargetFlow(target);

    size_t startIdx = SIZE_MAX;
    size_t endIdx = SIZE_MAX;
    FlowSection* taskSection = nullptr;

    for (FlowSection* section : flow->sections()) {
        size_t idx = 0;
        for (FlowTask* t : section->tasks()) {
            if (t->getName() == startTaskName) {
                startIdx = idx;
                taskSection = section;
            }
            if (t->getName() == endTaskName) {
                endIdx = idx;
                if (!taskSection) {
                    taskSection = section;
                }
            }
            idx++;
        }
        if (startIdx != SIZE_MAX && endIdx != SIZE_MAX) {
            break;
        }
    }

    if (startIdx == SIZE_MAX) {
        panic("Start task '{}' not found in flow '{}'", startTaskName, flow->getName());
    }

    if (endIdx == SIZE_MAX) {
        panic("End task '{}' not found in flow '{}'", endTaskName, flow->getName());
    }

    if (startIdx > endIdx) {
        panic("Start task '{}' comes after end task '{}'", startTaskName, endTaskName);
    }

    executeSection(taskSection, startIdx, endIdx);
}

void Stargate::createOutputDir() {
    const auto& stargateDir = _config.getStargateDir();

    // Empty output directory if it exists, create otherwise
    if (FileUtils::exists(stargateDir)) {
        FileUtils::removeDirectory(stargateDir);
    }

    FileUtils::createDirectory(stargateDir);

    _flowManager->setOutputDir(stargateDir);

    spdlog::info("Using stargate output directory {}", stargateDir);
}

void Stargate::writeTargets(const ProjectConfig* projConfig) {
    const auto basePath =
        std::filesystem::path(projConfig->getConfigPath()).parent_path().string();

    // Create project subdirectory
    const std::string projDirPath = _config.getStargateDir()+"/project";

    std::string targetPath;
    std::string fileListPath;
    for (const ProjectTarget* target : projConfig->targets()) {
        // Create target directory
        targetPath = projDirPath;
        targetPath += "/";
        targetPath += target->getName();

        FileUtils::createDirectory(targetPath);

        // File list path
        fileListPath = targetPath+"/files.list";

        writeTargetFileList(target, basePath, fileListPath);
    }
}

void Stargate::writeTargetFileList(const ProjectTarget* target,
                                   const std::string& basePath,
                                   const std::string& fileListPath) {
    FileSetCollector collector;
    collector.setBasePath(basePath);

    for (const FileSet* fileset : target->filesets()) {
        collector.addFileSet(fileset);
    }

    std::vector<std::string> paths;
    collector.collect(paths);

    std::ofstream out(fileListPath);
    if (!out) {
        panic("Failed to open file list for writing: {}", fileListPath);
    }

    for (const std::string& path : paths) {
        out << path << "\n";
    }
}

void Stargate::clean() {
    const auto& outDirPath = _config.getStargateDir();
    if (!FileUtils::exists(outDirPath)) {
        spdlog::info("Nothing to clean: {} does not exist", outDirPath);
        return;
    }

    FileUtils::removeDirectory(outDirPath);
    spdlog::info("Cleaned: {}", outDirPath);
}

Flow* Stargate::getTargetFlow(const ProjectTarget* target) {
    const std::string& flowName = target->getFlowName();
    if (flowName.empty()) {
        panic("Target '{}' does not have a flow specified", target->getName());
    }

    Flow* flow = _flowManager->getFlow(flowName);
    if (!flow) {
        panic("Flow '{}' not found", flowName);
    }

    return flow;
}

void Stargate::executeSection(FlowSection* section) {
    const auto& tasks = section->tasks();
    executeSection(section, 0, tasks.size() - 1);
}

void Stargate::executeSection(FlowSection* section, size_t startIdx, size_t endIdx) {
    const auto& tasks = section->tasks();

    if (!checkTaskDependencies(section, startIdx)) {
        const FlowTask* task = tasks[startIdx];
        panic("Task '{}' has unmet dependencies", task->getName());
    }

    for (size_t i = startIdx; i <= endIdx; i++) {
        FlowTask* task = tasks[i];
        spdlog::info("Executing task: {}", task->getName());
        task->execute();
    }
}

bool Stargate::checkTaskDependencies(FlowSection* section, size_t taskIdx) {
    const auto& tasks = section->tasks();

    for (size_t i = 0; i < taskIdx; i++) {
        const FlowTask* depTask = tasks[i];
        const TaskStatus status = depTask->getStatus();
        if (status != TaskStatus::Success) {
            spdlog::error(
                "Dependency task '{}' has not completed successfully (status: {})",
                depTask->getName(), taskStatusToString(status));
            return false;
        }
    }

    return true;
}

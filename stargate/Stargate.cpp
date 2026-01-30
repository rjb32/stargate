#include "Stargate.h"

#include <filesystem>
#include <fstream>
#include <optional>

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

void Stargate::runFlow(const ProjectConfig* projectConfig,
                       const std::string& targetName) {
    createOutputDir();
    writeTargets(projectConfig);

    const ProjectTarget* target = projectConfig->getTarget(targetName);
    if (!target) {
        panic("Target '{}' not found", targetName);
    }

    Flow* flow = getTargetFlow(target);

    FlowSection* buildSection = flow->getBuildSection();
    if (buildSection) {
        executeSection(buildSection);
    }

    FlowSection* runSection = flow->getRunSection();
    if (runSection) {
        executeSection(runSection);
    }
}

void Stargate::runSection(const ProjectConfig* projectConfig,
                          const std::string& targetName,
                          const std::string& sectionName) {
    createOutputDir();
    writeTargets(projectConfig);

    const ProjectTarget* target = projectConfig->getTarget(targetName);
    if (!target) {
        panic("Target '{}' not found", targetName);
    }

    Flow* flow = getTargetFlow(target);
    FlowSection* section = flow->getSection(sectionName);
    if (!section) {
        panic("Flow '{}' does not have a '{}' section", flow->getName(), sectionName);
    }

    executeSection(section);
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

    for (FlowSection* section : flow->sections()) {
        task = section->getTask(taskName);
        if (task) {
            taskSection = section;
            break;
        }
    }

    if (!task) {
        panic("Task '{}' not found in flow '{}'", taskName, flow->getName());
    }

    if (!checkSectionDependencies(flow, taskSection)) {
        panic("Task '{}' has unmet section dependencies", taskName);
    }

    if (!checkTaskDependencies(taskSection, task->getIndex())) {
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

    std::optional<size_t> startID;
    std::optional<size_t> endID;
    FlowSection* taskSection = nullptr;

    for (FlowSection* section : flow->sections()) {
        const auto& tasks = section->tasks();
        for (size_t i = 0; i < tasks.size(); i++) {
            if (tasks[i]->getName() == startTaskName) {
                startID = i;
                taskSection = section;
            }
            if (tasks[i]->getName() == endTaskName) {
                endID = i;
                if (!taskSection) {
                    taskSection = section;
                }
            }
        }
        if (startID && endID) {
            break;
        }
    }

    if (!startID) {
        panic("Start task '{}' not found in flow '{}'", startTaskName, flow->getName());
    }

    if (!endID) {
        panic("End task '{}' not found in flow '{}'", endTaskName, flow->getName());
    }

    if (*startID > *endID) {
        panic("Start task '{}' comes after end task '{}'", startTaskName, endTaskName);
    }

    executeSection(taskSection, *startID, *endID);
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
    Flow* flow = section->getParent();

    if (!checkSectionDependencies(flow, section)) {
        panic("Section '{}' has unmet dependencies", section->getName());
    }

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

bool Stargate::checkSectionDependencies(Flow* flow, FlowSection* section) {
    const auto& sections = flow->sections();

    for (FlowSection* depSection : sections) {
        if (depSection == section) {
            break;
        }

        for (const FlowTask* task : depSection->tasks()) {
            const TaskStatus::Status status = task->getStatus();
            if (status != TaskStatus::Status::Success) {
                spdlog::error(
                    "Dependency section '{}' task '{}' has not completed successfully "
                    "(status: {})",
                    depSection->getName(), task->getName(), TaskStatus::toString(status));
                return false;
            }
        }
    }

    return true;
}

bool Stargate::checkTaskDependencies(FlowSection* section, size_t taskIdx) {
    const auto& tasks = section->tasks();

    for (size_t i = 0; i < taskIdx; i++) {
        const FlowTask* depTask = tasks[i];
        const TaskStatus::Status status = depTask->getStatus();
        if (status != TaskStatus::Status::Success) {
            spdlog::error(
                "Dependency task '{}' has not completed successfully (status: {})",
                depTask->getName(), TaskStatus::toString(status));
            return false;
        }
    }

    return true;
}

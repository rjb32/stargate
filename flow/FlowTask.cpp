#include "FlowTask.h"

#include "FlowSection.h"
#include "Flow.h"
#include "FlowManager.h"

#include "FileUtils.h"

using namespace stargate;

FlowTask::FlowTask(FlowSection* parent)
    : _parent(parent)
{
}

FlowTask::~FlowTask() {
}

TaskStatus FlowTask::getStatus() const {
    std::string statusPath;
    getStatusFilePath(statusPath);
    return readTaskStatus(statusPath);
}

void FlowTask::getOutputDir(std::string& result) const {
    const Flow* flow = _parent->getParent();
    const FlowManager* manager = flow->getManager();

    result = manager->getOutputDir();
    result += "/";
    result += flow->getName();
    result += "/";
    result += getName();
}

void FlowTask::getStatusFilePath(std::string& result) const {
    getOutputDir(result);
    result += "/status.json";
}

void FlowTask::writeStatus(TaskStatus status,
                           int exitCode,
                           const std::string& errorMessage) {
    std::string outputDir;
    getOutputDir(outputDir);

    if (!FileUtils::exists(outputDir)) {
        FileUtils::createDirectory(outputDir);
    }

    std::string statusPath;
    getStatusFilePath(statusPath);
    writeTaskStatus(statusPath, status, exitCode, errorMessage);
}

#pragma once

#include <memory>
#include <string>

namespace stargate {

class ProjectConfig;
class StargateConfig;
class ProjectTarget;
class FlowManager;
class Flow;
class FlowSection;
class FlowTask;

class Stargate {
public:
    Stargate(const StargateConfig& config);
    ~Stargate();

    void init();
    void clean();

    void runFlow(const ProjectConfig* projectConfig, const std::string& targetName);
    void runSection(const ProjectConfig* projectConfig,
                    const std::string& targetName,
                    const std::string& sectionName);

    void executeTask(const ProjectConfig* projectConfig,
                     const std::string& targetName,
                     const std::string& taskName);

    void executeTaskRange(const ProjectConfig* projectConfig,
                          const std::string& targetName,
                          const std::string& startTaskName,
                          const std::string& endTaskName);

private:
    const StargateConfig& _config;
    std::unique_ptr<FlowManager> _flowManager;

    void createOutputDir();
    void writeTargets(const ProjectConfig* projConfig);
    void writeTargetFileList(const ProjectTarget* target,
                             const std::string& basePath,
                             const std::string& fileListPath);

    Flow* getTargetFlow(const ProjectTarget* target);
    void executeSection(FlowSection* section);
    void executeSection(FlowSection* section, size_t startIdx, size_t endIdx);
    bool checkSectionDependencies(Flow* flow, FlowSection* section);
    bool checkTaskDependencies(FlowSection* section, size_t taskIdx);
};

}

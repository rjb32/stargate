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
class DistribFlowManager;
class DistribFlow;
class DistribConfig;

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

    void infraInit(const ProjectConfig* projectConfig, bool dryMode);
    void infraLs(const ProjectConfig* projectConfig);
    void infraStart(const ProjectConfig* projectConfig);
    void infraStop(const ProjectConfig* projectConfig);
    void infraDestroy(const ProjectConfig* projectConfig);

private:
    const StargateConfig& _config;
    std::unique_ptr<FlowManager> _flowManager;
    std::unique_ptr<DistribFlowManager> _distribFlowManager;

    void createOutputDir();
    void writeTargets(const ProjectConfig* projConfig);
    void writeTargetFileList(const ProjectTarget* target,
                             const std::string& basePath,
                             const std::string& fileListPath);

    Flow* getTargetFlow(const ProjectTarget* target);
    void getDistribFlow(const ProjectConfig* projectConfig,
                        DistribFlow*& flow,
                        const DistribConfig*& distribConfig);
    void executeSection(FlowSection* section);
    void executeSection(FlowSection* section, size_t startIdx, size_t endIdx);
    bool checkSectionDependencies(Flow* flow, FlowSection* section);
    bool checkTaskDependencies(FlowSection* section, size_t taskIdx);
};

}

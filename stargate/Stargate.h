#pragma once

#include <string>

namespace stargate {

class ProjectConfig;
class StargateConfig;
class ProjectTarget;

class Stargate {
public:
    Stargate(const StargateConfig& config);
    ~Stargate();

    void clean();

    void run(const ProjectConfig* projectConfig);

private:
    const StargateConfig& _config;

    void createOutputDir();
    void writeTargets(const ProjectConfig* projConfig);
    void writeTargetFileList(const ProjectTarget* target,
                             const std::string& basePath,
                             const std::string& fileListPath);
};

}

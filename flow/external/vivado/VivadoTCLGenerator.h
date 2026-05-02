#pragma once

#include <string>

namespace stargate {

class FlowManager;
class ProjectTarget;

class VivadoTCLGenerator {
public:
    VivadoTCLGenerator(const FlowManager* manager, const ProjectTarget* target);
    ~VivadoTCLGenerator();

    void writeSynthTcl(const std::string& outputDir);
    void writeImplTcl(const std::string& outputDir);
    void writeBitstreamTcl(const std::string& outputDir);

private:
    const FlowManager* _manager {nullptr};
    const ProjectTarget* _target {nullptr};

    void requireTop() const;
    void requirePart() const;
};

}

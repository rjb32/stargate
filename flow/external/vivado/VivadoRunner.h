#pragma once

#include <string>

namespace stargate {

class FlowManager;

class VivadoRunner {
public:
    VivadoRunner(const FlowManager* manager, const std::string& workingDir);
    ~VivadoRunner();

    int runTcl(const std::string& tclPath, const std::string& logBaseName);

private:
    const FlowManager* _manager {nullptr};
    std::string _workingDir;
};

}

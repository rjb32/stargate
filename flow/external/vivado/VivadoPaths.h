#pragma once

#include <string>

namespace stargate {

class FlowManager;
class ProjectTarget;

class VivadoPaths {
public:
    static void getFileListPath(const FlowManager* manager,
                                const ProjectTarget* target,
                                std::string& result);

    static void getSynthDir(const FlowManager* manager, std::string& result);
    static void getImplDir(const FlowManager* manager, std::string& result);
    static void getBitstreamDir(const FlowManager* manager, std::string& result);

    static void getSynthCheckpoint(const FlowManager* manager, std::string& result);
    static void getImplCheckpoint(const FlowManager* manager, std::string& result);
};

}

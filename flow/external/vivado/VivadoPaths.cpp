#include "VivadoPaths.h"

#include "FlowManager.h"

#include "ProjectTarget.h"

using namespace stargate;

static const std::string VIVADO_FLOW_NAME = "vivado";
static const std::string SYNTH_TASK_NAME = "synth";
static const std::string IMPL_TASK_NAME = "impl";
static const std::string BITSTREAM_TASK_NAME = "bitstream";
static const std::string FILES_LIST_NAME = "files.list";
static const std::string SYNTH_DCP_NAME = "synth.dcp";
static const std::string IMPL_DCP_NAME = "impl.dcp";

void VivadoPaths::getFileListPath(const FlowManager* manager,
                                  const ProjectTarget* target,
                                  std::string& result) {
    result = manager->getOutputDir();
    result += "/project/";
    result += target->getName();
    result += "/";
    result += FILES_LIST_NAME;
}

void VivadoPaths::getSynthDir(const FlowManager* manager, std::string& result) {
    result = manager->getOutputDir();
    result += "/";
    result += VIVADO_FLOW_NAME;
    result += "/";
    result += SYNTH_TASK_NAME;
}

void VivadoPaths::getImplDir(const FlowManager* manager, std::string& result) {
    result = manager->getOutputDir();
    result += "/";
    result += VIVADO_FLOW_NAME;
    result += "/";
    result += IMPL_TASK_NAME;
}

void VivadoPaths::getBitstreamDir(const FlowManager* manager, std::string& result) {
    result = manager->getOutputDir();
    result += "/";
    result += VIVADO_FLOW_NAME;
    result += "/";
    result += BITSTREAM_TASK_NAME;
}

void VivadoPaths::getSynthCheckpoint(const FlowManager* manager,
                                     std::string& result) {
    getSynthDir(manager, result);
    result += "/";
    result += SYNTH_DCP_NAME;
}

void VivadoPaths::getImplCheckpoint(const FlowManager* manager,
                                    std::string& result) {
    getImplDir(manager, result);
    result += "/";
    result += IMPL_DCP_NAME;
}

#include "ProjectTarget.h"

#include "ProjectConfig.h"

using namespace stargate;

ProjectTarget::ProjectTarget(const std::string& name)
    : _name(name)
{
}

ProjectTarget::~ProjectTarget() {
}

ProjectTarget* ProjectTarget::create(ProjectConfig* config, const std::string& name) {
    ProjectTarget* target = new ProjectTarget(name);
    config->addTarget(target);
    return target;
}

void ProjectTarget::addFileSet(const FileSet* fileset) {
    _filesets.push_back(fileset);
}

void ProjectTarget::setFlowName(const std::string& flowName) {
    _flowName = flowName;
}

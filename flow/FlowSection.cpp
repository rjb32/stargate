#include "FlowSection.h"

#include "Flow.h"
#include "FlowTask.h"

using namespace stargate;

FlowSection::FlowSection(Flow* parent, const std::string& name)
    : _parent(parent),
    _name(name)
{
}

FlowSection::~FlowSection() {
    for (FlowTask* task : _tasks) {
        delete task;
    }
}

FlowSection* FlowSection::create(Flow* parent, const std::string& name) {
    FlowSection* section = new FlowSection(parent, name);
    parent->addSection(section);
    return section;
}

void FlowSection::addTask(FlowTask* task) {
    _tasks.push_back(task);
    _taskNameMap[task->getName()] = task;
}

FlowTask* FlowSection::getTask(std::string_view name) const {
    const auto it = _taskNameMap.find(name);
    if (it == _taskNameMap.end()) {
        return nullptr;
    }

    return it->second;
}

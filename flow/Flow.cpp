#include "Flow.h"

#include "FlowSection.h"

using namespace stargate;

Flow::Flow()
{
}

Flow::~Flow() {
    for (FlowSection* section : _sections) {
        delete section;
    }
}

void Flow::addSection(FlowSection* section) {
    _sections.push_back(section);
    _sectionNameMap[section->getName()] = section;
}

FlowSection* Flow::getSection(std::string_view name) const {
    const auto it = _sectionNameMap.find(name);
    if (it == _sectionNameMap.end()) {
        return nullptr;
    }

    return it->second;
}

FlowSection* Flow::getBuildSection() const {
    return getSection("build");
}

FlowSection* Flow::getRunSection() const {
    return getSection("run");
}

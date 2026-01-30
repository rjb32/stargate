#pragma once

#include <string_view>
#include <vector>
#include <unordered_map>

namespace stargate {

class FlowManager;
class FlowSection;

class Flow {
public:
    friend FlowManager;
    using Sections = std::vector<FlowSection*>;
    using SectionNameMap = std::unordered_map<std::string_view, FlowSection*>;

    virtual ~Flow();
    virtual std::string_view getName() const = 0;

    const Sections& sections() const { return _sections; }
    FlowSection* getSection(std::string_view name) const;
    FlowSection* getBuildSection() const;
    FlowSection* getRunSection() const;

    FlowManager* getManager() const { return _manager; }

protected:
    friend FlowSection;

    Flow();

    void addSection(FlowSection* section);

private:
    FlowManager* _manager {nullptr};
    Sections _sections;
    SectionNameMap _sectionNameMap;
};

}

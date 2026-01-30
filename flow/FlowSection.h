#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace stargate {

class Flow;
class FlowTask;
class VivadoSynthTask;
class VivadoImplTask;
class VivadoBitstreamTask;

class FlowSection {
public:
    friend Flow;
    using Tasks = std::vector<FlowTask*>;
    using TaskNameMap = std::unordered_map<std::string_view, FlowTask*>;

    static FlowSection* create(Flow* parent, const std::string& name);

    Flow* getParent() const { return _parent; }
    const std::string& getName() const { return _name; }
    const Tasks& tasks() const { return _tasks; }
    FlowTask* getTask(std::string_view name) const;

private:
    friend FlowTask;
    friend VivadoSynthTask;
    friend VivadoImplTask;
    friend VivadoBitstreamTask;

    explicit FlowSection(Flow* parent, const std::string& name);
    ~FlowSection();

    void addTask(FlowTask* task);

    Flow* _parent {nullptr};
    std::string _name;
    Tasks _tasks;
    TaskNameMap _taskNameMap;
};

}

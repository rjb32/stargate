#pragma once

#include "FlowTask.h"

namespace stargate {

class FlowSection;
class ProjectTarget;

class VivadoBitstreamTask : public FlowTask {
public:
    static VivadoBitstreamTask* create(FlowSection* parent);

    std::string_view getName() const override { return "bitstream"; }
    void execute(const ProjectTarget* target) override;

private:
    explicit VivadoBitstreamTask(FlowSection* parent);
};

}

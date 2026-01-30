#pragma once

#include "FlowTask.h"

namespace stargate {

class FlowSection;

class VivadoImplTask : public FlowTask {
public:
    static VivadoImplTask* create(FlowSection* parent);

    std::string_view getName() const override { return "impl"; }
    void execute() override;

private:
    explicit VivadoImplTask(FlowSection* parent);
};

}

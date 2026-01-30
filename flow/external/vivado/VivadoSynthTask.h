#pragma once

#include "FlowTask.h"

namespace stargate {

class FlowSection;

class VivadoSynthTask : public FlowTask {
public:
    static VivadoSynthTask* create(FlowSection* parent);

    std::string_view getName() const override { return "synth"; }
    void execute() override;

private:
    explicit VivadoSynthTask(FlowSection* parent);
};

}

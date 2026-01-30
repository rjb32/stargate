#pragma once

#include "Flow.h"
#include "FlowTask.h"

namespace stargate {

class FlowManager;
class FlowSection;

class VivadoFlow : public Flow {
public:
    static VivadoFlow* create(FlowManager* manager);

    std::string_view getName() const override { return "vivado"; }

private:
    VivadoFlow();

    void initializeSections();
};

class VivadoSynthTask : public FlowTask {
public:
    static VivadoSynthTask* create(FlowSection* parent);

    std::string_view getName() const override { return "synth"; }
    void execute() override;

private:
    explicit VivadoSynthTask(FlowSection* parent);
};

class VivadoImplTask : public FlowTask {
public:
    static VivadoImplTask* create(FlowSection* parent);

    std::string_view getName() const override { return "impl"; }
    void execute() override;

private:
    explicit VivadoImplTask(FlowSection* parent);
};

class VivadoBitstreamTask : public FlowTask {
public:
    static VivadoBitstreamTask* create(FlowSection* parent);

    std::string_view getName() const override { return "bitstream"; }
    void execute() override;

private:
    explicit VivadoBitstreamTask(FlowSection* parent);
};

}

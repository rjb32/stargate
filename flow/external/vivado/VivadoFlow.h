#pragma once

#include "Flow.h"

namespace stargate {

class FlowManager;
class FlowSection;

class VivadoFlow : public Flow {
public:
    static VivadoFlow* create(FlowManager* manager);

    std::string_view getName() const override { return "vivado"; }

private:
    VivadoFlow();

    void initSections();
};

}

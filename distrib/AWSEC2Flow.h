#pragma once

#include "DistribFlow.h"

namespace stargate {

class DistribFlowManager;

class AWSEC2Flow : public DistribFlow {
public:
    static AWSEC2Flow* create(DistribFlowManager* manager);

    std::string_view getName() const override { return "awsec2"; }

    void init(const DistribConfig* config) override;

    int runCommand(const std::string& commandScriptPath) override;

private:
    AWSEC2Flow();
};

}

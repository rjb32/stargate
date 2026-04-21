#pragma once

#include <string>

#include "DistribFlow.h"

namespace stargate {

class AWSEC2Config;
class DistribFlowManager;

class AWSEC2Flow : public DistribFlow {
public:
    static AWSEC2Flow* create(DistribFlowManager* manager);

    std::string_view getName() const override { return "awsec2"; }

    void init(const DistribConfig* config, bool dryMode) override;

    int runCommand(const std::string& commandScriptPath) override;

private:
    AWSEC2Flow();

    std::string ensureVPC(const AWSEC2Config* config, bool dryMode);
    std::string ensureSubnet(const AWSEC2Config* config,
                             const std::string& vpcId,
                             bool dryMode);
    std::string ensureSecurityGroup(const AWSEC2Config* config,
                                    const std::string& vpcId,
                                    bool dryMode);
    std::string resolveAMI(const AWSEC2Config* config, bool dryMode);

    void writeAWSInfra(const AWSEC2Config* config,
                       const std::string& path,
                       const std::string& vpcId,
                       const std::string& subnetId,
                       const std::string& securityGroupId,
                       const std::string& amiId,
                       const std::string& buildInstanceName);
};

}

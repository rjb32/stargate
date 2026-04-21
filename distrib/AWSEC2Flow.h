#pragma once

#include <string>

#include "DistribFlow.h"

namespace stargate {

class AWSCLI;
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

    void logConfig(const AWSEC2Config* config);
    void logDryActions(const AWSEC2Config* config);

    void provision(const AWSEC2Config* config);

    void ensureVPC(AWSCLI& cli,
                   const AWSEC2Config* config,
                   std::string& vpcId);
    void findFirstAvailabilityZone(AWSCLI& cli, std::string& azName);
    void ensureSubnet(AWSCLI& cli,
                      const AWSEC2Config* config,
                      const std::string& vpcId,
                      std::string& subnetId);
    void ensureInternetGateway(AWSCLI& cli,
                               const std::string& vpcId,
                               std::string& igwId);
    void ensurePublicRouteTable(AWSCLI& cli,
                                const std::string& vpcId,
                                const std::string& igwId,
                                const std::string& subnetId,
                                std::string& routeTableId);
    void ensureSecurityGroup(AWSCLI& cli,
                             const std::string& vpcId,
                             std::string& securityGroupId);
    void resolveAMI(AWSCLI& cli,
                    const AWSEC2Config* config,
                    std::string& amiId);

    void writeAWSInfra(const AWSEC2Config* config,
                       const std::string& path,
                       const std::string& vpcId,
                       const std::string& subnetId,
                       const std::string& securityGroupId,
                       const std::string& amiId,
                       const std::string& buildInstanceName);
};

}

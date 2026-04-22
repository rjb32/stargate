#pragma once

#include <string>
#include <vector>

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
    void ls(const DistribConfig* config) override;
    void start(const DistribConfig* config) override;
    void stop(const DistribConfig* config) override;
    void destroy(const DistribConfig* config) override;

    int runCommand(const std::string& commandScriptPath) override;

private:
    AWSEC2Flow();

    void logConfig(const AWSEC2Config* config);
    void logDryActions(const AWSEC2Config* config);

    void provision(const AWSEC2Config* config);

    struct LsRow {
        std::string type;
        std::string name;
        std::string id;
        std::string state;
        std::string details;
    };

    void collectLsRows(AWSCLI& cli,
                       const AWSEC2Config* config,
                       std::vector<LsRow>& rows);
    void printLsTable(const std::vector<LsRow>& rows);

    void lookupVPC(AWSCLI& cli,
                   const std::string& name,
                   std::string& id);
    void lookupSubnet(AWSCLI& cli,
                      const std::string& name,
                      const std::string& vpcId,
                      std::string& id);
    void lookupIGW(AWSCLI& cli,
                   const std::string& vpcId,
                   std::string& id);
    void lookupRouteTable(AWSCLI& cli,
                          const std::string& vpcId,
                          std::string& id);
    void lookupSecurityGroup(AWSCLI& cli,
                             const std::string& vpcId,
                             std::string& id);
    void lookupBuildInstances(AWSCLI& cli,
                              const std::string& vpcId,
                              std::vector<std::string>& instanceIds);

    void destroyInstances(AWSCLI& cli,
                          const std::vector<std::string>& instanceIds);
    void destroyRouteTable(AWSCLI& cli, const std::string& routeTableId);
    void destroyIGW(AWSCLI& cli,
                    const std::string& igwId,
                    const std::string& vpcId);
    void removeLocalFlowState(const std::string& flowDir,
                              const std::string& pemPath);

    void createAndAttachIGW(AWSCLI& cli,
                            const std::string& vpcId,
                            std::string& igwId);
    bool hasDefaultRoute(AWSCLI& cli,
                         const std::string& routeTableId);
    void addDefaultRoute(AWSCLI& cli,
                         const std::string& routeTableId,
                         const std::string& igwId);
    void deleteDefaultRoute(AWSCLI& cli,
                            const std::string& routeTableId);

    void detectExistingInfra(AWSCLI& cli,
                             const AWSEC2Config* config,
                             std::vector<std::string>& found);

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
    void ensureKeyPair(AWSCLI& cli,
                       const AWSEC2Config* config,
                       const std::string& flowDir,
                       std::string& pemPath);
    void resolveAMI(AWSCLI& cli,
                    const AWSEC2Config* config,
                    std::string& amiId);
    void ensureBuildInstance(AWSCLI& cli,
                             const AWSEC2Config* config,
                             const std::string& subnetId,
                             const std::string& securityGroupId,
                             const std::string& amiId,
                             std::string& instanceId,
                             std::string& instanceName,
                             std::string& publicIP);

    void writeAWSInfra(const AWSEC2Config* config,
                       const std::string& path,
                       const std::string& vpcId,
                       const std::string& subnetId,
                       const std::string& securityGroupId,
                       const std::string& amiId,
                       const std::string& pemPath,
                       const std::string& buildInstanceName,
                       const std::string& buildInstanceId,
                       const std::string& buildInstancePublicIP);
};

}

#include "AWSEC2Flow.h"

#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

#include <spdlog/spdlog.h>

#include "AWSCLI.h"
#include "AWSEC2Config.h"
#include "DistribConfig.h"
#include "DistribFlowManager.h"

#include "Command.h"
#include "CommandExecutor.h"
#include "FileUtils.h"
#include "Panic.h"

using namespace stargate;

namespace {

constexpr const char* BASH_BINARY = "/bin/bash";
constexpr const char* AWSEC2_SUBDIR_NAME = "awsec2";
constexpr const char* AWS_INFRA_FILE_NAME = "aws_infra.toml";
constexpr const char* BUILD_INSTANCE_NAME_PREFIX = "stargate-build-";
constexpr size_t INSTANCE_ID_LENGTH = 8;

constexpr const char* DEFAULT_VPC_CIDR = "10.0.0.0/16";
constexpr const char* DEFAULT_SUBNET_CIDR = "10.0.1.0/24";
constexpr const char* SECURITY_GROUP_NAME = "stargate-sg";
constexpr const char* SECURITY_GROUP_DESCRIPTION =
    "Stargate security group (SSH ingress)";
constexpr const char* ROUTE_TABLE_NAME = "stargate-public-rtb";
constexpr const char* IGW_NAME = "stargate-igw";
constexpr const char* AWS_NONE = "None";
constexpr const char* XILINX_AMI_OWNER = "aws-marketplace";
constexpr const char* XILINX_AMI_NAME_FILTER = "*Vivado*";
constexpr int SSH_PORT = 22;
constexpr const char* SSH_BANNER =
    "========================================================================";

constexpr const char* LS_TYPE_VPC = "VPC";
constexpr const char* LS_TYPE_SUBNET = "Subnet";
constexpr const char* LS_TYPE_IGW = "InternetGateway";
constexpr const char* LS_TYPE_RTB = "RouteTable";
constexpr const char* LS_TYPE_SG = "SecurityGroup";
constexpr const char* LS_TYPE_KEYPAIR = "KeyPair";
constexpr const char* LS_TYPE_INSTANCE = "BuildInstance";
constexpr const char* LS_STATE_MISSING = "missing";
constexpr const char* LS_STATE_PRESENT = "present";

std::string joinPath(const std::string& dir, const std::string& name) {
    if (dir.empty()) {
        return name;
    }
    if (dir.back() == '/') {
        return dir + name;
    }
    return dir + "/" + name;
}

std::string generateInstanceId() {
    std::random_device rd;
    std::stringstream ss;
    ss << std::hex << std::setw(INSTANCE_ID_LENGTH) << std::setfill('0')
       << (rd() & 0xffffffff);
    return ss.str();
}

bool isEmptyAWSResult(const std::string& value) {
    return value.empty() || value == AWS_NONE;
}

bool isYesAnswer(const std::string& answer) {
    return answer == "y" || answer == "Y" || answer == "yes" || answer == "YES";
}

void confirmCreate(const std::string& resourceDescription) {
    if (!isatty(STDIN_FILENO)) {
        panic("Cannot prompt for confirmation: stdin is not a terminal. "
              "Re-run {} interactively to create missing AWS resources.",
              resourceDescription);
    }

    std::cout << resourceDescription << " Create? [y/N]: " << std::flush;

    std::string answer;
    if (!std::getline(std::cin, answer)) {
        panic("Failed to read confirmation for: {}", resourceDescription);
    }

    if (!isYesAnswer(answer)) {
        panic("Aborted by user: {}", resourceDescription);
    }
}

void confirmDestroy(const std::vector<std::string>& toDestroy) {
    std::cout << "About to DESTROY the following Stargate resources:\n";
    for (const auto& entry : toDestroy) {
        std::cout << "  - " << entry << "\n";
    }
    std::cout << "This action is irreversible. Proceed? [y/N]: " << std::flush;

    if (!isatty(STDIN_FILENO)) {
        panic("Cannot prompt for destroy: stdin is not a terminal.");
    }

    std::string answer;
    if (!std::getline(std::cin, answer)) {
        panic("Failed to read destroy confirmation");
    }

    if (!isYesAnswer(answer)) {
        panic("Aborted by user: destroy declined");
    }
}

void confirmReuse(const std::vector<std::string>& found) {
    std::cout << "Found existing Stargate AWS resources:\n";
    for (const auto& entry : found) {
        std::cout << "  - " << entry << "\n";
    }
    std::cout << "Reuse these and finish the provision? [y/N]: " << std::flush;

    if (!isatty(STDIN_FILENO)) {
        panic("Cannot prompt for reuse: stdin is not a terminal. "
              "Re-run interactively, or delete the resources to provision fresh.");
    }

    std::string answer;
    if (!std::getline(std::cin, answer)) {
        panic("Failed to read reuse confirmation");
    }

    if (!isYesAnswer(answer)) {
        panic("Aborted by user: reuse declined. Delete the existing AWS "
              "resources (or pick different names in the distrib config) "
              "before re-running.");
    }
}

}

AWSEC2Flow::AWSEC2Flow()
    : DistribFlow()
{
}

AWSEC2Flow* AWSEC2Flow::create(DistribFlowManager* manager) {
    AWSEC2Flow* flow = new AWSEC2Flow();
    manager->addFlow(flow);
    return flow;
}

void AWSEC2Flow::init(const DistribConfig* config, bool dryMode) {
    if (!config) {
        panic("AWSEC2Flow::init requires a distrib config");
    }

    const AWSEC2Config* awsec2Config = config->getAWSEC2Config();
    if (!awsec2Config) {
        panic("AWSEC2Flow::init requires an awsec2 config section");
    }

    if (dryMode) {
        spdlog::info("AWSEC2 infra init: DRY RUN (no resources will be created)");
    }

    logConfig(awsec2Config);

    if (dryMode) {
        logDryActions(awsec2Config);
        return;
    }

    provision(awsec2Config);
}

void AWSEC2Flow::logConfig(const AWSEC2Config* config) {
    spdlog::info("AWSEC2 infra init: validating distrib config");
    spdlog::info("  region              = {}", config->getRegion());
    spdlog::info("  profile             = {}", config->getProfile());
    spdlog::info("  key_pair            = {}", config->getKeyPairName());
    spdlog::info("  ssh_user            = {}", config->getSSHUser());
    spdlog::info("  vpc                 = {}", config->getVPCName());
    spdlog::info("  public_subnet       = {}", config->getPublicSubnetName());
    spdlog::info("  build_instance_type = {}", config->getBuildInstanceType());
    spdlog::info("  fpga_instance_type  = {}", config->getFPGAInstanceType());
    spdlog::info("  autostop            = {}", config->getAutostop());
}

void AWSEC2Flow::logDryActions(const AWSEC2Config* config) {
    spdlog::info("AWSEC2 infra init: [dry] ensuring VPC '{}' exists in region {}",
                 config->getVPCName(), config->getRegion());
    spdlog::info("AWSEC2 infra init: [dry] ensuring public subnet '{}' in VPC",
                 config->getPublicSubnetName());
    spdlog::info("AWSEC2 infra init: [dry] ensuring internet gateway "
                 "attached to VPC");
    spdlog::info("AWSEC2 infra init: [dry] ensuring public route table "
                 "with default route via IGW");
    spdlog::info("AWSEC2 infra init: [dry] ensuring security group "
                 "(SSH ingress only)");
    spdlog::info("AWSEC2 infra init: [dry] ensuring key pair '{}' "
                 "(pem will be written under the awsec2 flow dir)",
                 config->getKeyPairName());
    if (!config->getAMIID().empty()) {
        spdlog::info("AWSEC2 infra init: [dry] using configured AMI {}",
                     config->getAMIID());
    } else {
        spdlog::info("AWSEC2 infra init: [dry] resolving latest Vivado AMI "
                     "from Xilinx in region {}",
                     config->getRegion());
    }
    const std::string buildInstanceName =
        std::string(BUILD_INSTANCE_NAME_PREFIX) + generateInstanceId();
    spdlog::info("AWSEC2 infra init: build instance name = {}",
                 buildInstanceName);
    spdlog::info("AWSEC2 infra init: [dry] launching build instance '{}' "
                 "(type {}) and waiting for public IP",
                 buildInstanceName, config->getBuildInstanceType());

    DistribFlowManager* manager = getManager();
    const std::string& distribDir = manager->getDistribDir();
    const std::string flowDir = joinPath(distribDir, AWSEC2_SUBDIR_NAME);
    const std::string pemPath =
        joinPath(flowDir, config->getKeyPairName() + ".pem");
    spdlog::info(SSH_BANNER);
    spdlog::info("AWSEC2 infra init: [dry] to ssh into the build instance, "
                 "you will run:");
    spdlog::info("    ssh -i {} {}@<INSTANCE_PUBLIC_IP>",
                 pemPath, config->getSSHUser());
    spdlog::info(SSH_BANNER);

    spdlog::info("AWSEC2 infra init: dry run complete, "
                 "skipping aws_infra.toml write");
}

void AWSEC2Flow::provision(const AWSEC2Config* config) {
    DistribFlowManager* manager = getManager();
    const std::string& distribDir = manager->getDistribDir();
    if (distribDir.empty()) {
        panic("AWSEC2Flow::init requires the distrib output directory to be set");
    }

    const std::string flowDir = joinPath(distribDir, AWSEC2_SUBDIR_NAME);
    const std::string awsInfraPath = joinPath(flowDir, AWS_INFRA_FILE_NAME);
    if (FileUtils::exists(awsInfraPath)) {
        spdlog::info("AWSEC2 infra init: already provisioned, "
                     "reusing {} (delete it to force reprovision)",
                     awsInfraPath);
        return;
    }

    if (!FileUtils::exists(flowDir)) {
        FileUtils::createDirectory(flowDir);
    }

    AWSCLI cli;
    cli.setRegion(config->getRegion());
    cli.setProfile(config->getProfile());

    spdlog::info("AWSEC2 infra init: probing AWS for existing Stargate resources");
    std::vector<std::string> foundResources;
    detectExistingInfra(cli, config, foundResources);
    if (!foundResources.empty()) {
        confirmReuse(foundResources);
        spdlog::info("AWSEC2 infra init: reusing existing Stargate AWS resources");
    }

    std::string vpcId;
    ensureVPC(cli, config, vpcId);

    std::string subnetId;
    ensureSubnet(cli, config, vpcId, subnetId);

    std::string igwId;
    ensureInternetGateway(cli, vpcId, igwId);

    std::string routeTableId;
    ensurePublicRouteTable(cli, vpcId, igwId, subnetId, routeTableId);

    std::string securityGroupId;
    ensureSecurityGroup(cli, vpcId, securityGroupId);

    std::string pemPath;
    ensureKeyPair(cli, config, flowDir, pemPath);

    std::string amiId;
    resolveAMI(cli, config, amiId);

    std::string buildInstanceId;
    std::string buildInstanceName;
    std::string buildInstancePublicIP;
    ensureBuildInstance(cli, config, subnetId, securityGroupId, amiId,
                        buildInstanceId, buildInstanceName,
                        buildInstancePublicIP);

    writeAWSInfra(config,
                  awsInfraPath,
                  vpcId,
                  subnetId,
                  securityGroupId,
                  amiId,
                  pemPath,
                  buildInstanceName,
                  buildInstanceId,
                  buildInstancePublicIP);

    spdlog::info("AWSEC2 infra init: wrote {}", awsInfraPath);
    spdlog::info(SSH_BANNER);
    spdlog::info("AWSEC2 infra init: to ssh into the build instance, run:");
    spdlog::info("    ssh -i {} {}@{}",
                 pemPath, config->getSSHUser(), buildInstancePublicIP);
    spdlog::info(SSH_BANNER);
}

void AWSEC2Flow::lookupVPC(AWSCLI& cli,
                           const std::string& name,
                           std::string& id) {
    cli.run({"ec2", "describe-vpcs",
             "--filters", "Name=tag:Name,Values=" + name,
             "--query", "Vpcs[0].VpcId",
             "--output", "text"},
            id);
    if (isEmptyAWSResult(id)) {
        id.clear();
    }
}

void AWSEC2Flow::lookupSubnet(AWSCLI& cli,
                              const std::string& name,
                              const std::string& vpcId,
                              std::string& id) {
    if (vpcId.empty()) {
        id.clear();
        return;
    }
    cli.run({"ec2", "describe-subnets",
             "--filters",
             "Name=tag:Name,Values=" + name,
             "Name=vpc-id,Values=" + vpcId,
             "--query", "Subnets[0].SubnetId",
             "--output", "text"},
            id);
    if (isEmptyAWSResult(id)) {
        id.clear();
    }
}

void AWSEC2Flow::lookupIGW(AWSCLI& cli,
                           const std::string& vpcId,
                           std::string& id) {
    if (vpcId.empty()) {
        id.clear();
        return;
    }
    cli.run({"ec2", "describe-internet-gateways",
             "--filters", "Name=attachment.vpc-id,Values=" + vpcId,
             "--query", "InternetGateways[0].InternetGatewayId",
             "--output", "text"},
            id);
    if (isEmptyAWSResult(id)) {
        id.clear();
    }
}

void AWSEC2Flow::lookupRouteTable(AWSCLI& cli,
                                  const std::string& vpcId,
                                  std::string& id) {
    if (vpcId.empty()) {
        id.clear();
        return;
    }
    cli.run({"ec2", "describe-route-tables",
             "--filters",
             std::string("Name=tag:Name,Values=") + ROUTE_TABLE_NAME,
             "Name=vpc-id,Values=" + vpcId,
             "--query", "RouteTables[0].RouteTableId",
             "--output", "text"},
            id);
    if (isEmptyAWSResult(id)) {
        id.clear();
    }
}

void AWSEC2Flow::lookupSecurityGroup(AWSCLI& cli,
                                     const std::string& vpcId,
                                     std::string& id) {
    if (vpcId.empty()) {
        id.clear();
        return;
    }
    cli.run({"ec2", "describe-security-groups",
             "--filters",
             std::string("Name=group-name,Values=") + SECURITY_GROUP_NAME,
             "Name=vpc-id,Values=" + vpcId,
             "--query", "SecurityGroups[0].GroupId",
             "--output", "text"},
            id);
    if (isEmptyAWSResult(id)) {
        id.clear();
    }
}

void AWSEC2Flow::lookupBuildInstances(AWSCLI& cli,
                                      const std::string& vpcId,
                                      std::vector<std::string>& instanceIds) {
    instanceIds.clear();
    if (vpcId.empty()) {
        return;
    }

    std::string raw;
    cli.run({"ec2", "describe-instances",
             "--filters",
             std::string("Name=tag:Name,Values=")
                 + BUILD_INSTANCE_NAME_PREFIX + "*",
             "Name=vpc-id,Values=" + vpcId,
             std::string("Name=instance-state-name,")
                 + "Values=pending,running,stopping,stopped",
             "--query", "Reservations[].Instances[].InstanceId",
             "--output", "text"},
            raw);

    if (isEmptyAWSResult(raw)) {
        return;
    }

    std::istringstream iss(raw);
    std::string token;
    while (iss >> token) {
        if (!token.empty()) {
            instanceIds.push_back(token);
        }
    }
}

void AWSEC2Flow::createAndAttachIGW(AWSCLI& cli,
                                    const std::string& vpcId,
                                    std::string& igwId) {
    spdlog::info("AWSEC2: creating internet gateway");
    cli.run({"ec2", "create-internet-gateway",
             "--tag-specifications",
             std::string("ResourceType=internet-gateway,")
                 + "Tags=[{Key=Name,Value=" + IGW_NAME + "}]",
             "--query", "InternetGateway.InternetGatewayId",
             "--output", "text"},
            igwId);

    std::string attachOut;
    cli.run({"ec2", "attach-internet-gateway",
             "--internet-gateway-id", igwId,
             "--vpc-id", vpcId},
            attachOut);

    spdlog::info("AWSEC2: created and attached IGW {}", igwId);
}

bool AWSEC2Flow::hasDefaultRoute(AWSCLI& cli,
                                 const std::string& routeTableId) {
    std::string gw;
    cli.run({"ec2", "describe-route-tables",
             "--route-table-ids", routeTableId,
             "--query",
             std::string("RouteTables[0].Routes")
                 + "[?DestinationCidrBlock=='0.0.0.0/0']|[0].GatewayId",
             "--output", "text"},
            gw);
    return !isEmptyAWSResult(gw);
}

void AWSEC2Flow::addDefaultRoute(AWSCLI& cli,
                                 const std::string& routeTableId,
                                 const std::string& igwId) {
    spdlog::info("AWSEC2: adding default route in {} via {}",
                 routeTableId, igwId);
    std::string out;
    cli.run({"ec2", "create-route",
             "--route-table-id", routeTableId,
             "--destination-cidr-block", "0.0.0.0/0",
             "--gateway-id", igwId},
            out);
}

void AWSEC2Flow::deleteDefaultRoute(AWSCLI& cli,
                                    const std::string& routeTableId) {
    if (!hasDefaultRoute(cli, routeTableId)) {
        return;
    }
    spdlog::info("AWSEC2: deleting default route from {}", routeTableId);
    std::string out;
    cli.run({"ec2", "delete-route",
             "--route-table-id", routeTableId,
             "--destination-cidr-block", "0.0.0.0/0"},
            out);
}

void AWSEC2Flow::start(const DistribConfig* config) {
    if (!config) {
        panic("AWSEC2Flow::start requires a distrib config");
    }

    const AWSEC2Config* awsec2Config = config->getAWSEC2Config();
    if (!awsec2Config) {
        panic("AWSEC2Flow::start requires an awsec2 config section");
    }

    AWSCLI cli;
    cli.setRegion(awsec2Config->getRegion());
    cli.setProfile(awsec2Config->getProfile());

    std::string vpcId;
    lookupVPC(cli, awsec2Config->getVPCName(), vpcId);
    if (vpcId.empty()) {
        panic("AWSEC2 infra start: VPC '{}' not found; run 'infra init' first",
              awsec2Config->getVPCName());
    }

    std::string igwId;
    lookupIGW(cli, vpcId, igwId);
    if (igwId.empty()) {
        createAndAttachIGW(cli, vpcId, igwId);
    }

    std::string routeTableId;
    lookupRouteTable(cli, vpcId, routeTableId);
    if (routeTableId.empty()) {
        panic("AWSEC2 infra start: route table '{}' not found; "
              "run 'infra init' first", ROUTE_TABLE_NAME);
    }
    if (!hasDefaultRoute(cli, routeTableId)) {
        addDefaultRoute(cli, routeTableId, igwId);
    }

    std::vector<std::string> instanceIds;
    lookupBuildInstances(cli, vpcId, instanceIds);
    if (instanceIds.empty()) {
        panic("AWSEC2 infra start: no build instance found; "
              "run 'infra init' first");
    }

    AWSCLI::Args startArgs = {"ec2", "start-instances", "--instance-ids"};
    for (const auto& instanceId : instanceIds) {
        spdlog::info("AWSEC2 infra start: starting instance {}", instanceId);
        startArgs.push_back(instanceId);
    }
    std::string startOut;
    cli.run(startArgs, startOut);

    AWSCLI::Args waitArgs =
        {"ec2", "wait", "instance-running", "--instance-ids"};
    for (const auto& instanceId : instanceIds) {
        waitArgs.push_back(instanceId);
    }
    std::string waitOut;
    cli.run(waitArgs, waitOut);

    for (const auto& instanceId : instanceIds) {
        std::string publicIP;
        cli.run({"ec2", "describe-instances",
                 "--instance-ids", instanceId,
                 "--query", "Reservations[0].Instances[0].PublicIpAddress",
                 "--output", "text"},
                publicIP);
        if (isEmptyAWSResult(publicIP)) {
            panic("Instance {} is running but has no public IP", instanceId);
        }
        spdlog::info("AWSEC2 infra start: instance {} is running "
                     "(public_ip={})", instanceId, publicIP);

        DistribFlowManager* manager = getManager();
        const std::string& distribDir = manager->getDistribDir();
        std::string pemPath;
        if (!distribDir.empty()) {
            const std::string flowDir =
                joinPath(distribDir, AWSEC2_SUBDIR_NAME);
            pemPath = joinPath(flowDir,
                               awsec2Config->getKeyPairName() + ".pem");
        }

        spdlog::info(SSH_BANNER);
        spdlog::info("AWSEC2 infra start: to ssh into the build instance, "
                     "run:");
        spdlog::info("    ssh -i {} {}@{}",
                     pemPath, awsec2Config->getSSHUser(), publicIP);
        spdlog::info(SSH_BANNER);
    }
}

void AWSEC2Flow::stop(const DistribConfig* config) {
    if (!config) {
        panic("AWSEC2Flow::stop requires a distrib config");
    }

    const AWSEC2Config* awsec2Config = config->getAWSEC2Config();
    if (!awsec2Config) {
        panic("AWSEC2Flow::stop requires an awsec2 config section");
    }

    AWSCLI cli;
    cli.setRegion(awsec2Config->getRegion());
    cli.setProfile(awsec2Config->getProfile());

    std::string vpcId;
    lookupVPC(cli, awsec2Config->getVPCName(), vpcId);

    std::vector<std::string> instanceIds;
    lookupBuildInstances(cli, vpcId, instanceIds);

    if (!instanceIds.empty()) {
        for (const auto& instanceId : instanceIds) {
            spdlog::info("AWSEC2 infra stop: stopping instance {}",
                         instanceId);
        }

        AWSCLI::Args stopArgs = {"ec2", "stop-instances", "--instance-ids"};
        for (const auto& instanceId : instanceIds) {
            stopArgs.push_back(instanceId);
        }
        std::string stopOut;
        cli.run(stopArgs, stopOut);

        AWSCLI::Args waitArgs =
            {"ec2", "wait", "instance-stopped", "--instance-ids"};
        for (const auto& instanceId : instanceIds) {
            waitArgs.push_back(instanceId);
        }
        std::string waitOut;
        cli.run(waitArgs, waitOut);

        for (const auto& instanceId : instanceIds) {
            spdlog::info("AWSEC2 infra stop: instance {} is stopped",
                         instanceId);
        }
    } else {
        spdlog::info("AWSEC2 infra stop: no build instance to stop");
    }

    std::string routeTableId;
    lookupRouteTable(cli, vpcId, routeTableId);
    if (!routeTableId.empty()) {
        deleteDefaultRoute(cli, routeTableId);
    }

    std::string igwId;
    lookupIGW(cli, vpcId, igwId);
    if (!igwId.empty()) {
        destroyIGW(cli, igwId, vpcId);
    } else {
        spdlog::info("AWSEC2 infra stop: no internet gateway to remove");
    }
}

void AWSEC2Flow::destroyInstances(AWSCLI& cli,
                                  const std::vector<std::string>& instanceIds) {
    if (instanceIds.empty()) {
        return;
    }

    for (const auto& instanceId : instanceIds) {
        spdlog::info("AWSEC2 infra destroy: terminating instance {}",
                     instanceId);
    }

    AWSCLI::Args termArgs = {"ec2", "terminate-instances", "--instance-ids"};
    for (const auto& instanceId : instanceIds) {
        termArgs.push_back(instanceId);
    }
    std::string termOut;
    cli.run(termArgs, termOut);

    AWSCLI::Args waitArgs =
        {"ec2", "wait", "instance-terminated", "--instance-ids"};
    for (const auto& instanceId : instanceIds) {
        waitArgs.push_back(instanceId);
    }
    std::string waitOut;
    cli.run(waitArgs, waitOut);

    spdlog::info("AWSEC2 infra destroy: terminated {} instance(s)",
                 instanceIds.size());
}

void AWSEC2Flow::destroyRouteTable(AWSCLI& cli,
                                   const std::string& routeTableId) {
    std::string assocIds;
    cli.run({"ec2", "describe-route-tables",
             "--route-table-ids", routeTableId,
             "--query",
             std::string("RouteTables[0].Associations")
                 + "[?Main==`false`].RouteTableAssociationId",
             "--output", "text"},
            assocIds);

    if (!isEmptyAWSResult(assocIds)) {
        std::istringstream iss(assocIds);
        std::string assoc;
        while (iss >> assoc) {
            if (assoc.empty()) {
                continue;
            }
            spdlog::info("AWSEC2 infra destroy: disassociating {} from "
                         "route table {}", assoc, routeTableId);
            std::string out;
            cli.run({"ec2", "disassociate-route-table",
                     "--association-id", assoc},
                    out);
        }
    }

    spdlog::info("AWSEC2 infra destroy: deleting route table {}",
                 routeTableId);
    std::string out;
    cli.run({"ec2", "delete-route-table",
             "--route-table-id", routeTableId},
            out);
}

void AWSEC2Flow::destroyIGW(AWSCLI& cli,
                            const std::string& igwId,
                            const std::string& vpcId) {
    spdlog::info("AWSEC2 infra destroy: detaching IGW {} from VPC {}",
                 igwId, vpcId);
    std::string detachOut;
    cli.run({"ec2", "detach-internet-gateway",
             "--internet-gateway-id", igwId,
             "--vpc-id", vpcId},
            detachOut);

    spdlog::info("AWSEC2 infra destroy: deleting IGW {}", igwId);
    std::string deleteOut;
    cli.run({"ec2", "delete-internet-gateway",
             "--internet-gateway-id", igwId},
            deleteOut);
}

void AWSEC2Flow::removeLocalFlowState(const std::string& flowDir,
                                      const std::string& pemPath) {
    if (!pemPath.empty() && FileUtils::exists(pemPath)) {
        spdlog::info("AWSEC2 infra destroy: removing local pem {}", pemPath);
        ::unlink(pemPath.c_str());
    }

    const std::string awsInfraPath = joinPath(flowDir, AWS_INFRA_FILE_NAME);
    if (FileUtils::exists(awsInfraPath)) {
        spdlog::info("AWSEC2 infra destroy: removing {}", awsInfraPath);
        ::unlink(awsInfraPath.c_str());
    }
}

void AWSEC2Flow::destroy(const DistribConfig* config) {
    if (!config) {
        panic("AWSEC2Flow::destroy requires a distrib config");
    }

    const AWSEC2Config* awsec2Config = config->getAWSEC2Config();
    if (!awsec2Config) {
        panic("AWSEC2Flow::destroy requires an awsec2 config section");
    }

    AWSCLI cli;
    cli.setRegion(awsec2Config->getRegion());
    cli.setProfile(awsec2Config->getProfile());

    std::string vpcId;
    lookupVPC(cli, awsec2Config->getVPCName(), vpcId);

    std::vector<std::string> instanceIds;
    lookupBuildInstances(cli, vpcId, instanceIds);

    std::string subnetId;
    lookupSubnet(cli, awsec2Config->getPublicSubnetName(), vpcId, subnetId);

    std::string igwId;
    lookupIGW(cli, vpcId, igwId);

    std::string routeTableId;
    lookupRouteTable(cli, vpcId, routeTableId);

    std::string securityGroupId;
    lookupSecurityGroup(cli, vpcId, securityGroupId);

    std::string keyName;
    cli.run({"ec2", "describe-key-pairs",
             "--filters",
             "Name=key-name,Values=" + awsec2Config->getKeyPairName(),
             "--query", "KeyPairs[0].KeyName",
             "--output", "text"},
            keyName);
    const bool haveKey = !isEmptyAWSResult(keyName);

    std::vector<std::string> summary;
    for (const auto& id : instanceIds) {
        summary.push_back("build instance " + id);
    }
    if (!securityGroupId.empty()) {
        summary.push_back(fmt::format("security group '{}' ({})",
                                      SECURITY_GROUP_NAME,
                                      securityGroupId));
    }
    if (!routeTableId.empty()) {
        summary.push_back(fmt::format("route table '{}' ({})",
                                      ROUTE_TABLE_NAME,
                                      routeTableId));
    }
    if (!igwId.empty()) {
        summary.push_back(fmt::format("internet gateway ({})", igwId));
    }
    if (!subnetId.empty()) {
        summary.push_back(fmt::format("subnet '{}' ({})",
                                      awsec2Config->getPublicSubnetName(),
                                      subnetId));
    }
    if (!vpcId.empty()) {
        summary.push_back(fmt::format("VPC '{}' ({})",
                                      awsec2Config->getVPCName(),
                                      vpcId));
    }
    if (haveKey) {
        summary.push_back(fmt::format("key pair '{}'",
                                      awsec2Config->getKeyPairName()));
    }

    DistribFlowManager* manager = getManager();
    const std::string& distribDir = manager->getDistribDir();
    std::string flowDir;
    std::string pemPath;
    if (!distribDir.empty()) {
        flowDir = joinPath(distribDir, AWSEC2_SUBDIR_NAME);
        pemPath = joinPath(flowDir,
                           awsec2Config->getKeyPairName() + ".pem");
    }
    if (!pemPath.empty() && FileUtils::exists(pemPath)) {
        summary.push_back("local pem " + pemPath);
    }
    const std::string awsInfraPath =
        flowDir.empty() ? std::string() : joinPath(flowDir, AWS_INFRA_FILE_NAME);
    if (!awsInfraPath.empty() && FileUtils::exists(awsInfraPath)) {
        summary.push_back("local " + awsInfraPath);
    }

    if (summary.empty()) {
        spdlog::info("AWSEC2 infra destroy: nothing to destroy");
        return;
    }

    confirmDestroy(summary);

    destroyInstances(cli, instanceIds);

    if (!securityGroupId.empty()) {
        spdlog::info("AWSEC2 infra destroy: deleting security group {}",
                     securityGroupId);
        std::string out;
        cli.run({"ec2", "delete-security-group",
                 "--group-id", securityGroupId},
                out);
    }

    if (!routeTableId.empty()) {
        destroyRouteTable(cli, routeTableId);
    }

    if (!igwId.empty() && !vpcId.empty()) {
        destroyIGW(cli, igwId, vpcId);
    }

    if (!subnetId.empty()) {
        spdlog::info("AWSEC2 infra destroy: deleting subnet {}", subnetId);
        std::string out;
        cli.run({"ec2", "delete-subnet",
                 "--subnet-id", subnetId},
                out);
    }

    if (!vpcId.empty()) {
        spdlog::info("AWSEC2 infra destroy: deleting VPC {}", vpcId);
        std::string out;
        cli.run({"ec2", "delete-vpc", "--vpc-id", vpcId}, out);
    }

    if (haveKey) {
        spdlog::info("AWSEC2 infra destroy: deleting key pair '{}'",
                     awsec2Config->getKeyPairName());
        std::string out;
        cli.run({"ec2", "delete-key-pair",
                 "--key-name", awsec2Config->getKeyPairName()},
                out);
    }

    if (!flowDir.empty()) {
        removeLocalFlowState(flowDir, pemPath);
    }

    spdlog::info("AWSEC2 infra destroy: done");
}

void AWSEC2Flow::ls(const DistribConfig* config) {
    if (!config) {
        panic("AWSEC2Flow::ls requires a distrib config");
    }

    const AWSEC2Config* awsec2Config = config->getAWSEC2Config();
    if (!awsec2Config) {
        panic("AWSEC2Flow::ls requires an awsec2 config section");
    }

    AWSCLI cli;
    cli.setRegion(awsec2Config->getRegion());
    cli.setProfile(awsec2Config->getProfile());

    std::vector<LsRow> rows;
    collectLsRows(cli, awsec2Config, rows);
    printLsTable(rows);
}

void AWSEC2Flow::collectLsRows(AWSCLI& cli,
                               const AWSEC2Config* config,
                               std::vector<LsRow>& rows) {
    std::string vpcId;
    cli.run({"ec2", "describe-vpcs",
             "--filters", "Name=tag:Name,Values=" + config->getVPCName(),
             "--query", "Vpcs[0].VpcId",
             "--output", "text"},
            vpcId);
    if (isEmptyAWSResult(vpcId)) {
        rows.push_back({LS_TYPE_VPC, config->getVPCName(), "",
                        LS_STATE_MISSING, ""});
    } else {
        std::string state;
        cli.run({"ec2", "describe-vpcs",
                 "--vpc-ids", vpcId,
                 "--query", "Vpcs[0].State",
                 "--output", "text"},
                state);
        std::string cidr;
        cli.run({"ec2", "describe-vpcs",
                 "--vpc-ids", vpcId,
                 "--query", "Vpcs[0].CidrBlock",
                 "--output", "text"},
                cidr);
        rows.push_back({LS_TYPE_VPC, config->getVPCName(), vpcId,
                        state, cidr});
    }

    std::string subnetId;
    if (!isEmptyAWSResult(vpcId)) {
        cli.run({"ec2", "describe-subnets",
                 "--filters",
                 "Name=tag:Name,Values=" + config->getPublicSubnetName(),
                 "Name=vpc-id,Values=" + vpcId,
                 "--query", "Subnets[0].SubnetId",
                 "--output", "text"},
                subnetId);
    }
    if (isEmptyAWSResult(subnetId)) {
        rows.push_back({LS_TYPE_SUBNET, config->getPublicSubnetName(), "",
                        LS_STATE_MISSING, ""});
    } else {
        std::string state;
        cli.run({"ec2", "describe-subnets",
                 "--subnet-ids", subnetId,
                 "--query", "Subnets[0].State",
                 "--output", "text"},
                state);
        std::string cidr;
        cli.run({"ec2", "describe-subnets",
                 "--subnet-ids", subnetId,
                 "--query", "Subnets[0].CidrBlock",
                 "--output", "text"},
                cidr);
        std::string az;
        cli.run({"ec2", "describe-subnets",
                 "--subnet-ids", subnetId,
                 "--query", "Subnets[0].AvailabilityZone",
                 "--output", "text"},
                az);
        rows.push_back({LS_TYPE_SUBNET, config->getPublicSubnetName(),
                        subnetId, state, cidr + " " + az});
    }

    std::string igwId;
    if (!isEmptyAWSResult(vpcId)) {
        cli.run({"ec2", "describe-internet-gateways",
                 "--filters", "Name=attachment.vpc-id,Values=" + vpcId,
                 "--query", "InternetGateways[0].InternetGatewayId",
                 "--output", "text"},
                igwId);
    }
    if (isEmptyAWSResult(igwId)) {
        rows.push_back({LS_TYPE_IGW, IGW_NAME, "",
                        LS_STATE_MISSING, ""});
    } else {
        rows.push_back({LS_TYPE_IGW, IGW_NAME, igwId,
                        "attached", "vpc " + vpcId});
    }

    std::string routeTableId;
    if (!isEmptyAWSResult(vpcId)) {
        cli.run({"ec2", "describe-route-tables",
                 "--filters",
                 std::string("Name=tag:Name,Values=") + ROUTE_TABLE_NAME,
                 "Name=vpc-id,Values=" + vpcId,
                 "--query", "RouteTables[0].RouteTableId",
                 "--output", "text"},
                routeTableId);
    }
    if (isEmptyAWSResult(routeTableId)) {
        rows.push_back({LS_TYPE_RTB, ROUTE_TABLE_NAME, "",
                        LS_STATE_MISSING, ""});
    } else {
        rows.push_back({LS_TYPE_RTB, ROUTE_TABLE_NAME, routeTableId,
                        LS_STATE_PRESENT, ""});
    }

    std::string securityGroupId;
    if (!isEmptyAWSResult(vpcId)) {
        cli.run({"ec2", "describe-security-groups",
                 "--filters",
                 std::string("Name=group-name,Values=") + SECURITY_GROUP_NAME,
                 "Name=vpc-id,Values=" + vpcId,
                 "--query", "SecurityGroups[0].GroupId",
                 "--output", "text"},
                securityGroupId);
    }
    if (isEmptyAWSResult(securityGroupId)) {
        rows.push_back({LS_TYPE_SG, SECURITY_GROUP_NAME, "",
                        LS_STATE_MISSING, ""});
    } else {
        rows.push_back({LS_TYPE_SG, SECURITY_GROUP_NAME, securityGroupId,
                        LS_STATE_PRESENT,
                        fmt::format("SSH tcp/{} 0.0.0.0/0", SSH_PORT)});
    }

    std::string keyName;
    cli.run({"ec2", "describe-key-pairs",
             "--filters", "Name=key-name,Values=" + config->getKeyPairName(),
             "--query", "KeyPairs[0].KeyName",
             "--output", "text"},
            keyName);
    if (isEmptyAWSResult(keyName)) {
        rows.push_back({LS_TYPE_KEYPAIR, config->getKeyPairName(), "",
                        LS_STATE_MISSING, ""});
    } else {
        std::string fp;
        cli.run({"ec2", "describe-key-pairs",
                 "--key-names", config->getKeyPairName(),
                 "--query", "KeyPairs[0].KeyFingerprint",
                 "--output", "text"},
                fp);
        rows.push_back({LS_TYPE_KEYPAIR, config->getKeyPairName(), "",
                        LS_STATE_PRESENT, "fp " + fp});
    }

    std::string instanceId;
    if (!isEmptyAWSResult(subnetId)) {
        cli.run({"ec2", "describe-instances",
                 "--filters",
                 std::string("Name=tag:Name,Values=")
                     + BUILD_INSTANCE_NAME_PREFIX + "*",
                 "Name=subnet-id,Values=" + subnetId,
                 std::string("Name=instance-state-name,")
                     + "Values=pending,running,stopped,stopping",
                 "--query", "Reservations[0].Instances[0].InstanceId",
                 "--output", "text"},
                instanceId);
    }
    if (isEmptyAWSResult(instanceId)) {
        rows.push_back({LS_TYPE_INSTANCE, "", "", LS_STATE_MISSING, ""});
    } else {
        std::string instanceName;
        cli.run({"ec2", "describe-instances",
                 "--instance-ids", instanceId,
                 "--query",
                 std::string("Reservations[0].Instances[0].Tags")
                     + "[?Key=='Name']|[0].Value",
                 "--output", "text"},
                instanceName);
        std::string state;
        cli.run({"ec2", "describe-instances",
                 "--instance-ids", instanceId,
                 "--query", "Reservations[0].Instances[0].State.Name",
                 "--output", "text"},
                state);
        std::string publicIP;
        cli.run({"ec2", "describe-instances",
                 "--instance-ids", instanceId,
                 "--query", "Reservations[0].Instances[0].PublicIpAddress",
                 "--output", "text"},
                publicIP);
        std::string instanceType;
        cli.run({"ec2", "describe-instances",
                 "--instance-ids", instanceId,
                 "--query", "Reservations[0].Instances[0].InstanceType",
                 "--output", "text"},
                instanceType);

        std::string details = instanceType;
        if (!isEmptyAWSResult(publicIP)) {
            details += " " + publicIP;
        }
        rows.push_back({LS_TYPE_INSTANCE, instanceName, instanceId,
                        state, details});
    }
}

void AWSEC2Flow::printLsTable(const std::vector<LsRow>& rows) {
    const std::vector<std::string> headers =
        {"TYPE", "NAME", "ID", "STATE", "DETAILS"};

    std::vector<size_t> widths(headers.size());
    for (size_t i = 0; i < headers.size(); i++) {
        widths[i] = headers[i].size();
    }
    for (const auto& row : rows) {
        widths[0] = std::max(widths[0], row.type.size());
        widths[1] = std::max(widths[1], row.name.size());
        widths[2] = std::max(widths[2], row.id.size());
        widths[3] = std::max(widths[3], row.state.size());
        widths[4] = std::max(widths[4], row.details.size());
    }

    const auto printRow = [&](const std::array<std::string, 5>& cells) {
        for (size_t i = 0; i < cells.size(); i++) {
            std::cout << cells[i];
            if (i + 1 < cells.size()) {
                std::cout << std::string(widths[i] - cells[i].size() + 2, ' ');
            }
        }
        std::cout << "\n";
    };

    printRow({headers[0], headers[1], headers[2], headers[3], headers[4]});

    std::array<std::string, 5> sep;
    for (size_t i = 0; i < sep.size(); i++) {
        sep[i] = std::string(widths[i], '-');
    }
    printRow(sep);

    for (const auto& row : rows) {
        printRow({row.type, row.name, row.id, row.state, row.details});
    }
}

void AWSEC2Flow::detectExistingInfra(AWSCLI& cli,
                                     const AWSEC2Config* config,
                                     std::vector<std::string>& found) {
    std::string vpcId;
    cli.run({"ec2", "describe-vpcs",
             "--filters", "Name=tag:Name,Values=" + config->getVPCName(),
             "--query", "Vpcs[0].VpcId",
             "--output", "text"},
            vpcId);
    const bool haveVPC = !isEmptyAWSResult(vpcId);
    if (haveVPC) {
        found.push_back(fmt::format("VPC '{}' ({})",
                                    config->getVPCName(), vpcId));
    }

    if (haveVPC) {
        std::string subnetId;
        cli.run({"ec2", "describe-subnets",
                 "--filters",
                 "Name=tag:Name,Values=" + config->getPublicSubnetName(),
                 "Name=vpc-id,Values=" + vpcId,
                 "--query", "Subnets[0].SubnetId",
                 "--output", "text"},
                subnetId);
        if (!isEmptyAWSResult(subnetId)) {
            found.push_back(fmt::format("public subnet '{}' ({})",
                                        config->getPublicSubnetName(),
                                        subnetId));
        }

        std::string igwId;
        cli.run({"ec2", "describe-internet-gateways",
                 "--filters", "Name=attachment.vpc-id,Values=" + vpcId,
                 "--query", "InternetGateways[0].InternetGatewayId",
                 "--output", "text"},
                igwId);
        if (!isEmptyAWSResult(igwId)) {
            found.push_back(fmt::format("internet gateway ({})", igwId));
        }

        std::string routeTableId;
        cli.run({"ec2", "describe-route-tables",
                 "--filters",
                 std::string("Name=tag:Name,Values=") + ROUTE_TABLE_NAME,
                 "Name=vpc-id,Values=" + vpcId,
                 "--query", "RouteTables[0].RouteTableId",
                 "--output", "text"},
                routeTableId);
        if (!isEmptyAWSResult(routeTableId)) {
            found.push_back(fmt::format("public route table '{}' ({})",
                                        ROUTE_TABLE_NAME, routeTableId));
        }

        std::string securityGroupId;
        cli.run({"ec2", "describe-security-groups",
                 "--filters",
                 std::string("Name=group-name,Values=") + SECURITY_GROUP_NAME,
                 "Name=vpc-id,Values=" + vpcId,
                 "--query", "SecurityGroups[0].GroupId",
                 "--output", "text"},
                securityGroupId);
        if (!isEmptyAWSResult(securityGroupId)) {
            found.push_back(fmt::format("security group '{}' ({})",
                                        SECURITY_GROUP_NAME,
                                        securityGroupId));
        }

        std::string instanceIds;
        cli.run({"ec2", "describe-instances",
                 "--filters",
                 std::string("Name=tag:Name,Values=")
                     + BUILD_INSTANCE_NAME_PREFIX + "*",
                 "Name=vpc-id,Values=" + vpcId,
                 std::string("Name=instance-state-name,")
                     + "Values=pending,running,stopped,stopping",
                 "--query", "Reservations[].Instances[].InstanceId",
                 "--output", "text"},
                instanceIds);
        if (!isEmptyAWSResult(instanceIds)) {
            found.push_back(fmt::format("build instance(s): {}",
                                        instanceIds));
        }
    }

    std::string keyName;
    cli.run({"ec2", "describe-key-pairs",
             "--filters", "Name=key-name,Values=" + config->getKeyPairName(),
             "--query", "KeyPairs[0].KeyName",
             "--output", "text"},
            keyName);
    if (!isEmptyAWSResult(keyName)) {
        found.push_back(fmt::format("key pair '{}'", keyName));
    }
}

void AWSEC2Flow::ensureVPC(AWSCLI& cli,
                           const AWSEC2Config* config,
                           std::string& vpcId) {
    const std::string& vpcName = config->getVPCName();
    spdlog::info("AWSEC2 infra init: looking up VPC '{}'", vpcName);

    cli.run({"ec2", "describe-vpcs",
             "--filters", "Name=tag:Name,Values=" + vpcName,
             "--query", "Vpcs[0].VpcId",
             "--output", "text"},
            vpcId);

    if (!isEmptyAWSResult(vpcId)) {
        spdlog::info("AWSEC2 infra init: found existing VPC {}", vpcId);
        return;
    }

    confirmCreate(fmt::format("VPC '{}' (CIDR {}) in region {}",
                              vpcName, DEFAULT_VPC_CIDR, cli.getRegion()));

    spdlog::info("AWSEC2 infra init: creating VPC '{}' with CIDR {}",
                 vpcName, DEFAULT_VPC_CIDR);

    cli.run({"ec2", "create-vpc",
             "--cidr-block", DEFAULT_VPC_CIDR,
             "--tag-specifications",
             "ResourceType=vpc,Tags=[{Key=Name,Value=" + vpcName + "}]",
             "--query", "Vpc.VpcId",
             "--output", "text"},
            vpcId);

    spdlog::info("AWSEC2 infra init: created VPC {}", vpcId);
}

void AWSEC2Flow::findFirstAvailabilityZone(AWSCLI& cli, std::string& azName) {
    cli.run({"ec2", "describe-availability-zones",
             "--query", "AvailabilityZones[0].ZoneName",
             "--output", "text"},
            azName);
    if (isEmptyAWSResult(azName)) {
        panic("No availability zone found in the region");
    }
}

void AWSEC2Flow::ensureSubnet(AWSCLI& cli,
                              const AWSEC2Config* config,
                              const std::string& vpcId,
                              std::string& subnetId) {
    const std::string& subnetName = config->getPublicSubnetName();
    spdlog::info("AWSEC2 infra init: looking up subnet '{}' in VPC {}",
                 subnetName, vpcId);

    cli.run({"ec2", "describe-subnets",
             "--filters",
             "Name=tag:Name,Values=" + subnetName,
             "Name=vpc-id,Values=" + vpcId,
             "--query", "Subnets[0].SubnetId",
             "--output", "text"},
            subnetId);

    if (!isEmptyAWSResult(subnetId)) {
        spdlog::info("AWSEC2 infra init: found existing subnet {}", subnetId);
        return;
    }

    std::string azName;
    findFirstAvailabilityZone(cli, azName);

    confirmCreate(fmt::format("Public subnet '{}' (CIDR {}) in AZ {} of VPC {}",
                              subnetName, DEFAULT_SUBNET_CIDR, azName, vpcId));

    spdlog::info("AWSEC2 infra init: creating subnet '{}' in AZ {} "
                 "with CIDR {}",
                 subnetName, azName, DEFAULT_SUBNET_CIDR);

    cli.run({"ec2", "create-subnet",
             "--vpc-id", vpcId,
             "--cidr-block", DEFAULT_SUBNET_CIDR,
             "--availability-zone", azName,
             "--tag-specifications",
             "ResourceType=subnet,Tags=[{Key=Name,Value=" + subnetName + "}]",
             "--query", "Subnet.SubnetId",
             "--output", "text"},
            subnetId);

    spdlog::info("AWSEC2 infra init: created subnet {}", subnetId);
}

void AWSEC2Flow::ensureInternetGateway(AWSCLI& cli,
                                       const std::string& vpcId,
                                       std::string& igwId) {
    spdlog::info("AWSEC2 infra init: looking up internet gateway for VPC {}",
                 vpcId);

    cli.run({"ec2", "describe-internet-gateways",
             "--filters", "Name=attachment.vpc-id,Values=" + vpcId,
             "--query", "InternetGateways[0].InternetGatewayId",
             "--output", "text"},
            igwId);

    if (!isEmptyAWSResult(igwId)) {
        spdlog::info("AWSEC2 infra init: found existing IGW {}", igwId);
        return;
    }

    confirmCreate(fmt::format("Internet gateway attached to VPC {}", vpcId));

    createAndAttachIGW(cli, vpcId, igwId);
}

void AWSEC2Flow::ensurePublicRouteTable(AWSCLI& cli,
                                        const std::string& vpcId,
                                        const std::string& igwId,
                                        const std::string& subnetId,
                                        std::string& routeTableId) {
    spdlog::info("AWSEC2 infra init: looking up public route table '{}' "
                 "in VPC {}",
                 ROUTE_TABLE_NAME, vpcId);

    cli.run({"ec2", "describe-route-tables",
             "--filters",
             std::string("Name=tag:Name,Values=") + ROUTE_TABLE_NAME,
             "Name=vpc-id,Values=" + vpcId,
             "--query", "RouteTables[0].RouteTableId",
             "--output", "text"},
            routeTableId);

    if (!isEmptyAWSResult(routeTableId)) {
        spdlog::info("AWSEC2 infra init: found existing route table {}",
                     routeTableId);
        return;
    }

    confirmCreate(fmt::format(
        "Public route table '{}' in VPC {} (default route via {}, "
        "associated with subnet {})",
        ROUTE_TABLE_NAME, vpcId, igwId, subnetId));

    spdlog::info("AWSEC2 infra init: creating public route table");

    cli.run({"ec2", "create-route-table",
             "--vpc-id", vpcId,
             "--tag-specifications",
             std::string("ResourceType=route-table,")
                 + "Tags=[{Key=Name,Value=" + ROUTE_TABLE_NAME + "}]",
             "--query", "RouteTable.RouteTableId",
             "--output", "text"},
            routeTableId);

    addDefaultRoute(cli, routeTableId, igwId);

    std::string assocOutput;
    cli.run({"ec2", "associate-route-table",
             "--route-table-id", routeTableId,
             "--subnet-id", subnetId},
            assocOutput);

    spdlog::info("AWSEC2 infra init: created route table {} associated "
                 "with subnet {}", routeTableId, subnetId);
}

void AWSEC2Flow::ensureSecurityGroup(AWSCLI& cli,
                                     const std::string& vpcId,
                                     std::string& securityGroupId) {
    spdlog::info("AWSEC2 infra init: looking up security group '{}' "
                 "in VPC {}",
                 SECURITY_GROUP_NAME, vpcId);

    cli.run({"ec2", "describe-security-groups",
             "--filters",
             std::string("Name=group-name,Values=") + SECURITY_GROUP_NAME,
             "Name=vpc-id,Values=" + vpcId,
             "--query", "SecurityGroups[0].GroupId",
             "--output", "text"},
            securityGroupId);

    if (!isEmptyAWSResult(securityGroupId)) {
        spdlog::info("AWSEC2 infra init: found existing security group {}",
                     securityGroupId);
        return;
    }

    confirmCreate(fmt::format(
        "Security group '{}' in VPC {} with SSH (port {}) ingress from 0.0.0.0/0",
        SECURITY_GROUP_NAME, vpcId, SSH_PORT));

    spdlog::info("AWSEC2 infra init: creating security group with SSH ingress");

    cli.run({"ec2", "create-security-group",
             "--group-name", SECURITY_GROUP_NAME,
             "--description", SECURITY_GROUP_DESCRIPTION,
             "--vpc-id", vpcId,
             "--query", "GroupId",
             "--output", "text"},
            securityGroupId);

    std::string ingressOutput;
    cli.run({"ec2", "authorize-security-group-ingress",
             "--group-id", securityGroupId,
             "--protocol", "tcp",
             "--port", std::to_string(SSH_PORT),
             "--cidr", "0.0.0.0/0"},
            ingressOutput);

    spdlog::info("AWSEC2 infra init: created security group {} "
                 "with SSH ingress",
                 securityGroupId);
}

void AWSEC2Flow::ensureKeyPair(AWSCLI& cli,
                               const AWSEC2Config* config,
                               const std::string& flowDir,
                               std::string& pemPath) {
    const std::string& keyName = config->getKeyPairName();
    pemPath = joinPath(flowDir, keyName + ".pem");

    spdlog::info("AWSEC2 infra init: looking up key pair '{}'", keyName);

    std::string awsKeyName;
    cli.run({"ec2", "describe-key-pairs",
             "--filters", "Name=key-name,Values=" + keyName,
             "--query", "KeyPairs[0].KeyName",
             "--output", "text"},
            awsKeyName);

    const bool awsHasKey = !isEmptyAWSResult(awsKeyName);
    const bool pemExists = FileUtils::exists(pemPath);

    if (awsHasKey && pemExists) {
        spdlog::info("AWSEC2 infra init: found existing key pair '{}' "
                     "with local pem at {}",
                     keyName, pemPath);
        return;
    }

    if (awsHasKey && !pemExists) {
        panic("Key pair '{}' exists in AWS but local pem {} is missing. "
              "AWS cannot return private material for an existing key pair. "
              "Either restore the pem file, or delete the AWS key pair "
              "(aws ec2 delete-key-pair --key-name {}) and rerun.",
              keyName, pemPath, keyName);
    }

    if (!awsHasKey && pemExists) {
        panic("Local pem {} exists but key pair '{}' is missing in AWS. "
              "Remove the stale pem file and rerun to recreate the key pair.",
              pemPath, keyName);
    }

    confirmCreate(fmt::format(
        "EC2 key pair '{}' in region {} (private key will be written to {})",
        keyName, cli.getRegion(), pemPath));

    spdlog::info("AWSEC2 infra init: creating key pair '{}'", keyName);

    std::string keyMaterial;
    cli.run({"ec2", "create-key-pair",
             "--key-name", keyName,
             "--query", "KeyMaterial",
             "--output", "text"},
            keyMaterial);

    if (keyMaterial.empty()) {
        panic("AWS returned empty key material for key pair '{}'", keyName);
    }

    std::ofstream pemOut(pemPath);
    if (!pemOut.is_open()) {
        panic("Failed to open pem file for writing: {}", pemPath);
    }
    pemOut << keyMaterial;
    if (!keyMaterial.empty() && keyMaterial.back() != '\n') {
        pemOut << "\n";
    }
    pemOut.close();

    if (chmod(pemPath.c_str(), S_IRUSR | S_IWUSR) != 0) {
        panic("Failed to chmod 600 pem file: {}", pemPath);
    }

    spdlog::info("AWSEC2 infra init: wrote key pair private key to {}",
                 pemPath);
}

void AWSEC2Flow::resolveAMI(AWSCLI& cli,
                            const AWSEC2Config* config,
                            std::string& amiId) {
    if (!config->getAMIID().empty()) {
        amiId = config->getAMIID();
        spdlog::info("AWSEC2 infra init: using configured AMI {}", amiId);
        return;
    }

    spdlog::info("AWSEC2 infra init: resolving latest Vivado AMI from Xilinx "
                 "in region {}",
                 config->getRegion());

    cli.run({"ec2", "describe-images",
             "--owners", XILINX_AMI_OWNER,
             "--filters",
             std::string("Name=name,Values=") + XILINX_AMI_NAME_FILTER,
             "--query", "sort_by(Images, &CreationDate)[-1].ImageId",
             "--output", "text"},
            amiId);

    if (isEmptyAWSResult(amiId)) {
        panic("Could not resolve a Vivado AMI from Xilinx in region {}",
              config->getRegion());
    }

    spdlog::info("AWSEC2 infra init: resolved AMI {}", amiId);
}

void AWSEC2Flow::ensureBuildInstance(AWSCLI& cli,
                                     const AWSEC2Config* config,
                                     const std::string& subnetId,
                                     const std::string& securityGroupId,
                                     const std::string& amiId,
                                     std::string& instanceId,
                                     std::string& instanceName,
                                     std::string& publicIP) {
    spdlog::info("AWSEC2 infra init: looking up existing build instance "
                 "in subnet {}", subnetId);

    cli.run({"ec2", "describe-instances",
             "--filters",
             std::string("Name=tag:Name,Values=")
                 + BUILD_INSTANCE_NAME_PREFIX + "*",
             "Name=subnet-id,Values=" + subnetId,
             std::string("Name=instance-state-name,")
                 + "Values=pending,running,stopped,stopping",
             "--query", "Reservations[0].Instances[0].InstanceId",
             "--output", "text"},
            instanceId);

    if (!isEmptyAWSResult(instanceId)) {
        cli.run({"ec2", "describe-instances",
                 "--instance-ids", instanceId,
                 "--query",
                 std::string("Reservations[0].Instances[0].Tags")
                     + "[?Key=='Name']|[0].Value",
                 "--output", "text"},
                instanceName);

        std::string state;
        cli.run({"ec2", "describe-instances",
                 "--instance-ids", instanceId,
                 "--query", "Reservations[0].Instances[0].State.Name",
                 "--output", "text"},
                state);

        if (state == "stopped" || state == "stopping") {
            spdlog::info("AWSEC2 infra init: starting stopped instance {}",
                         instanceId);
            std::string startOut;
            cli.run({"ec2", "start-instances",
                     "--instance-ids", instanceId},
                    startOut);
        }

        spdlog::info("AWSEC2 infra init: waiting for instance {} to reach "
                     "running state", instanceId);
        std::string waitOut;
        cli.run({"ec2", "wait", "instance-running",
                 "--instance-ids", instanceId},
                waitOut);

        cli.run({"ec2", "describe-instances",
                 "--instance-ids", instanceId,
                 "--query",
                 "Reservations[0].Instances[0].PublicIpAddress",
                 "--output", "text"},
                publicIP);

        if (isEmptyAWSResult(publicIP)) {
            panic("Existing build instance {} has no public IP address",
                  instanceId);
        }

        spdlog::info("AWSEC2 infra init: reusing build instance {} "
                     "(name={}, public_ip={})",
                     instanceId, instanceName, publicIP);
        return;
    }

    instanceName = std::string(BUILD_INSTANCE_NAME_PREFIX) + generateInstanceId();

    confirmCreate(fmt::format(
        "Build instance '{}' ({}, AMI {}) in subnet {} with SG {}",
        instanceName, config->getBuildInstanceType(), amiId,
        subnetId, securityGroupId));

    spdlog::info("AWSEC2 infra init: launching build instance '{}'",
                 instanceName);

    const std::string networkInterfaces = fmt::format(
        "AssociatePublicIpAddress=true,DeviceIndex=0,SubnetId={},Groups={}",
        subnetId, securityGroupId);

    cli.run({"ec2", "run-instances",
             "--image-id", amiId,
             "--instance-type", config->getBuildInstanceType(),
             "--key-name", config->getKeyPairName(),
             "--network-interfaces", networkInterfaces,
             "--tag-specifications",
             std::string("ResourceType=instance,")
                 + "Tags=[{Key=Name,Value=" + instanceName + "}]",
             "--query", "Instances[0].InstanceId",
             "--output", "text"},
            instanceId);

    if (isEmptyAWSResult(instanceId)) {
        panic("AWS did not return an instance id after run-instances");
    }

    spdlog::info("AWSEC2 infra init: launched instance {}, waiting for "
                 "running state", instanceId);

    std::string waitOut;
    cli.run({"ec2", "wait", "instance-running",
             "--instance-ids", instanceId},
            waitOut);

    cli.run({"ec2", "describe-instances",
             "--instance-ids", instanceId,
             "--query", "Reservations[0].Instances[0].PublicIpAddress",
             "--output", "text"},
            publicIP);

    if (isEmptyAWSResult(publicIP)) {
        panic("Instance {} is running but has no public IP address",
              instanceId);
    }

    spdlog::info("AWSEC2 infra init: build instance {} is running "
                 "(name={}, public_ip={})",
                 instanceId, instanceName, publicIP);
}

void AWSEC2Flow::writeAWSInfra(const AWSEC2Config* config,
                               const std::string& path,
                               const std::string& vpcId,
                               const std::string& subnetId,
                               const std::string& securityGroupId,
                               const std::string& amiId,
                               const std::string& pemPath,
                               const std::string& buildInstanceName,
                               const std::string& buildInstanceId,
                               const std::string& buildInstancePublicIP) {
    std::ofstream out(path);
    if (!out.is_open()) {
        panic("Failed to open aws infra file for writing: {}", path);
    }

    out << "region = \"" << config->getRegion() << "\"\n";
    out << "profile = \"" << config->getProfile() << "\"\n";
    out << "autostop = " << (config->getAutostop() ? "true" : "false") << "\n";
    out << "ami_id = \"" << amiId << "\"\n";
    out << "vpc_name = \"" << config->getVPCName() << "\"\n";
    out << "vpc_id = \"" << vpcId << "\"\n";
    out << "subnet_name = \"" << config->getPublicSubnetName() << "\"\n";
    out << "subnet_id = \"" << subnetId << "\"\n";
    out << "security_group_id = \"" << securityGroupId << "\"\n";
    out << "key_pair_name = \"" << config->getKeyPairName() << "\"\n";
    out << "ssh_user = \"" << config->getSSHUser() << "\"\n";
    out << "pem_path = \"" << pemPath << "\"\n";

    out << "\n[build_instance]\n";
    out << "name = \"" << buildInstanceName << "\"\n";
    out << "id = \"" << buildInstanceId << "\"\n";
    out << "public_ip = \"" << buildInstancePublicIP << "\"\n";
    out << "instance_type = \"" << config->getBuildInstanceType() << "\"\n";

    out << "\n[fpga_instance]\n";
    out << "instance_type = \"" << config->getFPGAInstanceType() << "\"\n";
}

int AWSEC2Flow::runCommand(const std::string& commandScriptPath) {
    spdlog::info("AWSEC2 flow: running command script {}", commandScriptPath);

    Command command;
    command.setName(BASH_BINARY);
    command.addArg(commandScriptPath);

    CommandExecutor executor;
    return executor.exec(&command);
}

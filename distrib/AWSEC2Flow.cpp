#include "AWSEC2Flow.h"

#include <stdint.h>
#include <unistd.h>

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

    if (answer != "y" && answer != "Y" && answer != "yes" && answer != "YES") {
        panic("Aborted by user: {}", resourceDescription);
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
    spdlog::info("AWSEC2 infra init: dry run complete, "
                 "skipping aws_infra.toml write");
}

void AWSEC2Flow::provision(const AWSEC2Config* config) {
    AWSCLI cli;
    cli.setRegion(config->getRegion());

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

    std::string amiId;
    resolveAMI(cli, config, amiId);

    const std::string buildInstanceName =
        std::string(BUILD_INSTANCE_NAME_PREFIX) + generateInstanceId();
    spdlog::info("AWSEC2 infra init: build instance name = {}",
                 buildInstanceName);

    DistribFlowManager* manager = getManager();
    const std::string& distribDir = manager->getDistribDir();
    if (distribDir.empty()) {
        panic("AWSEC2Flow::init requires the distrib output directory to be set");
    }

    const std::string flowDir = joinPath(distribDir, AWSEC2_SUBDIR_NAME);
    if (!FileUtils::exists(flowDir)) {
        FileUtils::createDirectory(flowDir);
    }

    const std::string awsInfraPath = joinPath(flowDir, AWS_INFRA_FILE_NAME);
    writeAWSInfra(config,
                  awsInfraPath,
                  vpcId,
                  subnetId,
                  securityGroupId,
                  amiId,
                  buildInstanceName);

    spdlog::info("AWSEC2 infra init: wrote {}", awsInfraPath);
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

    spdlog::info("AWSEC2 infra init: creating internet gateway");

    cli.run({"ec2", "create-internet-gateway",
             "--tag-specifications",
             std::string("ResourceType=internet-gateway,")
                 + "Tags=[{Key=Name,Value=" + IGW_NAME + "}]",
             "--query", "InternetGateway.InternetGatewayId",
             "--output", "text"},
            igwId);

    std::string attachOutput;
    cli.run({"ec2", "attach-internet-gateway",
             "--internet-gateway-id", igwId,
             "--vpc-id", vpcId},
            attachOutput);

    spdlog::info("AWSEC2 infra init: created and attached IGW {}", igwId);
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

    std::string routeOutput;
    cli.run({"ec2", "create-route",
             "--route-table-id", routeTableId,
             "--destination-cidr-block", "0.0.0.0/0",
             "--gateway-id", igwId},
            routeOutput);

    std::string assocOutput;
    cli.run({"ec2", "associate-route-table",
             "--route-table-id", routeTableId,
             "--subnet-id", subnetId},
            assocOutput);

    spdlog::info("AWSEC2 infra init: created route table {} with default "
                 "route via {} and associated with subnet {}",
                 routeTableId, igwId, subnetId);
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

void AWSEC2Flow::writeAWSInfra(const AWSEC2Config* config,
                               const std::string& path,
                               const std::string& vpcId,
                               const std::string& subnetId,
                               const std::string& securityGroupId,
                               const std::string& amiId,
                               const std::string& buildInstanceName) {
    std::ofstream out(path);
    if (!out.is_open()) {
        panic("Failed to open aws infra file for writing: {}", path);
    }

    out << "region = \"" << config->getRegion() << "\"\n";
    out << "autostop = " << (config->getAutostop() ? "true" : "false") << "\n";
    out << "ami_id = \"" << amiId << "\"\n";
    out << "vpc_name = \"" << config->getVPCName() << "\"\n";
    out << "vpc_id = \"" << vpcId << "\"\n";
    out << "subnet_name = \"" << config->getPublicSubnetName() << "\"\n";
    out << "subnet_id = \"" << subnetId << "\"\n";
    out << "security_group_id = \"" << securityGroupId << "\"\n";

    out << "\n[build_instance]\n";
    out << "name = \"" << buildInstanceName << "\"\n";
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

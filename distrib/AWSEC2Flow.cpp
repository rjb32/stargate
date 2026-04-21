#include "AWSEC2Flow.h"

#include <stdint.h>

#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>

#include <spdlog/spdlog.h>

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

    spdlog::info("AWSEC2 infra init: validating distrib config");
    spdlog::info("  region              = {}", awsec2Config->getRegion());
    spdlog::info("  vpc                 = {}", awsec2Config->getVPCName());
    spdlog::info("  public_subnet       = {}", awsec2Config->getPublicSubnetName());
    spdlog::info("  build_instance_type = {}", awsec2Config->getBuildInstanceType());
    spdlog::info("  fpga_instance_type  = {}", awsec2Config->getFPGAInstanceType());
    spdlog::info("  autostop            = {}", awsec2Config->getAutostop());

    const std::string vpcId = ensureVPC(awsec2Config, dryMode);
    const std::string subnetId = ensureSubnet(awsec2Config, vpcId, dryMode);
    const std::string securityGroupId =
        ensureSecurityGroup(awsec2Config, vpcId, dryMode);
    const std::string amiId = resolveAMI(awsec2Config, dryMode);

    const std::string buildInstanceName =
        std::string(BUILD_INSTANCE_NAME_PREFIX) + generateInstanceId();

    spdlog::info("AWSEC2 infra init: build instance name = {}", buildInstanceName);

    if (dryMode) {
        spdlog::info("AWSEC2 infra init: dry run complete, "
                     "skipping aws_infra.toml write");
        return;
    }

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
    writeAWSInfra(awsec2Config,
                  awsInfraPath,
                  vpcId,
                  subnetId,
                  securityGroupId,
                  amiId,
                  buildInstanceName);

    spdlog::info("AWSEC2 infra init: wrote {}", awsInfraPath);
}

std::string AWSEC2Flow::ensureVPC(const AWSEC2Config* config, bool dryMode) {
    spdlog::info("AWSEC2 infra init: {}ensuring VPC '{}' exists in region {}",
                 dryMode ? "[dry] " : "",
                 config->getVPCName(), config->getRegion());
    return "vpc-pending";
}

std::string AWSEC2Flow::ensureSubnet(const AWSEC2Config* config,
                                     const std::string& vpcId,
                                     bool dryMode) {
    spdlog::info("AWSEC2 infra init: {}ensuring public subnet '{}' in VPC {}",
                 dryMode ? "[dry] " : "",
                 config->getPublicSubnetName(), vpcId);
    return "subnet-pending";
}

std::string AWSEC2Flow::ensureSecurityGroup(const AWSEC2Config* config,
                                            const std::string& vpcId,
                                            bool dryMode) {
    spdlog::info("AWSEC2 infra init: {}ensuring security group in VPC {} "
                 "(SSH ingress only)",
                 dryMode ? "[dry] " : "",
                 vpcId);
    (void)config;
    return "sg-pending";
}

std::string AWSEC2Flow::resolveAMI(const AWSEC2Config* config, bool dryMode) {
    if (!config->getAMIID().empty()) {
        spdlog::info("AWSEC2 infra init: {}using configured AMI {}",
                     dryMode ? "[dry] " : "",
                     config->getAMIID());
        return config->getAMIID();
    }
    spdlog::info("AWSEC2 infra init: {}resolving latest Vivado AMI from Xilinx "
                 "in region {}",
                 dryMode ? "[dry] " : "",
                 config->getRegion());
    return "ami-pending-xilinx-vivado";
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

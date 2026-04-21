#include "AWSEC2Config.h"

#include <ostream>

#include <toml++/toml.hpp>

#include "Panic.h"

using namespace stargate;

namespace {

constexpr const char* REGION_KEY = "region";
constexpr const char* VPC_KEY = "vpc";
constexpr const char* PUBLIC_SUBNET_KEY = "public_subnet";
constexpr const char* BUILD_INSTANCE_TYPE_KEY = "build_instance_type";
constexpr const char* FPGA_INSTANCE_TYPE_KEY = "fpga_instance_type";
constexpr const char* AMI_ID_KEY = "ami_id";
constexpr const char* AUTOSTOP_KEY = "autostop";

constexpr const char* DEFAULT_REGION = "us-west-2";
constexpr const char* DEFAULT_VPC_NAME = "stargate-vpc";
constexpr const char* DEFAULT_PUBLIC_SUBNET_NAME = "stargate-public";
constexpr const char* DEFAULT_BUILD_INSTANCE_TYPE = "z1d.2xlarge";
constexpr const char* DEFAULT_FPGA_INSTANCE_TYPE = "f1.2xlarge";

constexpr const char* SECTION_NAME = "awsec2";

}

AWSEC2Config::AWSEC2Config()
    : _region(DEFAULT_REGION),
    _vpcName(DEFAULT_VPC_NAME),
    _publicSubnetName(DEFAULT_PUBLIC_SUBNET_NAME),
    _buildInstanceType(DEFAULT_BUILD_INSTANCE_TYPE),
    _fpgaInstanceType(DEFAULT_FPGA_INSTANCE_TYPE)
{
}

AWSEC2Config::~AWSEC2Config() {
}

void AWSEC2Config::loadFromTable(const toml::table& table) {
    for (const auto& [key, value] : table) {
        if (key == REGION_KEY) {
            if (const auto& str = value.value<std::string>()) {
                _region = *str;
            }
        } else if (key == VPC_KEY) {
            if (const auto& str = value.value<std::string>()) {
                _vpcName = *str;
            }
        } else if (key == PUBLIC_SUBNET_KEY) {
            if (const auto& str = value.value<std::string>()) {
                _publicSubnetName = *str;
            }
        } else if (key == BUILD_INSTANCE_TYPE_KEY) {
            if (const auto& str = value.value<std::string>()) {
                _buildInstanceType = *str;
            }
        } else if (key == FPGA_INSTANCE_TYPE_KEY) {
            if (const auto& str = value.value<std::string>()) {
                _fpgaInstanceType = *str;
            }
        } else if (key == AMI_ID_KEY) {
            if (const auto& str = value.value<std::string>()) {
                _amiID = *str;
            }
        } else if (key == AUTOSTOP_KEY) {
            if (const auto& b = value.value<bool>()) {
                _autostop = *b;
            }
        } else {
            panic("Unknown key '{}' in awsec2 distrib section", key.str());
        }
    }
}

void AWSEC2Config::save(std::ostream& out) const {
    out << "[" << SECTION_NAME << "]\n";
    out << REGION_KEY << " = \"" << _region << "\"\n";
    out << VPC_KEY << " = \"" << _vpcName << "\"\n";
    out << PUBLIC_SUBNET_KEY << " = \"" << _publicSubnetName << "\"\n";
    out << BUILD_INSTANCE_TYPE_KEY << " = \"" << _buildInstanceType << "\"\n";
    out << FPGA_INSTANCE_TYPE_KEY << " = \"" << _fpgaInstanceType << "\"\n";
    out << AMI_ID_KEY << " = \"" << _amiID << "\"\n";
    out << AUTOSTOP_KEY << " = " << (_autostop ? "true" : "false") << "\n";
}

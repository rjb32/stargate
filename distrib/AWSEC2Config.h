#pragma once

#include <iosfwd>
#include <string>

namespace toml {
inline namespace v3 {
class table;
}
}

namespace stargate {

class AWSEC2Config {
public:
    AWSEC2Config();
    ~AWSEC2Config();

    void loadFromTable(const toml::table& table);
    void save(std::ostream& out) const;

    const std::string& getRegion() const { return _region; }
    const std::string& getProfile() const { return _profile; }
    const std::string& getKeyPairName() const { return _keyPairName; }
    const std::string& getSSHUser() const { return _sshUser; }
    const std::string& getVPCName() const { return _vpcName; }
    const std::string& getPublicSubnetName() const { return _publicSubnetName; }
    const std::string& getBuildInstanceType() const { return _buildInstanceType; }
    const std::string& getFPGAInstanceType() const { return _fpgaInstanceType; }
    const std::string& getAMIID() const { return _amiID; }
    bool getAutostop() const { return _autostop; }

private:
    std::string _region;
    std::string _profile;
    std::string _keyPairName;
    std::string _sshUser;
    std::string _vpcName;
    std::string _publicSubnetName;
    std::string _buildInstanceType;
    std::string _fpgaInstanceType;
    std::string _amiID;
    bool _autostop {true};
};

}

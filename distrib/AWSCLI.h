#pragma once

#include <string>
#include <vector>

namespace stargate {

class AWSCLI {
public:
    using Args = std::vector<std::string>;

    AWSCLI();
    ~AWSCLI();

    void setRegion(const std::string& region) { _region = region; }
    const std::string& getRegion() const { return _region; }

    void setProfile(const std::string& profile) { _profile = profile; }
    const std::string& getProfile() const { return _profile; }

    void run(const Args& args, std::string& output) const;

private:
    std::string _region;
    std::string _profile;
};

}

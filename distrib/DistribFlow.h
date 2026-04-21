#pragma once

#include <string>
#include <string_view>

namespace stargate {

class DistribFlowManager;
class DistribConfig;

class DistribFlow {
public:
    friend DistribFlowManager;

    virtual ~DistribFlow();

    virtual std::string_view getName() const = 0;

    virtual void init(const DistribConfig* config, bool dryMode) = 0;

    virtual int runCommand(const std::string& commandScriptPath) = 0;

    DistribFlowManager* getManager() const { return _manager; }

protected:
    DistribFlow();

private:
    DistribFlowManager* _manager {nullptr};
};

}

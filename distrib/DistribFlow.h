#pragma once

#include <string>
#include <string_view>

namespace stargate {

class DistribFlowManager;
class DistribConfig;

enum class GUIAction {
    OPEN,
    START,
    STOP,
};

void setInfraYesMode(bool yes);
bool isInfraYesMode();

class DistribFlow {
public:
    friend DistribFlowManager;

    virtual ~DistribFlow();

    virtual std::string_view getName() const = 0;

    virtual void init(const DistribConfig* config, bool dryMode) = 0;
    virtual void ls(const DistribConfig* config) = 0;
    virtual void start(const DistribConfig* config) = 0;
    virtual void stop(const DistribConfig* config) = 0;
    virtual void destroy(const DistribConfig* config) = 0;
    virtual void gui(const DistribConfig* config, GUIAction action) = 0;

    virtual int runCommand(const std::string& commandScriptPath) = 0;

    DistribFlowManager* getManager() const { return _manager; }

protected:
    DistribFlow();

private:
    DistribFlowManager* _manager {nullptr};
};

}

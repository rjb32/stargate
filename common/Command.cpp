#include "Command.h"

using namespace stargate;

Command::Command() {
}

Command::~Command() {
}

void Command::addEnvVar(const std::string& name, const std::string& value) {
    _envVars.push_back({name, value});
}

void Command::addExecPath(const std::string& path) {
    _pathEntries.push_back(path);
}

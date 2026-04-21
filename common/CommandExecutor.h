#pragma once

#include <string>

namespace stargate {

class Command;

class CommandExecutor {
public:
    CommandExecutor();
    ~CommandExecutor();

    void writeScript(const Command* command, const std::string& path);
    int exec(const Command* command);
};

}

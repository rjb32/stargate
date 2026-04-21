#include "CommandExecutor.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>

#include "Command.h"
#include "Panic.h"

using namespace stargate;

namespace {

constexpr int EXEC_FAILED_EXIT_CODE = 127;
constexpr int SIGNAL_EXIT_OFFSET = 128;
constexpr int UNKNOWN_EXIT_CODE = -1;
constexpr size_t READ_BUFFER_SIZE = 4096;

std::string shellQuote(const std::string& arg) {
    std::string result = "'";
    for (char c : arg) {
        if (c == '\'') {
            result += "'\\''";
        } else {
            result += c;
        }
    }
    result += "'";
    return result;
}

void applyEnv(const Command* command) {
    for (const auto& env : command->envVars()) {
        setenv(env.first.c_str(), env.second.c_str(), 1);
    }

    if (command->pathEntries().empty()) {
        return;
    }

    std::string newPath;
    for (const auto& entry : command->pathEntries()) {
        if (!newPath.empty()) {
            newPath += ":";
        }
        newPath += entry;
    }

    const char* existing = getenv("PATH");
    if (existing != nullptr && *existing != '\0') {
        newPath += ":";
        newPath += existing;
    }
    setenv("PATH", newPath.c_str(), 1);
}

}

CommandExecutor::CommandExecutor() {
}

CommandExecutor::~CommandExecutor() {
}

void CommandExecutor::writeScript(const Command* command,
                                  const std::string& path) {
    std::ofstream script(path);
    if (!script.is_open()) {
        panic("Failed to open script file: {}", path);
    }

    script << "#!/bin/bash\n";
    script << "set -e\n\n";

    for (const auto& env : command->envVars()) {
        script << "export " << env.first << "="
               << shellQuote(env.second) << "\n";
    }

    if (!command->pathEntries().empty()) {
        script << "export PATH=";
        for (const auto& entry : command->pathEntries()) {
            script << shellQuote(entry) << ":";
        }
        script << "\"$PATH\"\n";
    }

    script << "\n" << shellQuote(command->getName());
    for (const auto& arg : command->args()) {
        script << " " << shellQuote(arg);
    }
    script << "\n";

    script.close();

    namespace fs = std::filesystem;
    fs::permissions(path,
                    fs::perms::owner_exec |
                    fs::perms::group_exec |
                    fs::perms::others_exec,
                    fs::perm_options::add);
}

int CommandExecutor::exec(const Command* command) {
    if (!command->getScriptPath().empty()) {
        writeScript(command, command->getScriptPath());
    }

    int pipeFd[2] = {-1, -1};
    if (pipe(pipeFd) < 0) {
        panic("Failed to create pipe: {}", strerror(errno));
    }

    const pid_t pid = fork();
    if (pid < 0) {
        panic("Failed to fork: {}", strerror(errno));
    }

    if (pid == 0) {
        close(pipeFd[0]);
        dup2(pipeFd[1], STDOUT_FILENO);
        dup2(pipeFd[1], STDERR_FILENO);
        close(pipeFd[1]);

        applyEnv(command);

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(command->getName().c_str()));
        for (const auto& arg : command->args()) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execvp(command->getName().c_str(), argv.data());

        const char* msg = "Failed to exec command\n";
        write(STDERR_FILENO, msg, strlen(msg));
        _exit(EXEC_FAILED_EXIT_CODE);
    }

    close(pipeFd[1]);

    std::ofstream logFile;
    if (!command->getLogPath().empty()) {
        logFile.open(command->getLogPath());
        if (!logFile.is_open()) {
            panic("Failed to open log file: {}", command->getLogPath());
        }
    }

    char buffer[READ_BUFFER_SIZE];
    while (true) {
        const ssize_t n = read(pipeFd[0], buffer, sizeof(buffer));
        if (n <= 0) {
            break;
        }

        ssize_t written = 0;
        while (written < n) {
            const ssize_t w = write(STDOUT_FILENO,
                                    buffer + written,
                                    n - written);
            if (w <= 0) {
                break;
            }
            written += w;
        }

        if (logFile.is_open()) {
            logFile.write(buffer, n);
        }
    }
    close(pipeFd[0]);

    if (logFile.is_open()) {
        logFile.close();
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        panic("waitpid failed: {}", strerror(errno));
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return SIGNAL_EXIT_OFFSET + WTERMSIG(status);
    }
    return UNKNOWN_EXIT_CODE;
}

#include "AWSCLI.h"

#include <stdio.h>
#include <sys/wait.h>

#include <spdlog/spdlog.h>

#include "Panic.h"

using namespace stargate;

namespace {

constexpr size_t READ_BUFFER_SIZE = 4096;
constexpr const char* AWS_BINARY = "aws";

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

void trimTrailingNewlines(std::string& s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.pop_back();
    }
}

}

AWSCLI::AWSCLI() {
}

AWSCLI::~AWSCLI() {
}

void AWSCLI::run(const Args& args, std::string& output) const {
    output.clear();

    std::string cmd = AWS_BINARY;
    if (!_region.empty()) {
        cmd += " --region ";
        cmd += shellQuote(_region);
    }
    for (const auto& arg : args) {
        cmd += " ";
        cmd += shellQuote(arg);
    }
    cmd += " 2>&1";

    spdlog::debug("AWSCLI: {}", cmd);

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        panic("Failed to invoke aws CLI: {}", cmd);
    }

    char buffer[READ_BUFFER_SIZE];
    while (true) {
        const size_t n = fread(buffer, 1, sizeof(buffer), pipe);
        if (n == 0) {
            break;
        }
        output.append(buffer, n);
    }

    const int status = pclose(pipe);
    if (status == -1) {
        panic("pclose failed for aws CLI command");
    }

    int exitCode = -1;
    if (WIFEXITED(status)) {
        exitCode = WEXITSTATUS(status);
    }

    if (exitCode != 0) {
        panic("aws CLI command failed (exit {}): {}\nOutput:\n{}",
              exitCode, cmd, output);
    }

    trimTrailingNewlines(output);
}

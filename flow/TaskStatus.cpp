#include "TaskStatus.h"

#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

#include <spdlog/spdlog.h>

#include "FileUtils.h"
#include "Panic.h"

using namespace stargate;

namespace {

const std::string STATUS_NOT_STARTED = "not_started";
const std::string STATUS_IN_PROGRESS = "in_progress";
const std::string STATUS_SUCCESS = "success";
const std::string STATUS_FAILED = "failed";

std::string getCurrentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto timeT = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&timeT), "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

// Simple JSON parsing helpers (avoiding external dependency)
std::string extractJsonString(const std::string& json, const std::string& key) {
    const std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) {
        return "";
    }

    pos = json.find(':', pos);
    if (pos == std::string::npos) {
        return "";
    }

    pos = json.find('"', pos);
    if (pos == std::string::npos) {
        return "";
    }

    size_t start = pos + 1;
    size_t end = json.find('"', start);
    if (end == std::string::npos) {
        return "";
    }

    return json.substr(start, end - start);
}

}

const std::string& TaskStatus::toString(Status status) {
    switch (status) {
        case Status::NotStarted:
            return STATUS_NOT_STARTED;

        case Status::InProgress:
            return STATUS_IN_PROGRESS;

        case Status::Success:
            return STATUS_SUCCESS;

        case Status::Failed:
            return STATUS_FAILED;
    }

    return STATUS_NOT_STARTED;
}

TaskStatus::Status TaskStatus::fromString(const std::string& str) {
    if (str == STATUS_SUCCESS) {
        return Status::Success;
    } else if (str == STATUS_FAILED) {
        return Status::Failed;
    } else if (str == STATUS_IN_PROGRESS) {
        return Status::InProgress;
    }

    return Status::NotStarted;
}

TaskStatus::Status TaskStatus::read(const std::string& statusFilePath) {
    if (!FileUtils::exists(statusFilePath)) {
        return Status::NotStarted;
    }

    std::ifstream file(statusFilePath);
    if (!file) {
        return Status::NotStarted;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string json = buffer.str();

    const std::string statusStr = extractJsonString(json, "status");
    return fromString(statusStr);
}

void TaskStatus::write(const std::string& statusFilePath,
                       Status status,
                       int exitCode,
                       const std::string& errorMessage) {
    std::ofstream file(statusFilePath);
    if (!file) {
        panic("Failed to write status file: {}", statusFilePath);
    }

    const std::string timestamp = getCurrentTimestamp();

    file << "{\n";
    file << "    \"status\": \"" << toString(status) << "\",\n";
    file << "    \"timestamp\": \"" << timestamp << "\",\n";
    file << "    \"exit_code\": " << exitCode << ",\n";
    file << "    \"error\": \"" << errorMessage << "\"\n";
    file << "}\n";
}

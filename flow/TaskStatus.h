#pragma once

#include <string>

namespace stargate {

enum class TaskStatus {
    NotStarted,
    InProgress,
    Success,
    Failed,
};

const std::string& taskStatusToString(TaskStatus status);
TaskStatus taskStatusFromString(const std::string& str);

// JSON format: { "status": "success", "timestamp": "...", "duration_ms": 123,
//                "exit_code": 0, "error": "" }
TaskStatus readTaskStatus(const std::string& statusFilePath);

void writeTaskStatus(const std::string& statusFilePath,
                     TaskStatus status,
                     int exitCode,
                     const std::string& errorMessage);

}

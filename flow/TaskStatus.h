#pragma once

#include <string>

namespace stargate {

class TaskStatus {
public:
    enum class Status {
        NotStarted,
        InProgress,
        Success,
        Failed,
    };

    static const std::string& toString(Status status);
    static Status fromString(const std::string& str);

    // JSON format: { "status": "success", "timestamp": "...", "duration_ms": 123,
    //                "exit_code": 0, "error": "" }
    static Status read(const std::string& statusFilePath);

    static void write(const std::string& statusFilePath,
                      Status status,
                      int exitCode,
                      const std::string& errorMessage);
};

}

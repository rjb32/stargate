#pragma once

#include <string>
#include <string_view>

#include "TaskStatus.h"

namespace stargate {

class FlowSection;

class FlowTask {
public:
    friend FlowSection;

    virtual ~FlowTask();
    virtual std::string_view getName() const = 0;
    virtual void execute() = 0;

    FlowSection* getParent() const { return _parent; }
    TaskStatus::Status getStatus() const;

    void getOutputDir(std::string& result) const;
    void getStatusFilePath(std::string& result) const;

protected:
    explicit FlowTask(FlowSection* parent);
    void registerTask();
    void writeStatus(TaskStatus::Status status,
                     int exitCode,
                     const std::string& errorMessage);

private:
    FlowSection* _parent {nullptr};
};

}

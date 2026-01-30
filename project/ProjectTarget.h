#pragma once

#include <string>
#include <vector>

namespace stargate {

class FileSet;
class ProjectConfig;

class ProjectTarget {
public:
    friend ProjectConfig;
    using FileSets = std::vector<const FileSet*>;

    static ProjectTarget* create(ProjectConfig* config, const std::string& name);

    const std::string& getName() const { return _name; }

    const FileSets& filesets() const { return _filesets; }

    void addFileSet(const FileSet* fileset);

    const std::string& getFlowName() const { return _flowName; }
    void setFlowName(const std::string& flowName);

private:
    std::string _name;
    std::string _flowName;
    FileSets _filesets;

    explicit ProjectTarget(const std::string& name);
    ~ProjectTarget();
};

}

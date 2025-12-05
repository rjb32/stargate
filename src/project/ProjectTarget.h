#pragma once

#include <string>
#include <vector>

namespace stargate {

class FileSet;
class ProjectConfig;

class ProjectTarget {
public:
    friend ProjectConfig;

    static ProjectTarget* create(ProjectConfig* config, const std::string& name);

    const std::string& getName() const { return _name; }

    void addFileSet(FileSet* fileset);

private:
    std::string _name;
    std::vector<FileSet*> _filesets;

    explicit ProjectTarget(const std::string& name);
    ~ProjectTarget();
};

}

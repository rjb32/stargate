#pragma once

#include <string>
#include <vector>

namespace stargate {

class FileSet;
class ProjectConfig;

class ProjectTarget {
public:
    friend ProjectConfig;
    using FileSets = std::vector<FileSet*>;

    static ProjectTarget* create(ProjectConfig* config, const std::string& name);

    const std::string& getName() const { return _name; }

    const FileSets& filesets() const { return _filesets; }

    void addFileSet(FileSet* fileset);

private:
    std::string _name;
    FileSets _filesets;

    explicit ProjectTarget(const std::string& name);
    ~ProjectTarget();
};

}

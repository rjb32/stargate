#pragma once

#include <string>
#include <vector>

namespace stargate {

class FileSet;

class FileSetCollector {
public:
    FileSetCollector();
    ~FileSetCollector();

    void setBasePath(const std::string& basePath) { _basePath = basePath; }

    void addFileSet(const FileSet* fileset);

    void collect(std::vector<std::string>& paths) const;

private:
    std::string _basePath;
    std::vector<const FileSet*> _filesets;
};

}

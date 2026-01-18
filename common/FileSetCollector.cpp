#include "FileSetCollector.h"

#include <unordered_set>

#include "FileSet.h"
#include "FileUtils.h"

using namespace stargate;

FileSetCollector::FileSetCollector() {
}

FileSetCollector::~FileSetCollector() {
}

void FileSetCollector::addFileSet(const FileSet* fileset) {
    _filesets.push_back(fileset);
}

void FileSetCollector::collect(std::vector<std::string>& paths) const {
    std::unordered_set<std::string> seen;
    std::vector<std::string> expanded;

    for (const FileSet* fileset : _filesets) {
        for (const std::string& pattern : fileset->patterns()) {
            expanded.clear();
            FileUtils::expandGlob(pattern, _basePath, expanded);

            for (const std::string& path : expanded) {
                if (seen.find(path) == seen.end()) {
                    seen.insert(path);
                    paths.push_back(path);
                }
            }
        }
    }
}

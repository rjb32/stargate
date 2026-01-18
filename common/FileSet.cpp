#include "FileSet.h"

using namespace stargate;

FileSet::FileSet()
{
}

FileSet::FileSet(const std::string& name)
    : _name(name)
{
}

FileSet::~FileSet() {
}

void FileSet::addFilePattern(const std::string& pattern) {
    _patterns.push_back(pattern);
}
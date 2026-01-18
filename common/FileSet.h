#pragma once

#include <string>
#include <vector>

namespace stargate {

class FileSet {
public:
    using Patterns = std::vector<std::string>;

    FileSet();
    FileSet(const std::string& name);
    ~FileSet();

    const std::string& getName() const { return _name; }

    const Patterns& patterns() const { return _patterns; }

    void setName(const std::string& name) { _name = name; }
    void addFilePattern(const std::string& pattern);

private:
    std::string _name;
    Patterns _patterns;
};

};
#pragma once

#include <string>
#include <vector>

namespace stargate {

class FileUtils {
public:
    static bool exists(const std::string& path);
    static bool isFile(const std::string& path);
    static void createDirectory(const std::string& path);
    static void removeDirectory(const std::string& path);
    static std::string absolute(const std::string& path);

    // Expand a glob pattern relative to basePath, appending absolute paths to results.
    // Supports * (single segment) and ** (recursive) wildcards.
    static void expandGlob(const std::string& pattern,
                           const std::string& basePath,
                           std::vector<std::string>& results);
};

}

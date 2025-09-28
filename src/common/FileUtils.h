#pragma once

#include <string>

namespace stargate {

class FileUtils {
public:
    static bool exists(const std::string& path);
    static void createDirectory(const std::string& path);
    static void removeDirectory(const std::string& path);
};

}
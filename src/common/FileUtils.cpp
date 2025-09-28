#include "FileUtils.h"

#include <filesystem>

#include "Panic.h"

using namespace stargate;

bool FileUtils::exists(const std::string& path) {
    return std::filesystem::exists(path);
}

void FileUtils::createDirectory(const std::string& path) {
    try {
        std::filesystem::create_directories(path);
    } catch (const std::filesystem::filesystem_error& e) {
        panic("Failed to create directory: {} {}", path, e.what());
    }
}

void FileUtils::removeDirectory(const std::string& path) {
    try {
        std::filesystem::remove_all(path);
    } catch (const std::filesystem::filesystem_error& e) {
        panic("Failed to remove directory: {} {}", path, e.what());
    }
}
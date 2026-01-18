#include "FileUtils.h"

#include <filesystem>

#include "Panic.h"

using namespace stargate;

bool FileUtils::exists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool FileUtils::isFile(const std::string& path) {
    return std::filesystem::is_regular_file(path);
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

std::string FileUtils::absolute(const std::string& path) {
    return std::filesystem::absolute(path).string();
}

namespace {

bool matchSegment(const std::string& segment, const std::string& pattern) {
    size_t si = 0;
    size_t pi = 0;
    size_t starIdx = std::string::npos;
    size_t matchIdx = 0;

    while (si < segment.size()) {
        if (pi < pattern.size() && (pattern[pi] == '?' || pattern[pi] == segment[si])) {
            si++;
            pi++;
        } else if (pi < pattern.size() && pattern[pi] == '*') {
            starIdx = pi;
            matchIdx = si;
            pi++;
        } else if (starIdx != std::string::npos) {
            pi = starIdx + 1;
            matchIdx++;
            si = matchIdx;
        } else {
            return false;
        }
    }

    while (pi < pattern.size() && pattern[pi] == '*') {
        pi++;
    }

    return pi == pattern.size();
}

void splitPath(const std::string& pattern, std::vector<std::string>& segments) {
    std::string current;

    for (char c : pattern) {
        if (c == '/' || c == '\\') {
            if (!current.empty()) {
                segments.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        segments.push_back(current);
    }
}

struct GlobState {
    std::filesystem::path path;
    size_t segmentIndex;
};

} // anonymous namespace

void FileUtils::expandGlob(const std::string& pattern,
                           const std::string& basePath,
                           std::vector<std::string>& results) {
    std::vector<std::string> segments;
    splitPath(pattern, segments);

    if (segments.empty()) {
        return;
    }

    const std::filesystem::path base = basePath.empty() ? "." : basePath;

    std::vector<GlobState> stack;
    stack.push_back({base, 0});

    while (!stack.empty()) {
        GlobState state = stack.back();
        stack.pop_back();

        if (state.segmentIndex >= segments.size()) {
            if (std::filesystem::exists(state.path)) {
                results.push_back(std::filesystem::absolute(state.path).string());
            }
            continue;
        }

        const std::string& segment = segments[state.segmentIndex];

        if (segment == "**") {
            // Match zero directories (skip **)
            stack.push_back({state.path, state.segmentIndex + 1});

            // Match one or more directories
            if (std::filesystem::exists(state.path) &&
                std::filesystem::is_directory(state.path)) {
                for (const auto& entry : std::filesystem::directory_iterator(state.path)) {
                    if (entry.is_directory()) {
                        stack.push_back({entry.path(), state.segmentIndex});
                    }
                }
            }
        } else if (segment.find('*') != std::string::npos ||
                   segment.find('?') != std::string::npos) {
            // Segment contains wildcards
            if (std::filesystem::exists(state.path) &&
                std::filesystem::is_directory(state.path)) {
                for (const auto& entry : std::filesystem::directory_iterator(state.path)) {
                    const std::string filename = entry.path().filename().string();
                    if (matchSegment(filename, segment)) {
                        stack.push_back({entry.path(), state.segmentIndex + 1});
                    }
                }
            }
        } else {
            // Literal segment
            stack.push_back({state.path / segment, state.segmentIndex + 1});
        }
    }
}

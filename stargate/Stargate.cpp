#include "Stargate.h"

#include <filesystem>
#include <fstream>

#include <spdlog/spdlog.h>

#include "ProjectConfig.h"
#include "StargateConfig.h"
#include "ProjectTarget.h"

#include "FileUtils.h"
#include "FileSetCollector.h"
#include "Panic.h"

using namespace stargate;

Stargate::Stargate(const StargateConfig& config)
    : _config(config)
{
}

Stargate::~Stargate() {
}

void Stargate::run(const ProjectConfig* projConfig) {
    createOutputDir();
    writeTargets(projConfig);
}

void Stargate::createOutputDir() {
    const auto& stargateDir = _config.getStargateDir();

    // Empty output directory if it exists, create otherwise
    if (FileUtils::exists(stargateDir)) {
        FileUtils::removeDirectory(stargateDir);
    }

    FileUtils::createDirectory(stargateDir);

    spdlog::info("Using stargate output directory {}", stargateDir);
}

void Stargate::writeTargets(const ProjectConfig* projConfig) {
    const auto basePath = std::filesystem::path(projConfig->getConfigPath()).parent_path().string();

    // Create project subdirectory
    const std::string projDirPath = _config.getStargateDir()+"/project";

    std::string targetPath;
    std::string fileListPath;
    for (const ProjectTarget* target : projConfig->targets()) {
        // Create target directory
        targetPath = projDirPath;
        targetPath += "/";
        targetPath += target->getName();

        FileUtils::createDirectory(targetPath);

        // File list path
        fileListPath = targetPath+"/files.list";

        writeTargetFileList(target, basePath, fileListPath);
    }
}

void Stargate::writeTargetFileList(const ProjectTarget* target,
                                   const std::string& basePath,
                                   const std::string& fileListPath) {
    FileSetCollector collector;
    collector.setBasePath(basePath);

    for (const FileSet* fileset : target->filesets()) {
        collector.addFileSet(fileset);
    }

    std::vector<std::string> paths;
    collector.collect(paths);

    std::ofstream out(fileListPath);
    if (!out) {
        panic("Failed to open file list for writing: {}", fileListPath);
    }

    for (const std::string& path : paths) {
        out << path << "\n";
    }
}

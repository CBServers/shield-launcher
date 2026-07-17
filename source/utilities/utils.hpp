#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace utils {
    bool createDirectoryIfNotExists(const std::filesystem::path& path);
    bool removeDirectoryRecursive(const std::filesystem::path& path);
    std::vector<std::filesystem::path> findFiles(const std::filesystem::path& directory, const std::string& pattern);
    
    std::string replaceAll(std::string str, const std::string& from, const std::string& to);
    bool endsWith(const std::string& str, const std::string& suffix);
    std::string trimEnd(std::string str, char ch);
    
    bool runCommand(const std::string& command, std::string& output);
    bool copyDirectoryRecursive(const std::filesystem::path& sourceDir, const std::filesystem::path& destDir, bool overwrite = true);
}

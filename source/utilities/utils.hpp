#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <system_error>

namespace utils {
    struct CopyFailure {
        std::filesystem::path path;
        std::error_code error;
    };

    bool createDirectoryIfNotExists(const std::filesystem::path& path);
    bool removeDirectoryRecursive(const std::filesystem::path& path);
    std::vector<std::filesystem::path> findFiles(const std::filesystem::path& directory, const std::string& pattern);
    
    std::string replaceAll(std::string str, const std::string& from, const std::string& to);
    bool endsWith(const std::string& str, const std::string& suffix);
    std::string trimEnd(std::string str, char ch);
    
    bool runCommand(const std::string& command, std::string& output);
    // Skips the copy when dest already has identical contents, so files in use by a running game are left alone.
    bool copyFileIfChanged(const std::filesystem::path& source, const std::filesystem::path& dest, std::error_code& error);
    // Copies only files that are missing or differ in dest; returns every file that could not be copied.
    std::vector<CopyFailure> syncDirectory(const std::filesystem::path& sourceDir, const std::filesystem::path& destDir);
}

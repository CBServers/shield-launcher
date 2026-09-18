#pragma once
#include <string>
#include <filesystem>
#include <system_error>

namespace DllLoading {
    enum class Result {
        Success,
        FileNotFound,
        InvalidGamePath,
        CopyError
    };

    bool isGameRunning();

    Result extractDlls(const std::filesystem::path& gameDir, bool isOnline, bool reshadeEnabled, std::error_code& copyError);

    bool launchGame(const std::filesystem::path& gameExePath, bool isOnline);
}
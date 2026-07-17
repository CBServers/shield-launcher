#pragma once
#include <string>
#include <filesystem>

namespace DllLoading {
    enum class Result {
        Success,
        FileNotFound,
        InvalidGamePath,
        CopyError
    };

    bool isGameRunning();

    Result extractDlls(const std::filesystem::path& gameDir, bool isOnline, bool reshadeEnabled);

    bool launchGame(const std::filesystem::path& gameExePath, bool isOnline);
}
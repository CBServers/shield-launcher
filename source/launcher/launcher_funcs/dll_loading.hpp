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

    // copies project-bo4/launcher/XInput9_1_0.dll into the game dir
    Result copyClientDll(const std::filesystem::path& gameDir, std::error_code& copyError);

    // flags start with a space, " -dedicated"
    bool launchGame(const std::filesystem::path& gameExePath, const std::wstring& gameFlags = L" -zombies");
}

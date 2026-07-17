#include <windows.h>
#include <string>
#include <filesystem>
#include <iostream>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    std::filesystem::path shortcutPath = exePath;

    std::filesystem::path baseDir = shortcutPath.parent_path();

    std::filesystem::path launcherPath = baseDir / "project-bo4" / "launcher" / "Shield_Launcher.exe";

    if (!std::filesystem::exists(launcherPath)) {
        MessageBoxW(NULL,
            L"Launcher executable not found. Please make sure this shortcut is in the correct location.",
            L"BO4 Launcher Error",
            MB_ICONERROR | MB_OK);
        return 1;
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::wstring cmdLine = L"\"" + launcherPath.wstring() + L"\"";
    if (lpCmdLine && *lpCmdLine) {
        cmdLine += L" ";
        cmdLine += lpCmdLine;
    }

    std::wstring launcherPathStr = launcherPath.wstring();
    std::wstring baseDirStr = baseDir.wstring();

    if (!CreateProcessW(
        launcherPathStr.c_str(),        // Path to executable
        cmdLine.data(),                 // Command line (forwards args from this shortcut)
        NULL,                           // Process security attributes
        NULL,                           // Thread security attributes
        FALSE,                          // Inherit handles
        0,                              // Creation flags
        NULL,                           // Environment
        baseDirStr.c_str(),             // Current directory
        &si,                            // Startup info
        &pi                             // Process information
    )) {
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"Failed to start launcher. Error code: %lu", GetLastError());
        MessageBoxW(NULL, errorMsg, L"BO4 Launcher Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}

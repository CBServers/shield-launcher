#include "std_include.hpp"
#include "dll_loading.hpp"
#include "utils.hpp"
#include <Windows.h>
#include <TlHelp32.h>
#include <filesystem>
#include <fstream>
#include <zlib.h>
#include <thread>
#include <atomic>

namespace fs = std::filesystem;

namespace {
    // .string() throws on names the system code page can't hold, so logged paths go out as utf-8
    std::string logPath(const fs::path& path) {
        const std::wstring& wide = path.native();
        int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
        std::string narrow(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), narrow.data(), size, nullptr, nullptr);
        return narrow;
    }
}

namespace DllLoading {
    std::thread cleanupThread;
    std::atomic<bool> cleanupThreadRunning(false);
    fs::path gameDirectoryForCleanup;
    DWORD gameProcessId = 0;

    void monitorGameAndCleanup(const fs::path& gamePath, DWORD processId) {
        cleanupThreadRunning = true;
        std::cout << "Starting cleanup monitor thread for process ID: " << processId << std::endl;
        
        gameProcessId = processId;
        
        HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, processId);
        if (hProcess == NULL) {
            std::cout << "Failed to open process handle: " << GetLastError() << std::endl;
            while (isGameRunning()) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        } else {

            std::cout << "Waiting for game process to exit..." << std::endl;
            WaitForSingleObject(hProcess, INFINITE);
            CloseHandle(hProcess);
        }
        
        gameProcessId = 0;  
        std::cout << "Game has terminated. Cleaning up DLLs..." << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        fs::path dllPath = gamePath / "XInput9_1_0.dll";
        
        try {
            if (fs::exists(dllPath)) {
                fs::remove(dllPath);
                std::cout << "Successfully deleted: " << logPath(dllPath) << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error deleting DLLs: " << e.what() << std::endl;
        }
        
        cleanupThreadRunning = false;
    }

    bool isGameRunning() {
        if (gameProcessId != 0) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, gameProcessId);
            if (hProcess != NULL) {
                DWORD exitCode;
                if (GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE) {
                    CloseHandle(hProcess);
                    return true;
                }
                CloseHandle(hProcess);
                return false;
            }
        }
        
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return false;

        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);

        if (!Process32FirstW(snapshot, &processEntry)) {
            CloseHandle(snapshot);
            return false;
        }

        bool found = false;
        do {
            if (_wcsicmp(processEntry.szExeFile, L"BlackOps4.exe") == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &processEntry));

        CloseHandle(snapshot);
        return found;
    }

    Result copyClientDll(const fs::path& gameDir, std::error_code& copyError) {
        if (gameDir.empty() || !fs::exists(gameDir / "BlackOps4.exe")) {
            return Result::InvalidGamePath;
        }

        fs::path gamePath = gameDir;

        // read from the release layout even when the launcher sits in the game root, where our copy would be the target
        fs::path srcDll = gamePath / "project-bo4" / "launcher" / "XInput9_1_0.dll";
        std::cout << "Looking for client dll at: " << logPath(srcDll) << std::endl;

        if (!fs::exists(srcDll)) {
            std::cout << "Source DLL not found" << std::endl;
            return Result::FileNotFound;
        }

        fs::path dstDll = gamePath / "XInput9_1_0.dll";
        if (!utils::copyFileIfChanged(srcDll, dstDll, copyError)) {
            std::cout << "Copy failed: " << copyError.message() << std::endl;
            return Result::CopyError;
        }

        std::cout << "Copied DLL to: " << logPath(dstDll) << std::endl;
        return Result::Success;
    }

    void cleanupDllsAfterGame() {
        cleanupThreadRunning = true;
        std::cout << "Starting DLL cleanup monitor thread..." << std::endl;
        std::cout << "Will clean up DLLs in directory: " << logPath(gameDirectoryForCleanup) << std::endl;
        
        while (isGameRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        
        std::cout << "Game has terminated. Cleaning up DLLs..." << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        if (gameDirectoryForCleanup.empty()) {
            std::cout << "Error: Game directory path is empty" << std::endl;
            cleanupThreadRunning = false;
            return;
        }
        
        fs::path dllPath = gameDirectoryForCleanup / "XInput9_1_0.dll";
        
        std::cout << "Attempting to delete DLL at: " << logPath(dllPath) << std::endl;
        
        try {
            if (fs::exists(dllPath)) {
                std::cout << "DLL found, deleting: " << logPath(dllPath) << std::endl;
                
                for (int i = 0; i < 5; i++) {
                    try {
                        if (fs::remove(dllPath)) {
                            std::cout << "Successfully deleted DLL" << std::endl;
                            break;
                        } else {
                            std::cout << "Failed to delete DLL, attempt " << (i+1) << std::endl;
                            std::this_thread::sleep_for(std::chrono::seconds(1));
                        }
                    } catch (const std::exception& e) {
                        std::cout << "Error deleting DLL (attempt " << (i+1) << "): " << e.what() << std::endl;
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                }
            } else {
                std::cout << "DLL not found at: " << logPath(dllPath) << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error accessing DLL path: " << e.what() << std::endl;
        }
        
        cleanupThreadRunning = false;
    }

    bool launchGame(const fs::path& gameExePath, const std::wstring& gameFlags) {
        std::cout << "Launching game: " << logPath(gameExePath) << std::endl;

        fs::path gameDir = gameExePath.parent_path();

        gameDirectoryForCleanup = gameDir;

        std::wstring exeStr = gameExePath.wstring();
        std::wstring dirStr = gameDir.wstring();

        SHELLEXECUTEINFOW shExInfo = {0};
        shExInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExInfo.hwnd = NULL;
        shExInfo.lpVerb = L"open";
        shExInfo.lpFile = exeStr.c_str();
        shExInfo.lpParameters = gameFlags.c_str();
        shExInfo.lpDirectory = dirStr.c_str();
        shExInfo.nShow = SW_SHOW;

        std::cout << "Launching from directory: " << logPath(gameDir) << std::endl;

        if (!ShellExecuteExW(&shExInfo)) {
            DWORD error = GetLastError();
            std::cout << "ShellExecuteEx failed with error: " << error << std::endl;
            return false;
        }
        
        if (shExInfo.hProcess) {
            CloseHandle(shExInfo.hProcess);
        }
        
        if (cleanupThreadRunning && cleanupThread.joinable()) {
            cleanupThread.join(); 
        }
        
        cleanupThread = std::thread(cleanupDllsAfterGame);
        cleanupThread.detach(); 
        
        std::cout << "Game launched successfully!" << std::endl;
        return true;
    }
}
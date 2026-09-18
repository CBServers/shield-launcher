#include "utils.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace utils {
    bool createDirectoryIfNotExists(const std::filesystem::path& path) {
        try {
            return std::filesystem::create_directories(path);
        } catch (...) {
            return false;
        }
    }

    bool removeDirectoryRecursive(const std::filesystem::path& path) {
        try {
            return std::filesystem::remove_all(path) > 0;
        } catch (...) {
            return false;
        }
    }

    std::vector<std::filesystem::path> findFiles(const std::filesystem::path& directory, const std::string& pattern) {
        std::vector<std::filesystem::path> files;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (pattern.empty() || filename.find(pattern) != std::string::npos) {
                        files.push_back(entry.path());
                    }
                }
            }
        } catch (...) {}
        return files;
    }

    std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return str;
    }

    bool endsWith(const std::string& str, const std::string& suffix) {
        if (str.length() < suffix.length()) {
            return false;
        }
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    std::string trimEnd(std::string str, char ch) {
        while (!str.empty() && str.back() == ch) {
            str.pop_back();
        }
        return str;
    }

    bool runCommand(const std::string& command, std::string& output) {
        output.clear();
        
#ifdef _WIN32
        HANDLE hReadPipe, hWritePipe;
        SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
        
        if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
            return false;
        }

        STARTUPINFOA si = { sizeof(STARTUPINFOA) };
        si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        si.wShowWindow = SW_HIDE;
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;

        PROCESS_INFORMATION pi = { 0 };
        std::string cmdLine = "cmd.exe /c " + command;
        
        if (!CreateProcessA(nullptr, const_cast<LPSTR>(cmdLine.c_str()), 
            nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            CloseHandle(hReadPipe);
            CloseHandle(hWritePipe);
            return false;
        }

        CloseHandle(hWritePipe);
        
        char buffer[4096];
        DWORD bytesRead;
        
        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr)) {
            if (bytesRead == 0) break;
            buffer[bytesRead] = 0;
            output += buffer;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hReadPipe);
        
        return exitCode == 0;
#else
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            return false;
        }

        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }

        return pclose(pipe) == 0;
#endif
    }

    static bool filesMatch(const std::filesystem::path& a, const std::filesystem::path& b) {
        std::error_code ec;
        const auto sizeA = std::filesystem::file_size(a, ec);
        if (ec) {
            return false;
        }
        const auto sizeB = std::filesystem::file_size(b, ec);
        if (ec || sizeA != sizeB) {
            return false;
        }

        std::ifstream streamA(a, std::ios::binary);
        std::ifstream streamB(b, std::ios::binary);
        if (!streamA || !streamB) {
            return false;
        }

        std::vector<char> bufA(1 << 16);
        std::vector<char> bufB(1 << 16);
        while (streamA && streamB) {
            streamA.read(bufA.data(), bufA.size());
            streamB.read(bufB.data(), bufB.size());
            if (streamA.gcount() != streamB.gcount() ||
                std::memcmp(bufA.data(), bufB.data(), static_cast<size_t>(streamA.gcount())) != 0) {
                return false;
            }
        }
        return true;
    }

    bool copyFileIfChanged(const std::filesystem::path& source, const std::filesystem::path& dest, std::error_code& error) {
        namespace fs = std::filesystem;

        error.clear();
        if (filesMatch(source, dest)) {
            return true;
        }

        // a read-only destination makes the overwrite fail with access denied
        fs::permissions(dest, fs::perms::owner_write, fs::perm_options::add, error);

        error.clear();
        return fs::copy_file(source, dest, fs::copy_options::overwrite_existing, error);
    }

    std::vector<CopyFailure> syncDirectory(const std::filesystem::path& sourceDir, const std::filesystem::path& destDir) {
        namespace fs = std::filesystem;

        std::vector<CopyFailure> failures;
        std::error_code ec;

        fs::create_directories(destDir, ec);
        if (ec) {
            failures.push_back({ destDir, ec });
            return failures;
        }

        try {
            for (const auto& entry : fs::directory_iterator(sourceDir)) {
                const fs::path sourcePath = entry.path();
                const fs::path destPath = destDir / sourcePath.filename();

                if (entry.is_directory()) {
                    auto nested = syncDirectory(sourcePath, destPath);
                    failures.insert(failures.end(), nested.begin(), nested.end());
                    continue;
                }

                if (entry.is_regular_file() && !copyFileIfChanged(sourcePath, destPath, ec)) {
                    failures.push_back({ destPath, ec });
                }
            }
        } catch (const fs::filesystem_error& e) {
            failures.push_back({ e.path1(), e.code() });
        }

        return failures;
    }

}

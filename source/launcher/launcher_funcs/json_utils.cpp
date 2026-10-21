#include "std_include.hpp"
#include "json_utils.hpp"
#include <Windows.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>

namespace fs = std::filesystem;

namespace JsonUtils {
    // the CRT keeps the underlying Windows error in _doserrno, which matches the other launcher errors
    static std::error_code lastCrtError(int crtError) {
        unsigned long osError = 0;
        if (_get_doserrno(&osError) == 0 && osError != 0) {
            return std::error_code(static_cast<int>(osError), std::system_category());
        }
        return std::error_code(crtError, std::generic_category());
    }

    std::error_code createDefaultJson(const fs::path& jsonPath) {
        std::error_code error;
        fs::create_directories(jsonPath.parent_path(), error);
        if (error) {
            return error;
        }

        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        // read the username as UTF-16 and store it as UTF-8, so accents survive
        wchar_t wideUsername[256];
        DWORD size = ARRAYSIZE(wideUsername);
        char username[512] = "Player";
        if (GetUserNameW(wideUsername, &size)) {
            WideCharToMultiByte(CP_UTF8, 0, wideUsername, -1, username, sizeof(username), nullptr, nullptr);
        }

        rapidjson::Value demonware(rapidjson::kObjectType);
        demonware.AddMember("ipv4", rapidjson::StringRef(DEFAULT_SERVER_IP), allocator);
        doc.AddMember("demonware", demonware, allocator);

        rapidjson::Value identity(rapidjson::kObjectType);
        identity.AddMember("name", rapidjson::StringRef(username), allocator);
        doc.AddMember("identity", identity, allocator);

        _set_doserrno(0);
        FILE* fp = nullptr;
        if (const errno_t openError = _wfopen_s(&fp, jsonPath.c_str(), L"wb"); openError != 0) {
            return lastCrtError(openError);
        }

        char writeBuffer[65536];
        rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
        rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
        doc.Accept(writer);

        const bool writeFailed = ferror(fp) != 0;
        if (fclose(fp) != 0 || writeFailed) {
            return lastCrtError(errno);
        }
        return {};
    }

    std::string getJsonItem(const fs::path& jsonPath, const std::string& spot, const std::string& name) {
        if (!fs::exists(jsonPath)) {
            createDefaultJson(jsonPath);
        }

        FILE* fp = nullptr;
        _wfopen_s(&fp, jsonPath.c_str(), L"rb");
        if (!fp) return "";

        char readBuffer[65536];
        rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
        rapidjson::Document doc;
        doc.ParseStream(is);
        fclose(fp);

        if (doc.HasParseError() || !doc.IsObject()) return "";

        if (!doc.HasMember(spot.c_str()) || !doc[spot.c_str()].IsObject()) return "";

        const auto& spotObj = doc[spot.c_str()];
        if (!spotObj.HasMember(name.c_str()) || !spotObj[name.c_str()].IsString()) return "";

        return spotObj[name.c_str()].GetString();
    }

    bool replaceJsonValue(const fs::path& jsonPath, const std::string& value,
                         const std::string& spot, const std::string& key) {
        if (!fs::exists(jsonPath)) {
            createDefaultJson(jsonPath);
        }

        FILE* fp = nullptr;
        _wfopen_s(&fp, jsonPath.c_str(), L"rb");
        if (!fp) return false;

        char readBuffer[65536];
        rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
        rapidjson::Document doc;
        doc.ParseStream(is);
        fclose(fp);

        if (doc.HasParseError() || !doc.IsObject()) return false;

        if (!doc.HasMember(spot.c_str())) {
            doc.AddMember(
                rapidjson::Value(spot.c_str(), doc.GetAllocator()).Move(),
                rapidjson::Value(rapidjson::kObjectType),
                doc.GetAllocator()
            );
        }

        auto& spotObj = doc[spot.c_str()];
        if (!spotObj.IsObject()) return false;

        if (spotObj.HasMember(key.c_str())) {
            spotObj[key.c_str()].SetString(value.c_str(), doc.GetAllocator());
        } else {
            spotObj.AddMember(
                rapidjson::Value(key.c_str(), doc.GetAllocator()).Move(),
                rapidjson::Value(value.c_str(), doc.GetAllocator()).Move(),
                doc.GetAllocator()
            );
        }

        _wfopen_s(&fp, jsonPath.c_str(), L"wb");
        if (!fp) return false;

        char writeBuffer[65536];
        rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
        rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
        doc.Accept(writer);
        fclose(fp);

        return true;
    }

    void migrateServerIp(const fs::path& jsonPath) {
        if (!fs::exists(jsonPath)) return;

        const std::string currentIp = getJsonItem(jsonPath, "demonware", "ipv4");
        if (std::ranges::find(LEGACY_SERVER_IPS, currentIp) != LEGACY_SERVER_IPS.end()) {
            replaceJsonValue(jsonPath, DEFAULT_SERVER_IP, "demonware", "ipv4");
        }
    }
}
#pragma once
#include <string>
#include <filesystem>
#include <system_error>
#include <array>
#include <string_view>

namespace JsonUtils {
    inline constexpr const char* DEFAULT_SERVER_IP = "142.248.82.8";
    inline constexpr std::array<std::string_view, 3> LEGACY_SERVER_IPS = {
        "70.55.126.7",
        "70.55.125.113",
        "70.55.125.26",
    };

    std::error_code createDefaultJson(const std::filesystem::path& jsonPath);

    void migrateServerIp(const std::filesystem::path& jsonPath);


    std::string getJsonItem(const std::filesystem::path& jsonPath, const std::string& spot, const std::string& name);


    bool replaceJsonValue(const std::filesystem::path& jsonPath, const std::string& value,
                         const std::string& spot, const std::string& key);
}

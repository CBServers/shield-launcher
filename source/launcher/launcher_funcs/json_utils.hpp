#pragma once
#include <string>
#include <filesystem>

namespace JsonUtils {
    inline constexpr const char* DEFAULT_SERVER_IP = "70.55.125.113";
    inline constexpr const char* LEGACY_SERVER_IP = "70.55.126.7";

    void createDefaultJson(const std::filesystem::path& jsonPath);

    void migrateServerIp(const std::filesystem::path& jsonPath);


    std::string getJsonItem(const std::filesystem::path& jsonPath, const std::string& spot, const std::string& name);


    bool replaceJsonValue(const std::filesystem::path& jsonPath, const std::string& value,
                         const std::string& spot, const std::string& key);
}

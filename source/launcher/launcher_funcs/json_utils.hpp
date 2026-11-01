#pragma once
#include <string>
#include <filesystem>
#include <system_error>

namespace JsonUtils {
    std::error_code createDefaultJson(const std::filesystem::path& jsonPath);


    std::string getJsonItem(const std::filesystem::path& jsonPath, const std::string& spot, const std::string& name);


    bool replaceJsonValue(const std::filesystem::path& jsonPath, const std::string& value,
                         const std::string& spot, const std::string& key);
}

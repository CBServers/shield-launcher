#pragma once

#include <string>
#include <filesystem>

namespace cli {
    std::filesystem::path resolveGameDir();

    bool applySetting(const std::string& section, const std::string& key,
                      const std::string& value);
}

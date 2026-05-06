#pragma once

#include <string>

namespace cli {
    bool applySetting(const std::string& section, const std::string& key,
                      const std::string& value);
}

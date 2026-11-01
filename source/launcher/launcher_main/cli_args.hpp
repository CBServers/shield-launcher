#pragma once

#include <string>
#include <vector>
#include <optional>
#include <Windows.h>

namespace cli {
    // applies -name/-ip and runs an -online/-offline/-vanilla launch, an exit code means the window should not open
    std::optional<int> run();

    // the window's launches ask through this too
    bool confirmLaunchWithWarnings(HWND owner, const std::vector<std::string>& warnings);
}

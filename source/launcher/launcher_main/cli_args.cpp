#include "std_include.hpp"
#include "cli_args.hpp"
#include "../launcher_funcs/json_utils.hpp"

namespace fs = std::filesystem;

namespace cli {

    static fs::path resolveGameDir() {
        fs::path exeDir = QCoreApplication::applicationDirPath().toStdWString();
        if (fs::exists(exeDir / "BlackOps4.exe")) {
            return exeDir;
        }
        fs::path parent = exeDir.parent_path().parent_path();
        if (fs::exists(parent / "BlackOps4.exe")) {
            return parent;
        }
        return {};
    }

    bool applySetting(const std::string& section, const std::string& key,
                      const std::string& value) {
        if (value.empty()) {
            QMessageBox::critical(nullptr, "Error",
                QString("Value for %1 cannot be empty.").arg(QString::fromStdString(key)));
            return false;
        }

        fs::path gameDir = resolveGameDir();
        if (gameDir.empty()) {
            QMessageBox::critical(nullptr, "Error",
                "Could not locate BlackOps4.exe to find project-bo4.json.");
            return false;
        }

        fs::path jsonPath = gameDir / "project-bo4.json";
        if (!JsonUtils::replaceJsonValue(jsonPath, value, section, key)) {
            QMessageBox::critical(nullptr, "Error",
                QString("Failed to write %1 to project-bo4.json.")
                    .arg(QString::fromStdString(key)));
            return false;
        }
        return true;
    }
}

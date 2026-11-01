#include "std_include.hpp"
#include "cli_args.hpp"
#include "launcher_actions.hpp"
#include "../launcher_funcs/json_utils.hpp"

namespace fs = std::filesystem;

namespace cli {
    namespace {
        constexpr const wchar_t* title = L"Project-BO4";

        // warnings past this many are only counted, the rest are in the log
        constexpr size_t shown_warnings = 5;

        constexpr const wchar_t* usage =
            L"Shield Launcher for Project BO4. Pass a mode flag to launch the game without opening the UI.\n\n"
            L"-online\tLaunch the game in online mode and exit.\n"
            L"-offline\tLaunch the game in offline mode and exit.\n"
            L"-vanilla\tLaunch the vanilla (Battle.net) game and exit.\n"
            L"-name <value>\tSet player name in project-bo4.json.\n"
            L"-ip <value>\tSet server IP in project-bo4.json.\n"
            L"-help\tShow this help.";

        std::wstring to_wide(const std::string& text) {
            int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
            std::wstring wide(size, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
            return wide;
        }

        std::string to_utf8(const std::wstring& text) {
            int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
            std::string narrow(size, '\0');
            WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), narrow.data(), size, nullptr, nullptr);
            return narrow;
        }

        void show_error(const std::wstring& text) {
            MessageBoxW(nullptr, text.c_str(), title, MB_ICONERROR | MB_OK);
        }

        bool applySetting(const std::string& section, const std::string& key, const std::string& value) {
            if (value.empty()) {
                show_error(L"Value for " + to_wide(key) + L" cannot be empty.");
                return false;
            }

            fs::path gameDir = launcher::game_dir();
            if (gameDir.empty()) {
                show_error(L"Could not locate BlackOps4.exe to find project-bo4.json.");
                return false;
            }

            if (!JsonUtils::replaceJsonValue(gameDir / "project-bo4.json", value, section, key)) {
                show_error(L"Failed to write " + to_wide(key) + L" to project-bo4.json.");
                return false;
            }
            return true;
        }
    }

    bool confirmLaunchWithWarnings(HWND owner, const std::vector<std::string>& warnings) {
        std::wstring text = L"Some files in the game folder could not be updated, so the game may not work correctly.";
        for (size_t i = 0; i < warnings.size() && i < shown_warnings; ++i)
            text += L"\n\n" + to_wide(warnings[i]);
        if (warnings.size() > shown_warnings)
            text += L"\n\n...and " + std::to_wstring(warnings.size() - shown_warnings) + L" more.";
        text += L"\n\nLaunch anyway?";

        return MessageBoxW(owner, text.c_str(), L"Launch Warning", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2) == IDYES;
    }

    std::optional<int> run() {
        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (!argv)
            return std::nullopt;

        std::vector<std::wstring> args(argv + 1, argv + argc);
        LocalFree(argv);

        bool wantOnline = false;
        bool wantOffline = false;
        bool wantVanilla = false;
        std::optional<std::string> name;
        std::optional<std::string> ip;

        for (size_t i = 0; i < args.size(); ++i) {
            const std::wstring& arg = args[i];
            if (arg.size() < 2 || arg[0] != L'-')
                continue;

            // -name and --name both work, and a value can be -name=value or the next argument
            std::wstring option = arg.substr(arg.rfind(L"--", 0) == 0 ? 2 : 1);
            std::optional<std::wstring> inlineValue;
            if (size_t equals = option.find(L'='); equals != std::wstring::npos) {
                inlineValue = option.substr(equals + 1);
                option.resize(equals);
            }

            if (option == L"online") {
                wantOnline = true;
            }
            else if (option == L"offline") {
                wantOffline = true;
            }
            else if (option == L"vanilla") {
                wantVanilla = true;
            }
            else if (option == L"name" || option == L"ip") {
                if (!inlineValue && i + 1 >= args.size()) {
                    show_error(L"Missing value for option '" + option + L"'.");
                    return 1;
                }
                (option == L"name" ? name : ip) = to_utf8(inlineValue ? *inlineValue : args[++i]);
            }
            else if (option == L"help" || option == L"h" || option == L"?") {
                MessageBoxW(nullptr, usage, title, MB_ICONINFORMATION | MB_OK);
                return 0;
            }
            else {
                show_error(L"Unknown option '" + option + L"'.");
                return 1;
            }
        }

        const int modeCount = (wantOnline ? 1 : 0) + (wantOffline ? 1 : 0) + (wantVanilla ? 1 : 0);
        if (modeCount > 1) {
            show_error(L"Error: only one of -online, -offline, -vanilla may be passed.");
            return 1;
        }

        if (name && !applySetting("identity", "name", *name)) {
            return 1;
        }
        if (ip && !applySetting("demonware", "ipv4", *ip)) {
            return 1;
        }

        if (modeCount == 0) {
            return (name || ip) ? std::optional<int>(0) : std::nullopt;
        }

        // the client is a single dll now, so -online and -offline both start it
        bool cancelled = false;
        launcher::play(wantVanilla ? launcher::play_mode::vanilla : launcher::play_mode::client,
            [](int, const char*) {},
            [&](const std::vector<std::string>& warnings) {
                cancelled = !confirmLaunchWithWarnings(nullptr, warnings);
                return !cancelled;
            },
            [&](bool ok, std::string error) {
                if (!ok && !cancelled)
                    show_error(to_wide(error));
            });
        return 0;
    }
}

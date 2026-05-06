#include "std_include.hpp"
#include "launcher_main_window.hpp"
#include "cli_args.hpp"

#include <QCommandLineParser>
#include <QTextStream>

#ifdef _WIN32
#include <Windows.h>
#endif

static int run(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Shield Launcher");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Shield Launcher for Project BO4. Pass a mode flag to launch the game without opening the UI.");
    parser.addHelpOption();

    QCommandLineOption onlineOpt("online", "Launch the game in online mode and exit.");
    QCommandLineOption offlineOpt("offline", "Launch the game in offline mode and exit.");
    QCommandLineOption vanillaOpt("vanilla", "Launch the vanilla (Battle.net) game and exit.");
    QCommandLineOption nameOpt("name", "Set player name in project-bo4.json.", "value");
    QCommandLineOption ipOpt("ip", "Set server IP in project-bo4.json.", "value");

    parser.addOption(onlineOpt);
    parser.addOption(offlineOpt);
    parser.addOption(vanillaOpt);
    parser.addOption(nameOpt);
    parser.addOption(ipOpt);

    parser.process(app);

    const bool wantOnline  = parser.isSet(onlineOpt);
    const bool wantOffline = parser.isSet(offlineOpt);
    const bool wantVanilla = parser.isSet(vanillaOpt);
    const bool hasName     = parser.isSet(nameOpt);
    const bool hasIp       = parser.isSet(ipOpt);

    const int modeCount = (wantOnline ? 1 : 0) + (wantOffline ? 1 : 0) + (wantVanilla ? 1 : 0);
    if (modeCount > 1) {
        QTextStream(stderr) << "Error: only one of -online, -offline, -vanilla may be passed.\n";
        return 1;
    }

    if (hasName) {
        if (!cli::applySetting("identity", "name", parser.value(nameOpt).toStdString())) {
            return 1;
        }
    }
    if (hasIp) {
        if (!cli::applySetting("demonware", "ipv4", parser.value(ipOpt).toStdString())) {
            return 1;
        }
    }

    if (modeCount == 0) {
        if (hasName || hasIp) {
            return 0;
        }
        MainWindow window;
        window.show();
        return app.exec();
    }

    MainWindow window(nullptr, /*cliMode=*/true);
    window.startGame(wantOnline, wantVanilla);
    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return run(__argc, __argv);
}
#else
int main(int argc, char* argv[]) {
    return run(argc, argv);
}
#endif

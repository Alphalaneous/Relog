#include <Geode/Geode.hpp>
#include "LogHandler.hpp"

using namespace geode::prelude;

void setupKeybindListener() {
    listenForKeybindSettingPresses("toggle-console-keybind", [] (Keybind const& keybind, bool down, bool repeat, double timestamp) {
        if (down) {
            LogHandler::get()->toggleConsole();
		}
    });
}

void setupSettingsListeners() {
    listenForSettingChanges<float>("font-size", [](float value) {
        queueInMainThread([] {
            LogHandler::get()->destroyConsole();
            if (LogHandler::get()->isConsoleOpen()) {
                LogHandler::get()->showConsole();
            }
        });
    });

    listenForSettingChanges<int>("blur-passes", [](int value) {
        LogHandler::get()->setBlurPasses(value);
    });

    listenForSettingChanges<bool>("enable-blur", [](bool value) {
        LogHandler::get()->showBlur(value);
    });

    ButtonSettingPressedEvent(Mod::get(), "toggle-console").listen([] (auto key) {
        LogHandler::get()->toggleConsole();
    }).leak();
}

void setupLogListener() {
    log::LogEvent().listen([] (log::BorrowedLog const& log) {
        LogHandler::get()->pushLog(log);
    }).leak();
}

$on_mod(Loaded) {
    setupKeybindListener();
    setupSettingsListeners();
    setupLogListener();
}

$on_game(TexturesLoaded) {
    if (LogHandler::get()->isConsoleOpen()) {
        LogHandler::get()->showConsole();
    }
}

$on_game(TexturesUnloaded) {
    LogHandler::get()->destroyConsole();
}
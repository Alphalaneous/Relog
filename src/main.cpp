#include <Geode/Geode.hpp>
#include "LogHandler.hpp"

using namespace geode::prelude;

static constinit comm::ListenerHandle* GlobalListener = nullptr;

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

    listenForSettingChanges<bool>("console-touch-controls", [](bool enabled) {
        LogHandler::get()->setTouchControls(enabled);
    });

#ifdef GEODE_IS_DESKTOP
    listenForSettingChanges<bool>("console-scroll-controls", [](bool enabled) {
        LogHandler::get()->setScrollControls(enabled);
    });
#endif

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
    Result<> readRes = Ok();
    if (Mod::get()->getSettingValue<bool>("read-log-file"))
        readRes = LogHandler::get()->readLogFile();
    
    ::GlobalListener = log::LogEvent().listen([] (log::BorrowedLog const& log) {
        LogHandler::get()->pushLog(log);
    }).leak();

    queueInMainThread([res = std::move(readRes)] {
        if (res.isErr())
            log::error("Failed to read log from file: {}", res.unwrapErr());
    });
}

$on_mod(Loaded) {
    setupKeybindListener();
    setupSettingsListeners();
    setupLogListener();
    log::debug("Loaded relog");
}

$on_game(TexturesLoaded) {
    if (LogHandler::get()->isConsoleOpen()) {
        LogHandler::get()->showConsole();
    }
    log::debug("Started relog");
}

$on_game(TexturesUnloaded) {
    log::debug("Unloading relog");
    // Without this the game randomly stalls for like 20 seconds
    if (auto* listener = ::GlobalListener)
        listener->destroy();
    LogHandler::get()->destroyConsole();
}

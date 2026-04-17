#pragma once

#include <Geode/Geode.hpp>
#include <asp/time/SystemTime.hpp>

using namespace geode::prelude;

namespace relog::utils {

inline Mod* getGeode() {
    static auto geode = Loader::get()->getLoadedMod("geode.loader");
    return geode;
}

inline geode::Severity severityFromString(std::string_view severity) {
    if (severity == "trace") return geode::Severity::Trace;
    if (severity == "debug") return geode::Severity::Debug;
    if (severity == "info") return geode::Severity::Info;
    if (severity == "warning") return geode::Severity::Warning;
    if (severity == "error") return geode::Severity::Error;
    return geode::Severity::Info;
}

inline std::string severityToLogString(geode::Severity severity) {
    switch (severity) {
        case Severity::Trace:
            return "TRACE";
        case Severity::Debug:
            return "DEBUG";
        case Severity::Info:
            return "INFO ";
        case Severity::Warning:
            return "WARN ";
        case Severity::Error:
            return "ERROR";
    }
    return "?????";
}

inline ccColor3B severityToColor(geode::Severity severity) {
    switch (severity) {
        case Severity::Trace:
            return {153, 83, 152};
        case Severity::Debug:
            return {148, 148, 148};
        case Severity::Info:
            return {69, 159, 255};
        case Severity::Warning:
            return {237, 230, 102};
        case Severity::Error:
            return {250, 50, 50};
    }
    return {255, 255, 255};
}


inline Severity getConsoleLogLevel() {
    static auto setting = severityFromString(getGeode()->getSettingValue<std::string>("console-log-level"));
    static auto listener = listenForSettingChanges<std::string>("console-log-level", [](std::string value) {
        setting = severityFromString(value);
    }, getGeode());

    return setting;
}

inline bool shouldLogMillisconds() {
    static auto setting = getGeode()->getSettingValue<bool>("log-milliseconds");
    static auto listener = listenForSettingChanges<bool>("log-milliseconds", [](bool value) {
        setting = value;
    }, getGeode());

    return setting;
}

inline float getFontSize() {
    static auto setting = Mod::get()->getSettingValue<float>("font-size");
    static auto listener = listenForSettingChanges<float>("font-size", [](float value) {
        setting = value;
    });

    return setting;
}

inline CCPoint getConsolePosition() {
    auto winSize = CCDirector::get()->getWinSize();
    auto x = Mod::get()->getSavedValue<float>("console-x", 20);
    auto y = Mod::get()->getSavedValue<float>("console-y", winSize.height - 20);

    return {x, y};
}

inline CCSize getConsoleSize() {
    auto width = Mod::get()->getSavedValue<float>("console-width");
    auto height = Mod::get()->getSavedValue<float>("console-height");

    return {width, height};
}

inline bool shouldOutputLog(Severity sev, Mod* mod) {
    bool console = sev >= getConsoleLogLevel();
    bool modLevel = !mod || (mod->isLoggingEnabled() && sev >= mod->getLogLevel());

    return console && modLevel;
}

}
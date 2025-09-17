#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/VideoOptionsLayer.hpp>
#include <Geode/modify/CCScene.hpp>
#include "Console.hpp"

#ifndef GEODE_IS_IOS
#include <geode.custom-keybinds/include/Keybinds.hpp>
using namespace keybinds;
#endif

using namespace geode::prelude;

auto convertTime(auto timePoint) {
    auto timeEpoch = std::chrono::system_clock::to_time_t(timePoint);
    return fmt::localtime(timeEpoch);
}

Severity fromString(std::string_view severity) {
    if (severity == "debug") return Severity::Debug;
    if (severity == "info") return Severity::Info;
    if (severity == "warning") return Severity::Warning;
    if (severity == "error") return Severity::Error;
    return Severity::Info;
}

static Severity getConsoleLogLevel() {
    static Mod* geodeMod = Loader::get()->getLoadedMod("geode.loader");
    static Severity level = fromString(geodeMod->getSettingValue<std::string>("console-log-level"));

    listenForSettingChangesV3("console-log-level", [](std::string value) {
        level = fromString(value);
    }, geodeMod);

    return level;
}

void vlogImpl_H(Severity severity, Mod* mod, fmt::string_view format, fmt::format_args args) {
	log::vlogImpl(severity, mod, format, args);

    if (!mod->isLoggingEnabled()) return;
    if (severity < mod->getLogLevel()) return;
    if (severity < getConsoleLogLevel()) return;

    Log log {
        mod,
        severity,
        fmt::vformat(format, args),
        thread::getName(),
        convertTime(std::chrono::system_clock::now())
    };

    queueInMainThread([log = std::move(log)]() mutable {
        LogStore::get()->pushLog(std::move(log));
    });
}

$on_mod(Loaded) {
    (void) Mod::get()->hook(
        reinterpret_cast<void*>(addresser::getNonVirtual(&log::vlogImpl)),
        &vlogImpl_H,
        "log::vlogImpl"
    );
    queueInMainThread([] {
        auto console = Console::create();
        console->retain();
    });

    listenForSettingChanges("ui-scale", [](float value) {
        if (auto console = Console::get()) {
            console->destroyConsole();
            console = Console::create();
            console->retain();
            LogStore::get()->repopulateConsole();
            console->m_added = true;
            SceneManager::get()->keepAcrossScenes(console);
        }
    });

    listenForSettingChanges("enable-mod", [](bool value) {
        if (!value) {
            if (auto console = Console::get()) {
                console->destroyConsole();
            }
        }
        else {
            Mod::get()->setSavedValue("hidden", false);
            auto console = Console::create();
            console->retain();
            LogStore::get()->repopulateConsole();
            console->m_added = true;
            SceneManager::get()->keepAcrossScenes(console);
        }   
    });

    #ifndef GEODE_IS_IOS
    
    BindManager::get()->registerBindable({
        "hide-console"_spr,
        "Hide Console",
        "Hides the console window",
        { Keybind::create(cocos2d::KEY_F1, Modifier::Control) },
        "Relog",
		false
    });

    new EventListener([=](InvokeBindEvent* event) {
		if (event->isDown()) {
            auto state = Mod::get()->getSavedValue<bool>("hidden");
            Mod::get()->setSavedValue("hidden", !state);
            if (auto console = Console::get()) {
                console->setVisible(state);
            }
		}
		return ListenerResult::Propagate;
    }, InvokeBindFilter(nullptr, "hide-console"_spr));

    #endif
}

class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (Mod::get()->getSettingValue<bool>("enable-mod")) {
            auto console = Console::get();
            if (!console) {
                console = Console::create();
                console->retain();
                LogStore::get()->repopulateConsole();
            }

            if (!console->m_added) {
                console->m_added = true;
                SceneManager::get()->keepAcrossScenes(console);
            }
        }

        return true;
    }
};

#ifdef GEODE_IS_DESKTOP
class $modify(VideoOptionsLayer) {
    void onApply(cocos2d::CCObject* sender) {
        if (auto console = Console::get()) {
            console->destroyConsole();
        }
        VideoOptionsLayer::onApply(sender);
    }
};
#endif

class $modify(CCScene) {
    int getHighestChildZ() {
        if (auto console = Console::get()) {
            auto original = console->getZOrder();
            console->setZOrder(-1);

            auto highest = CCScene::getHighestChildZ();
            console->setZOrder(original);
            return highest;
        }
        return CCScene::getHighestChildZ();
    }
};
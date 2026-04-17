#include "LogHandler.hpp"
#include "Utils.hpp"

LogData LogData::fromBorrowedLog(const log::BorrowedLog& log) {
    return {log.m_time, log.m_severity, log.m_nestCount, std::string(log.m_content), std::string(log.m_thread), std::string(log.m_source), log.m_mod};
}

LogHandler* LogHandler::get() {
    static LogHandler handler;
    return &handler;
}

void LogHandler::pushLog(const log::BorrowedLog& log) {
    auto logData = LogData::fromBorrowedLog(log);
    queueInMainThread([this, logData = std::move(logData)] {
        m_logs.push_back(std::move(logData));
        LogData* ptr = &m_logs.back();

        if (m_console) {
            if (!ptr->m_cell) {
                ptr->m_cell = LogCell::create(ptr);
            }
            m_console->addLog(ptr->m_cell);
        }
    });
}

void LogHandler::createConsole() {
    m_console = Console::create();
    
    for (auto& log : m_logs) {
        if (!log.m_cell) {
            log.m_cell = LogCell::create(&log);
        }
        m_console->addLog(log.m_cell);
    }

    m_console->setContentSize(relog::utils::getConsoleSize());
    m_console->setPosition(relog::utils::getConsolePosition());

    auto passes = Mod::get()->getSettingValue<int>("blur-passes");
    auto hasBlur = Mod::get()->getSettingValue<bool>("enable-blur");

    m_console->showBlur(hasBlur);
    m_console->setBlurPasses(passes);
}

void LogHandler::clearCachedCells() {
    for (auto& log : m_logs) {
        log.m_cell = nullptr;
    }
}

void LogHandler::hideConsole() {
    Mod::get()->setSavedValue("console-open", false);
    if (m_console) m_console->removeFromParentAndCleanup(false);
}

void LogHandler::showConsole() {
    Mod::get()->setSavedValue("console-open", true);
    if (!m_console) createConsole();
    OverlayManager::get()->addChild(m_console);
}

void LogHandler::destroyConsole() {
    if (m_console) {
        m_console->removeFromParent();
        clearCachedCells();
        m_console = nullptr;
    }
}

void LogHandler::toggleConsole() {
    if (isConsoleOpen()) hideConsole();
    else showConsole();
}

bool LogHandler::isConsoleOpen() {
    return Mod::get()->getSavedValue<bool>("console-open");
}

void LogHandler::setBlurPasses(unsigned int passes) {
    m_console->setBlurPasses(passes);
}

void LogHandler::showBlur(bool show) {
    m_console->showBlur(show);
}
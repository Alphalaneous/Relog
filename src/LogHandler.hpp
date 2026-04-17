#pragma once

#include <Geode/Geode.hpp>
#include "LogCell.hpp"
#include "Console.hpp"

using namespace geode::prelude;

struct LogData {
    asp::SystemTime m_time;
    Severity m_severity;
    int32_t m_nestCount;
    std::string m_content;
    std::string m_thread;
    std::string m_source;
    Mod* m_mod = nullptr;
    Ref<LogCell> m_cell;
    
    static LogData fromBorrowedLog(const log::BorrowedLog& log);
};

class LogHandler {
public:
    static LogHandler* get();
    void pushLog(const log::BorrowedLog& log);

    void createConsole();
    void destroyConsole();

    void hideConsole();
    void showConsole();

    void toggleConsole();

    void clearCachedCells();
    bool isConsoleOpen();

    void setBlurPasses(unsigned int passes);
    void showBlur(bool show);

protected:
    std::vector<LogData> m_logs;
    Ref<Console> m_console;
};
#include "LogHandler.hpp"
#include "BlurAPI.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <bitset>
#include <arc/time/Sleep.hpp>

namespace {
struct RawLogLabel {
    asp::SystemTime time;
    Severity severity;
    std::string thread;
    std::string source;
};
}

static bool IsUsingBlurAPI() {
    return BlurAPI::isBlurAPIEnabled() || BlurAPI::willLoad();
}

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
        if (m_console) {
            LogData* ptr = &m_logs.back();
            if (!ptr->m_cell) {
                ptr->m_cell = LogCell::create(ptr);
            }
            m_console->addLog(ptr->m_cell);
        }
    });
}

void LogHandler::createConsole() {
    m_console = Console::create();
    if (!m_console) {
        Notification::create(
            "Failed to create console!", 
            NotificationIcon::Warning)->show();
        return;
    }
    
    for (auto& log : m_logs) {
        if (!log.m_cell) {
            log.m_cell = LogCell::create(&log);
        }
        m_console->addLog(log.m_cell);
    }

    m_console->setContentSize(relog::utils::getConsoleSize());
    m_console->setPosition(relog::utils::getConsolePosition());

    if (IsUsingBlurAPI()) initBlur();
}

void LogHandler::initBlur() {
    if (!m_console) return;
    
    auto hasBlur = Mod::get()->getSettingValue<bool>("enable-blur");
    m_console->showBlur(hasBlur);

    if (!hasBlur) return;
    auto passes = Mod::get()->getSettingValue<int>("blur-passes");
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

void LogHandler::setTouchControls(bool enabled) {
    m_console->setTouchControls(enabled);
}

void LogHandler::setScrollControls(bool enabled) {
    m_console->setScrollControls(enabled);
}

void LogHandler::setBlurPasses(unsigned int passes) {
    m_console->setBlurPasses(passes);
}

void LogHandler::showBlur(bool show) {
    m_console->showBlur(show);
}

// readLogFile

using BitTable = std::bitset<256>;

static constexpr BitTable MakeBitTable(std::string_view chars) {
    BitTable table;
    for (char C : chars)
        table.set((unsigned char)C);
    return table;
}

static constexpr BitTable kStripTable = MakeBitTable(" \r\n\t\v");

static std::string_view LStrip(std::string_view data) {
    auto* tbl = &kStripTable;
    const char* I = data.data();
    const char* const E = I + data.size();

    while (I != E) {
        if (!tbl->test(*I))
            break;
        ++I;
    }

    return std::string_view(I, E);
}
static std::string_view RStrip(std::string_view data) {
    auto* tbl = &kStripTable;
    const char* const I = data.data();
    const char* E = I + data.size();

    while (I != E) {
        if (!tbl->test(*(E - 1)))
            break;
        --E;
    }

    return std::string_view(I, E);
}
static std::string_view Strip(std::string_view data) {
    return LStrip(RStrip(data));
}

static Result<std::string_view> TakeUntilSpace(std::string_view& label) {
    const size_t start = label.find_first_not_of(' ');
    if (start == std::string_view::npos)
        return Err("no data in string");
    
    const size_t end = label.find_first_of(' ', start);
    if (end != std::string_view::npos) [[likely]] {
        auto out = label.substr(start, end - start);
        label = label.substr(end);
        return Ok(out);
    } else [[unlikely]] {
        auto out = label.substr(start);
        label = "";
        return Ok(out);
    }
}

static Result<asp::SystemTime> ReadDateFromLabel(std::string_view& label) {
    auto timestamp = GEODE_UNWRAP(TakeUntilSpace(label));
    auto sz = timestamp.size();
    if (sz != 8 && sz != 12)
        return Err(fmt::format("timestamp '{}' is the wrong size: {}", timestamp, sz));
    
    auto takeDigits = [&timestamp, sz] (bool millis = false) -> Result<int> {
        size_t pieceSize = (millis ? 4 : 3);
        if (timestamp.size() < pieceSize)
            return Err("timestamp too small, expected {}, got {}", pieceSize, timestamp.size());
        
        int out = 0;

        char front = timestamp.front();
        if (!millis && (front >= '0' && front <= '9')) {
            out = front - '0';
            pieceSize = 2;
        } else if (front != (millis ? '.' : ':'))
            return Err("invalid first timestamp char: '{}' ({:#02x})", front, unsigned(front));
        
        for (size_t pos = 1; pos < pieceSize; ++pos) {
            const char C = timestamp[pos];
            if (!(C >= '0' && C <= '9'))
                return Err("invalid timestamp char: '{}' ({:#02x})", C, unsigned(C));
            
            out *= 10;
            out += (C - '0');
        }

        timestamp = timestamp.substr(pieceSize);
        return Ok(out);
    };

    std::tm local = asp::localtime(asp::SystemTime::now().to_time_t());
    const int localHour = local.tm_hour;

    local.tm_hour = GEODE_UNWRAP(takeDigits());
    local.tm_min = GEODE_UNWRAP(takeDigits());
    local.tm_sec = GEODE_UNWRAP(takeDigits());

    if (local.tm_hour < localHour)
        --local.tm_mday;
    const time_t nowSecs = std::mktime(&local);

    if (sz == 12) {
        int64_t milliseconds = GEODE_UNWRAP(takeDigits(/*millis=*/true));
        const auto nowMs = asp::u64((nowSecs * 1000) + milliseconds);
        return Ok(asp::SystemTime::fromUnixMillis(nowMs));
    }

    return Ok(asp::SystemTime::fromUnix(nowSecs));
}

static Result<Severity> ReadSeverityFromLabel(std::string_view& label) {
    auto sev = GEODE_UNWRAP(TakeUntilSpace(label));
    auto Invalid = [sev] () GEODE_NOINLINE {
        return Err(fmt::format("invalid severity: {}", sev));
    };

    if (sev.size() == 4) {
        if (sev == "WARN")
            return Ok(Severity::Warning);
        else if (sev == "INFO")
            return Ok(Severity::Info);
        else [[unlikely]]
            return Invalid();
    } else if (sev.size() == 5) {
        if (sev.starts_with("ERRO") && sev[4] == 'R')
            return Ok(Severity::Error);
        else if (sev.starts_with("DEBU") && sev[4] == 'G')
            return Ok(Severity::Debug);
        else if (sev.starts_with("TRAC") && sev[4] == 'E')
            return Ok(Severity::Trace);
        else if (sev.starts_with("????") && sev[4] == '?')
            return Ok(Severity::Info);
        else [[unlikely]]
            return Invalid(); 
    } else [[unlikely]] {
        return Invalid(); 
    }
}

static Result<std::string_view> ReadBracketedFromLabel(std::string_view& label) {
    std::string_view word = GEODE_UNWRAP(TakeUntilSpace(label));
    if (!word.starts_with('[') || !word.ends_with(']'))
        return Err(fmt::format("invalid word: {}", word));
    return Ok(word.substr(1, word.size() - 2));
}

static Result<RawLogLabel> TakeLogLabelFromString(std::string_view& data) {
    // Only search on the current line...
    auto pos = data.substr(0, data.find('\n')).find("]: ");
    if (pos == std::string_view::npos) {
        data = "";
        return Err("string is invalid!");
    }

    std::string_view label = data.substr(0, pos + 1);
    data = data.substr(pos + 3);

    auto time = GEODE_UNWRAP(ReadDateFromLabel(label));
    auto severity = GEODE_UNWRAP(ReadSeverityFromLabel(label));

    auto thread = GEODE_UNWRAP(ReadBracketedFromLabel(label));
    if (thread.empty())
        return Err("invalid thread or source!");

    auto source = ReadBracketedFromLabel(label).unwrapOr("");
    if (source.empty())
        std::swap(source, thread);

    return Ok(RawLogLabel {
        .time = time,
        .severity = severity,
        .thread = std::string(thread),
        .source = std::string(source)
    });
}

static std::string_view TakeContentFromString(std::string_view& data,
                                              Result<RawLogLabel>& label) {
    size_t size = data.size();
    size_t end = data.find('\n');
    size_t pos = 0;

    while (end != std::string_view::npos) {
        if (end + 1 >= data.size()) break;

        pos = end + 1;
        size = end;

        // Check if the line starts with a timestamp...
        if (char C = data[pos]; C >= '0' && C <= '9') {
            auto dataCopy = data.substr(pos);
            label = TakeLogLabelFromString(dataCopy);
            if (label.isOk()) {
                std::string_view out = data.substr(0, size);
                data = dataCopy;
                return RStrip(out);
            }
        }

        end = data.find('\n', /*off=*/pos);
    }

    std::string_view out = data.substr(0, size);
    data = "";
    label = Err("string is empty!");
    return RStrip(out);
}

using StoredModsType = std::pair<Mod*, std::string_view>;

namespace {
template <bool IsChar> struct Getter {
    using type = std::conditional_t<IsChar, char, std::string_view>;
    constexpr type operator()(const StoredModsType& val) const {
        if constexpr (IsChar)
            return val.second[0];
        else
            return val.second;
    }
    constexpr type operator()(std::string_view val) const {
        if constexpr (IsChar)
            return val[0];
        else
            return val;
    } 
    constexpr char operator()(char C) const requires IsChar {
        return C;
    } 
};
}

static std::vector<StoredModsType> GetModsThatCanBeLoaded() {
    std::vector<StoredModsType> out;
    Mod* thisMod = Mod::get();
    for (Mod* mod : Loader::get()->getAllMods()) {
        if (!mod) continue;
        if (mod == thisMod) {
            out.push_back({mod, mod->getName()});
            continue;
        }
        if (!mod->isOrWillBeEnabled()) continue;
        if (!mod->isCurrentlyLoading() && !mod->isLoaded()) continue;
        out.push_back({mod, mod->getName()});
    }

    std::sort(out.begin(), out.end(),
    [] (const StoredModsType& lhs, const StoredModsType& rhs) {
        return lhs.second < rhs.second;
    });

    return out;
}

template <bool IsChar, typename T>
static const T* BinarySearch(const std::vector<T>& vec, typename Getter<IsChar>::type value) {
    static constexpr Getter<IsChar> G;
    const T* begin = vec.data();
    const T* end = begin + vec.size();
    auto cmp = [] (const auto& lhs, const auto& rhs) {
        return G(lhs) < G(rhs);
    };

    const T* lower = std::lower_bound(begin, end, value, cmp);
    if (lower == end) return nullptr;
    if (!lower->second.starts_with(value)) return nullptr;

    if constexpr (IsChar)
        return lower;
    else {
        const T* upper = lower;
        while (upper != end) {
            ++upper;
            if (!upper->second.starts_with(value))
                break;
        } 
        return (upper - lower) == 1 ? lower : nullptr;
    }
}

static Result<std::string> ReadLogFileAsString(const std::filesystem::path& path) {
    auto newPath = dirs::getModsSaveDir() / Mod::get()->getID() / "logs" / path.filename();
    std::filesystem::create_directories(newPath.parent_path());

    std::error_code ec;
    if (!std::filesystem::copy_file(path, newPath, ec)) {
        if (ec) return Err(fmt::format("Unable to copy file: {}", ec.message()));
        return Err("Unable to copy file (reason unspecified)");
    }
    
    auto out = utils::file::readString(newPath);
    std::filesystem::remove(path, ec);
    return out;
}

Result<> LogHandler::readLogFile() {
    auto path = log::getCurrentLogPath();
    log::flush();
    // Hope that the log flushes in this time >_<
    async::spawn(arc::sleepFor(asp::Duration::fromMillis(250))).blockOn();

    std::string file = GEODE_UNWRAP(ReadLogFileAsString(path));
    if (file.empty()) return Ok();
    std::string_view data = Strip(file);
    if (data.empty()) return Ok();
    
    utils::StringMap<Mod*> nameMap;
    std::vector<StoredModsType> mods;
    std::bitset<128> name0Table;

    auto IsQuickSearch = [&name0Table] (char C) {
        return C < name0Table.size() && name0Table.test((unsigned char)C);
    };
    auto FindModFromName = [&] (const std::string& name) -> Mod* {
        if (auto it = nameMap.find(name); it != nameMap.end())
            return it->second;
        
        if (name.empty() || name.back() != '>' || name.size() < 2)
            // The name is either empty, or isn't a partial name.
            // Either way, the mod can't be deduced.
            return nullptr;
        
        // Do a binary search...
        std::string_view svname(name);
        svname.remove_suffix(1);
        // Check if only the first character needs to be found.
        if (IsQuickSearch(svname[0])) {
            if (auto* it = BinarySearch<true>(mods, svname[0]))
                return it->first;
        } else {
            // Do the full search
            if (auto* it = BinarySearch<false>(mods, svname))
                return it->first;
        }

        return nullptr;
    };

    /*Init mod map*/ {
        auto modList = GetModsThatCanBeLoaded();
        nameMap.reserve(modList.size());
        mods.reserve(modList.size());

        std::bitset<128> presenceTable;
        for (auto [mod, name] : modList) {
            std::string modName(name);
            // If we have the same name (for some reason) just skip it.
            // It doesnt really matter that much if the mod value is accurate...
            if (nameMap.contains(modName)) continue;
            nameMap.insert({std::move(modName), mod});
            mods.push_back({mod, name});

            const size_t C = static_cast<unsigned char>(name[0]);
            if (C < name0Table.size()) {
                if (name0Table.test(C)) {
                    // If this has already been set, unset it and add to the
                    // presence table.
                    name0Table.set(C, false);
                    presenceTable.set(C);
                } else if (!presenceTable.test(C)) {
                    // This isn't in the presence table, so we can safely set it.
                    name0Table.set(C);
                }
            }
        }
    }

    std::vector<LogData> logs;
    Result<RawLogLabel> labelRes = TakeLogLabelFromString(data);

    while (!data.empty()) {
        RawLogLabel label = GEODE_UNWRAP(std::move(labelRes));
        Mod* mod = FindModFromName(label.source);
        std::string_view content = TakeContentFromString(data, labelRes);
        
        int nestCount = 0;
        if (auto off = content.find_first_not_of(' ');
            off != std::string_view::npos) {
            // We have leading spaces
            nestCount = static_cast<int>(off);
        }

        logs.push_back({
            .m_time = label.time,
            .m_severity = label.severity,
            .m_nestCount = nestCount,
            .m_content = std::string(content),
            .m_thread = std::move(label.thread),
            .m_source = std::move(label.source),
            .m_mod = mod
        });
    }

    logs.push_back({
        .m_time = asp::SystemTime::now(),
        .m_severity = Severity::Info,
        .m_nestCount = 0,
        .m_content = "Loaded logs from file!",
        .m_thread = std::string(utils::thread::getName()),
        .m_source = std::string(Mod::get()->getName()),
        .m_mod = Mod::get()
    });

    queueInMainThread([this, logs = std::move(logs)] {
        for (auto& logData : logs) {
            m_logs.push_back(std::move(logData));
            LogData* ptr = &m_logs.back();

            if (m_console) {
                if (!ptr->m_cell) {
                    ptr->m_cell = LogCell::create(ptr);
                }
                m_console->addLog(ptr->m_cell);
            }
        }
    });

    return Ok();
}

#include "LogCell.hpp"
#include "LogHandler.hpp"
#include "Utils.hpp"

CCSize LogCell::s_charSize;
bool LogCell::s_initFont = false;

LogCell* LogCell::create(LogData* logData) {
    auto ret = new LogCell();
    if (ret->init(logData)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool LogCell::init(LogData* logData) {
    if (!CCNode::init()) return false;

    m_data = logData;

    setAnchorPoint({0, 1});

    if (!s_initFont) {
        s_charSize = CCLabelBMFont::create("A", "Consolas.fnt"_spr)->getContentSize();
        s_initFont = true;
    }

    auto lines = utils::string::split(logData->m_content, "\n");

    for (const auto& [idx, line] : asp::iter::from(lines).enumerate()) {
        auto vec = std::vector<CCLabelBMFont*>();
        auto parts = utils::string::split(line, " ");

        if (idx == 0) {
            auto ms = logData->m_time.timeSinceEpoch().millis() % 1000;
            auto local = asp::localtime(logData->m_time.to_time_t());

            std::string timeStr;

            if (relog::utils::shouldLogMillisconds()) {
                timeStr = fmt::format("{:%H:%M:%S}.{:03}", local, ms);
            }
            else {
                timeStr = fmt::format("{:%H:%M:%S}", local);
            }

            auto color = relog::utils::severityToColor(logData->m_severity);

            auto timeLabel = CCLabelBMFont::create(timeStr.c_str(), "Consolas.fnt"_spr);
            timeLabel->setAnchorPoint({0, 1});
            timeLabel->setColor(color);
            addChild(timeLabel);

            vec.push_back(timeLabel);

            auto severityStr = relog::utils::severityToLogString(logData->m_severity);
            auto severityLabel = CCLabelBMFont::create(severityStr.c_str(), "Consolas.fnt"_spr);

            severityLabel->setAnchorPoint({0, 1});
            severityLabel->setColor(color);
            addChild(severityLabel);

            vec.push_back(severityLabel);

            auto threadStr = fmt::format("[{}]", logData->m_thread);
            auto threadLabel = CCLabelBMFont::create(threadStr.c_str(), "Consolas.fnt"_spr);

            threadLabel->setAnchorPoint({0, 1});
            addChild(threadLabel);

            vec.push_back(threadLabel);

            auto modStr = fmt::format("[{}]", logData->m_mod->getName());
            auto modLabel = CCLabelBMFont::create(modStr.c_str(), "Consolas.fnt"_spr);
            
            modLabel->setAnchorPoint({0, 1});
            addChild(modLabel);

            vec.push_back(modLabel);
        }

        for (const auto& part : parts) {
            auto label = CCLabelBMFont::create(part.c_str(), "Consolas.fnt"_spr);
            label->setAnchorPoint({0, 1});
            addChild(label);

            vec.push_back(label);
        }
        m_labels.push_back(std::move(vec));
    }

    return true;
}

void LogCell::setFontScale(float scale) {
    m_fontScale = scale;
    updateWrapping();
}

float LogCell::getFontScale() {
    return m_fontScale;
}

LogData* LogCell::getLogData() {
    return m_data;
}

void LogCell::updateWrapping() {
    if (!m_bRunning) return;
    auto fontScale = m_fontScale * relog::utils::getFontSize();
    m_lockWrapping = true;
    float width = getContentWidth();
    float accumWidth = 0;
    float lineHeight = (s_charSize.height + LINE_DISTANCE) * fontScale;
    float spaceWidth = s_charSize.width * fontScale;

    struct PosData {
        CCLabelBMFont* label;
        float xPos;
        unsigned int linePos;
    };

    std::vector<PosData> lines;

    unsigned int linePos = 0;

    for (const auto& line : m_labels) {
        for (auto part : line) {
            part->setScale(fontScale);

            float partWidth = part->getScaledContentWidth();

            bool tooWide = accumWidth + partWidth > width;
            bool isLongWord = accumWidth == 0 && partWidth > width;

            if (tooWide && !isLongWord) {
                accumWidth = 0;
                linePos++;
            }

            float x = accumWidth;
            accumWidth += partWidth + spaceWidth;

            lines.push_back({part, x, linePos});
        }

        accumWidth = 0;
        linePos++;
    }

    setContentHeight(linePos * lineHeight);

    for (const auto& line : lines) {
        line.label->setPosition({line.xPos, getContentHeight() - line.linePos * lineHeight});
    }

    m_lockWrapping = false;
    m_wrapSet = true;
    runAction(CallFuncExt::create([this] {
        m_wrapSet = false;
    }));
}

void LogCell::onEnter() {
    CCNode::onEnter();
    updateWrapping();
}

void LogCell::setContentSize(const CCSize& contentSize) {
    auto oldWidth = getContentWidth();
    CCNode::setContentSize(contentSize);
    if (!m_lockWrapping && oldWidth != contentSize.width && !m_wrapSet) {
        updateWrapping();
    }
}
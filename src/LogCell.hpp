#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class LogData;

class LogCell : public CCNode {
public:
    static LogCell* create(LogData* logData);
    void updateWrapping();

    void setContentSize(const CCSize& contentSize) override;
    void setFontScale(float scale);
    float getFontScale();
    LogData* getLogData();

protected:
    bool init(LogData* logData);
    void onEnter() override;

    LogData* m_data;

    bool m_lockWrapping = false;
    bool m_wrapSet = false;
    float m_fontScale = 0.3f;

    static CCSize s_charSize;
    static bool s_initFont;
    static constexpr float LINE_DISTANCE = 2.f;

    std::vector<std::vector<CCLabelBMFont*>> m_labels;
};
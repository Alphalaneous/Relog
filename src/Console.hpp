#pragma once

#include <Geode/Geode.hpp>
#include "BoundedScrollLayer.hpp"

using namespace geode::prelude;

struct Log {
    Mod* mod;
    Severity severity = Severity::Info;
    std::string message;
    std::string threadName;
    std::tm time;
    bool newLine;
    int offset;
};

class LogCell : public CCNode {
protected:
    Log m_log;

public:
    static LogCell* create(Log log, CCSize size);
    bool init(Log log, CCSize size);
    void resize(CCSize size);
    void refresh();
};

class DragBar : public CCLayerColor {
protected:
    CCNode* m_nodeToMove;
    CCSprite* m_resizeSprite;
    CCSprite* m_minimizeSprite;
    CCLabelBMFont* m_logsLabel;
    cocos2d::CCPoint m_lastTouchPos;
    bool m_dragging = false;
    bool m_resizing = false;
    bool m_minimized = false;
    CCSize m_queuedSize = {300, 150};
    CCSize m_expectedContentSize = {300, 150};
public:
    static DragBar* create();
    bool init() override;
    void setContentSize(const CCSize& size) override;
    void resizeSchedule(float dt);
    void setMinimized(bool minimized);
    void setNodeToMove(CCNode* node);
    void registerWithTouchDispatcher() override;
    void refresh();
    bool ccTouchBegan(CCTouch *pTouch, CCEvent *pEvent) override;
    void ccTouchMoved(CCTouch *pTouch, CCEvent *pEvent) override;
    void ccTouchEnded(CCTouch *pTouch, CCEvent *pEvent) override;
    void ccTouchCancelled(CCTouch *pTouch, CCEvent *pEvent) override;
};

class LogStore {
protected:
    static LogStore* s_instance;
    std::vector<Log> m_logs;
public:
    static LogStore* get();
    std::vector<Log> getLogs();
    void pushLog(Log log);
    void repopulateConsole();
};

class Console : public CCLayerColor {
protected:
    static Console* s_instance;
    geode::Scrollbar* m_scrollbar;
    CCMenu* m_blockMenu;
    CCMenuItemSpriteExtra* m_blockMenuItem;
    DragBar* m_dragBar;
    bool m_minimized = false;

public:
    static Console* create();
    static Console* get();

    ~Console();

    bool init() override;

    void setContentSize(const CCSize& size) override;
    void setPosition(const CCPoint& point) override;
    void setMinimized(bool minimized);
    void destroyConsole();

    CCNode* createCell(Log log);
    void pushLog(Log log, bool updateLayout = true);
    void updateScrollLayout();
    void refresh();

    void onEnter() override;

    bool m_added = false;
    BoundedScrollLayer* m_scrollLayer;
};
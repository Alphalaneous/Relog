#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <alphalaneous.alphas-ui-pack/include/API.hpp>
#include "LogCell.hpp"

using namespace geode::prelude;
using namespace alpha::prelude;

class Console : public CCNode, public TouchDelegate {
public:
    static Console* create();

    void onEnter() override;
    void onExit() override;

	bool clickBegan(TouchEvent* touch) override;
    void setContentSize(const CCSize& contentSize) override;
    void setPosition(const CCPoint& position) override;

    void addLog(LogCell* log);
    void setBlurPasses(unsigned int passes);
    void showBlur(bool show);

    geode::NineSlice* getBackground();
    AdvancedScrollLayer* getScrollLayer();
    CCNodeRGBA* getGrabber();

protected:
    bool init() override;

    geode::NineSlice* m_background;
    geode::NineSlice* m_border;
    CCNodeRGBA* m_grabber;
    AdvancedScrollLayer* m_scrollLayer;
};

class Dragger : public CCNode, public TouchDelegate {
public:
    static Dragger* create(Console* console);

    void onEnter() override;
    void onExit() override;

    bool clickBegan(TouchEvent* touch) override;
	void clickMoved(TouchEvent* touch) override;
	void clickEnded(TouchEvent* touch) override;
protected:
    bool init(Console* console);

    void waitForHold(float dt);

    CCPoint m_startLocation;
    CCPoint m_consolePos;
    CCSize m_consoleSize;
    Console* m_console;
    bool m_holding;
    bool m_holdingGrabber;
};
#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <alphalaneous.alphas-ui-pack/include/API.hpp>
#include "LogCell.hpp"

using namespace geode::prelude;
using namespace alpha::prelude;

class Dragger;

class Console : public CCNode, public TouchDelegate {
public:
    static Console* create();

    void onEnter() override;
    void onExit() override;

	bool clickBegan(TouchEvent* touch) override;
    void setContentSize(const CCSize& contentSize) override;
    void setPosition(const CCPoint& position) override;

    void addLog(LogCell* log);
    void setTouchControls(bool enable);
    void setScrollControls(bool enable);
    void setBlurPasses(unsigned int passes);
    void showBlur(bool show);

    geode::NineSlice* getBackground();
    AdvancedScrollLayer* getScrollLayer();
    CCNodeRGBA* getGrabber();

    inline bool isKeyDown() const {
#ifdef GEODE_IS_DESKTOP
        return m_keyDown;
#else
        return false;
#endif
    }

protected:
    bool init() override;
    void initCheckPressed(bool enable);

    void checkKeyDown(float dt);

    geode::NineSlice* m_background = nullptr;
    geode::NineSlice* m_border = nullptr;
    CCNodeRGBA* m_grabber = nullptr;
    AdvancedScrollLayer* m_scrollLayer = nullptr;
    Dragger* m_touchOverlay = nullptr;
    bool m_touchControls = true;
    bool m_scrollControls = false;
    bool m_keyDown = false;
};

class Dragger : public CCNode, public TouchDelegate {
public:
    static Dragger* create(Console* console);

    void onEnter() override;
    void onExit() override;

    bool clickBegan(TouchEvent* touch) override;
	void clickMoved(TouchEvent* touch) override;
	void clickEnded(TouchEvent* touch) override;
    void instantHold();
protected:
    bool init(Console* console);

    void waitForHold(float dt);

    CCPoint m_startLocation;
    CCPoint m_consolePos;
    CCSize m_consoleSize;
    Console* m_console;
    bool m_clicked = false;
    bool m_holding = false;
    bool m_holdingGrabber = false;
};

#include "Console.hpp"
#include "Utils.hpp"
#include "LogHandler.hpp"
#include "BlurAPI.hpp"

Console* Console::create() {
    auto ret = new Console();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool Console::init() {
    if (!CCNode::init()) return false;

    m_background = geode::NineSlice::create("square.png"_spr);
    m_background->setColor({0, 0, 0});
    m_background->setScaleMultiplier(0.5f);
    m_background->setPosition(getContentSize() / 2);
    m_background->setOpacity(220);

    m_border = geode::NineSlice::create("border.png"_spr);
    m_border->setColor({255, 255, 255});
    m_border->setScaleMultiplier(0.5f);
    m_border->setPosition(getContentSize() / 2);
    m_border->setOpacity(175);
    m_border->setZOrder(10);

    m_grabber = CCNodeRGBA::create();
    m_grabber->setCascadeOpacityEnabled(true);
    m_grabber->setAnchorPoint({0.5f, 0.5f});

    auto grabberSpr = CCSprite::create("grabber.png"_spr);
    grabberSpr->setScale(0.5f);

    m_grabber->addChild(grabberSpr);
    m_grabber->setContentSize(grabberSpr->getScaledContentSize() * 1.5f);
    grabberSpr->setPosition(m_grabber->getContentSize() / 2);

    m_grabber->setOpacity(100);

    m_background->addChild(m_grabber);

    m_scrollLayer = AdvancedScrollLayer::create({192, 99});

    auto layout = static_cast<SimpleColumnLayout*>(ScrollLayer::createDefaultListLayout(0));
    layout->setPadding({0, 3, 0, 3});

    m_scrollLayer->getContentLayer()->setLayout(layout);
    m_scrollLayer->setAnchorPoint({0, 0});
    m_scrollLayer->setPosition({4, 0.5f});

    addChild(m_background);
    m_background->addChild(m_scrollLayer);
    m_background->addChild(m_border);

    setAnchorPoint({0, 1});

    setContentSize({200, 100});

    auto touchOverlay = Dragger::create(this);
    touchOverlay->setZOrder(10000);
    addChild(touchOverlay);

    return true;
}

void Console::addLog(LogCell* log) {
    if (!relog::utils::shouldOutputLog(log->getLogData()->m_severity, log->getLogData()->m_mod)) return;

    if (m_scrollLayer->getContentLayer()->getChildrenCount() > Mod::get()->getSettingValue<int>("log-limit")) {
        m_scrollLayer->getContentLayer()->getChildrenExt()[0]->removeFromParent();
    }

    bool isBottom = m_scrollLayer->getScrollPoint().y == m_scrollLayer->getVerticalMax();
    log->setContentWidth(m_scrollLayer->getContentWidth());

    if (m_scrollLayer->getContentLayer()->getChildrenCount() == 0) {
        m_scrollLayer->getContentLayer()->addChild(log);
    }
    else {
        m_scrollLayer->getContentLayer()->insertAfter(log, static_cast<CCNode*>(m_scrollLayer->getContentLayer()->getChildren()->lastObject()));
    }
    m_scrollLayer->getContentLayer()->updateLayout();
    if (isBottom) {
        m_scrollLayer->setScrollY(m_scrollLayer->getVerticalMax());
    }
}

void Console::onEnter() {
    CCNode::onEnter();
    CCTouchDispatcher::get()->addTargetedDelegate(this, 10000 /*Scary*/, true);
}

void Console::onExit() {
    CCNode::onExit();
    CCTouchDispatcher::get()->removeDelegate(this);
}

bool Console::clickBegan(TouchEvent* touch) {
    if (!nodeIsVisible(this)) return false;
    if (touch->getButton() != MouseButton::LEFT) return false;
    if (alpha::utils::isPointInsideNode(m_grabber, touch->getLocation())) return true;
    if (!alpha::utils::isPointInsideNode(this, touch->getLocation())) return false;

    return true;
}

void Console::setContentSize(const CCSize& contentSize) {
    auto newSize = contentSize;
    auto winSize = CCDirector::get()->getWinSize();

    newSize.width = std::min(std::max(newSize.width, 20.f), winSize.width - 20.f);
    newSize.height = std::min(std::max(newSize.height, 20.f), winSize.height - 20.f);

    CCNode::setContentSize(newSize);

    Mod::get()->setSavedValue("console-width", newSize.width);
    Mod::get()->setSavedValue("console-height", newSize.height);

    setPosition(getPosition());

    if (m_background) {
        m_background->setContentSize(newSize);
        m_background->setPosition(newSize / 2);
    }
    if (m_border) {
        m_border->setContentSize(newSize);
        m_border->setPosition(newSize / 2);
    }

    if (m_grabber) {
        m_grabber->setPosition({newSize.width, 0});
    }

    if (m_scrollLayer) {
        bool isBottom = m_scrollLayer->getScrollPoint().y == m_scrollLayer->getVerticalMax();

        m_scrollLayer->setContentSize(newSize - CCSize{8, 1.f});
        for (auto cell : m_scrollLayer->getContentLayer()->getChildrenExt<LogCell>()) {
            cell->setContentWidth(m_scrollLayer->getContentWidth());
        }

        m_scrollLayer->getContentLayer()->setContentWidth(m_scrollLayer->getContentWidth());
        runAction(CallFuncExt::create([this, isBottom] {
            m_scrollLayer->getContentLayer()->updateLayout();

            if (isBottom) {
                m_scrollLayer->setScrollY(m_scrollLayer->getVerticalMax());
            }
            else {
                // hacky cull fix, I should update aup to allow manual cull calls
                m_scrollLayer->setScrollY(m_scrollLayer->getScrollPoint().y - 0.1f);
                m_scrollLayer->setScrollY(m_scrollLayer->getScrollPoint().y + 0.1f);
            }
        }));

    }
}

void Console::setPosition(const CCPoint& position) {
    auto newPos = position;

    CCSize winSize = CCDirector::get()->getWinSize();
        
    newPos.x = std::min(std::max(newPos.x, -getContentWidth() + 10), winSize.width - 10);
    newPos.y = std::min(std::max(newPos.y, 10.f), winSize.height + getContentHeight() - 10);

    CCNode::setPosition(newPos);
}

geode::NineSlice* Console::getBackground() {
    return m_background;
}

AdvancedScrollLayer* Console::getScrollLayer() {
    return m_scrollLayer;
}

CCNodeRGBA* Console::getGrabber() {
    return m_grabber;
}

void Console::setBlurPasses(unsigned int passes) {
    BlurAPI::getOptions(this)->forcePasses = true;
    BlurAPI::getOptions(this)->passes = passes;
}

void Console::showBlur(bool show) {
    if (show) {
        BlurAPI::addBlur(this);
    }
    else {
        BlurAPI::removeBlur(this);
    }
}

Dragger* Dragger::create(Console* console) {
    auto ret = new Dragger();
    if (ret->init(console)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool Dragger::init(Console* console) {
    if (!CCNode::init()) return false;
    m_console = console;
    return true;
}

void Dragger::onEnter() {
    CCNode::onEnter();
    CCTouchDispatcher::get()->addTargetedDelegate(this, 10001 /*Scary*/, false);
}

void Dragger::onExit() {
    CCNode::onExit();
    CCTouchDispatcher::get()->removeDelegate(this);
}

bool Dragger::clickBegan(TouchEvent* touch) {
    if (!nodeIsVisible(m_console)) return false;
    if (touch->getButton() != MouseButton::LEFT) return false;
    if (alpha::utils::isPointInsideNode(m_console->getGrabber(), touch->getLocation())) {
        m_holdingGrabber = true;
        m_startLocation = touch->getLocation();
        m_consoleSize = m_console->getContentSize();
        m_console->getGrabber()->setOpacity(175);
        return true;
    }
    if (!alpha::utils::isPointInsideNode(m_console, touch->getLocation())) return false;

    m_startLocation = touch->getLocation();
    
    scheduleOnce(schedule_selector(Dragger::waitForHold), 0.5f);

    return true;
}

void Dragger::clickMoved(TouchEvent* touch) {
    CCPoint prev = convertToNodeSpace(m_startLocation);
    CCPoint curr = convertToNodeSpace(touch->getLocation());
    CCPoint delta = curr - prev;

    if (m_holdingGrabber) {
        m_console->setContentSize(m_consoleSize + CCSize{delta.x, -delta.y});
        return;
    }

    if (!m_holding) {
        if (delta.getLength() > 10) {
            unschedule(schedule_selector(Dragger::waitForHold));
        }
    }
    else {
        m_console->setPosition(m_consolePos + delta); 

        Mod::get()->setSavedValue("console-x", m_console->getPositionX());
        Mod::get()->setSavedValue("console-y", m_console->getPositionY());
    }
}

void Dragger::clickEnded(TouchEvent* touch) {
    if (m_holding) {
        m_console->getBackground()->stopAllActions();
        m_console->getBackground()->runAction(CCScaleTo::create(0.1f, 1.0f));
        m_console->getBackground()->runAction(CCFadeTo::create(0.1f, 220));
    }
    m_holdingGrabber = false;
    m_holding = false;
    m_console->getGrabber()->setOpacity(100);
    unschedule(schedule_selector(Dragger::waitForHold));
}

void Dragger::waitForHold(float dt) {
    m_holding = true;
    auto touch = new CCTouch();
    m_console->getScrollLayer()->ccTouchCancelled(touch, nullptr);
    touch->release();

    m_console->getBackground()->runAction(CCScaleTo::create(0.1f, 1.05f));
    m_console->getBackground()->runAction(CCFadeTo::create(0.1f, 190));

    m_consolePos = m_console->getPosition();
}
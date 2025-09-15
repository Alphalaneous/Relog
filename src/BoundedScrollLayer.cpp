#include "BoundedScrollLayer.hpp"
#include <Geode/modify/CCMouseDispatcher.hpp>
#include "Console.hpp"

#ifndef GEODE_IS_IOS
// a bit hacky
class $modify(CCMouseDispatcher) {
	bool dispatchScrollMSG(float x, float y) {
        if (auto console = Console::get()) {
            if (console->m_scrollLayer->testLocation(getMousePos())) {
                console->m_scrollLayer->m_doScroll = true;
                console->m_scrollLayer->scrollWheel(x, y);
                console->m_scrollLayer->m_doScroll = false;
                return true;
            }
        }
        return CCMouseDispatcher::dispatchScrollMSG(x, y);
    }

	void addDelegate(CCMouseDelegate* pDelegate) {
        if (BoundedScrollLayer* scroll = typeinfo_cast<BoundedScrollLayer*>(pDelegate)) return;
        CCMouseDispatcher::addDelegate(pDelegate);
    }

};
#endif

BoundedScrollLayer* BoundedScrollLayer::create(CCSize const& size) {
    auto ret = new BoundedScrollLayer({ 0, 0, size.width, size.height });
    ret->autorelease();
    return ret;
}

bool BoundedScrollLayer::testLocation(CCPoint point) {
    CCPoint mousePoint = convertToNodeSpace({point.x + getPositionX(), point.y + getPositionY()});

    if (boundingBox().containsPoint(mousePoint)) {
        return true;
    }
    
    return false;
}

void BoundedScrollLayer::scrollWheel(float y, float x)  {
    if (!m_doScroll) return;

    ScrollLayer::scrollWheel(y, x);
}

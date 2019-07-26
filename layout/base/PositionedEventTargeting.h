



#ifndef mozilla_PositionedEventTargeting_h
#define mozilla_PositionedEventTargeting_h

#include <stdint.h>

class nsIFrame;
class nsGUIEvent;
struct nsPoint;

namespace mozilla {

enum {
  INPUT_IGNORE_ROOT_SCROLL_FRAME = 0x01
};





nsIFrame*
FindFrameTargetedByInputEvent(const nsGUIEvent *aEvent,
                              nsIFrame* aRootFrame,
                              const nsPoint& aPointRelativeToRootFrame,
                              uint32_t aFlags = 0);

}

#endif 





#ifndef mozilla_PositionedEventTargeting_h
#define mozilla_PositionedEventTargeting_h

#include "nsPoint.h"

class nsGUIEvent;
class nsIFrame;

namespace mozilla {

enum {
  INPUT_IGNORE_ROOT_SCROLL_FRAME = 0x01
};





nsIFrame*
FindFrameTargetedByInputEvent(uint8_t aEventStructType,
                              nsIFrame* aRootFrame,
                              const nsPoint& aPointRelativeToRootFrame,
                              uint32_t aFlags = 0);

}

#endif 

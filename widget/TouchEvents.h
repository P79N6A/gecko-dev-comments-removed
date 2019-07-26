




#ifndef mozilla_TouchEvents_h__
#define mozilla_TouchEvents_h__

#include <stdint.h>

#include "mozilla/dom/Touch.h"
#include "mozilla/MouseEvents.h"
#include "nsAutoPtr.h"
#include "nsIDOMSimpleGestureEvent.h"
#include "nsTArray.h"

namespace mozilla {













class WidgetGestureNotifyEvent : public WidgetGUIEvent
{
public:
  WidgetGestureNotifyEvent(bool aIsTrusted, uint32_t aMessage,
                           nsIWidget *aWidget) :
    WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_GESTURENOTIFY_EVENT),
    panDirection(ePanNone), displayPanFeedback(false)
  {
  }

  enum ePanDirection
  {
    ePanNone,
    ePanVertical,
    ePanHorizontal,
    ePanBoth
  };

  ePanDirection panDirection;
  bool displayPanFeedback;

  
  void AssignGestureNotifyEventData(const WidgetGestureNotifyEvent& aEvent,
                                    bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    panDirection = aEvent.panDirection;
    displayPanFeedback = aEvent.displayPanFeedback;
  }
};





class WidgetSimpleGestureEvent : public WidgetMouseEventBase
{
public:
  WidgetSimpleGestureEvent(bool aIsTrusted, uint32_t aMessage,
                           nsIWidget* aWidget, uint32_t aDirection,
                           double aDelta) :
    WidgetMouseEventBase(aIsTrusted, aMessage, aWidget,
                         NS_SIMPLE_GESTURE_EVENT),
    allowedDirections(0), direction(aDirection), delta(aDelta), clickCount(0)
  {
  }

  WidgetSimpleGestureEvent(const nsSimpleGestureEvent& aOther) :
    WidgetMouseEventBase(aOther.mFlags.mIsTrusted,
                         aOther.message, aOther.widget,
                         NS_SIMPLE_GESTURE_EVENT),
    allowedDirections(aOther.allowedDirections), direction(aOther.direction),
    delta(aOther.delta), clickCount(0)
  {
  }

  
  uint32_t allowedDirections;
  
  uint32_t direction;
  
  double delta;
  
  uint32_t clickCount;

  
  void AssignSimpleGestureEventData(const WidgetSimpleGestureEvent& aEvent,
                                    bool aCopyTargets)
  {
    AssignMouseEventBaseData(aEvent, aCopyTargets);

    
    direction = aEvent.direction;
    delta = aEvent.delta;
    clickCount = aEvent.clickCount;
  }
};





class WidgetTouchEvent : public WidgetInputEvent
{
public:
  WidgetTouchEvent()
  {
  }

  WidgetTouchEvent(bool aIsTrusted, WidgetTouchEvent* aEvent) :
    WidgetInputEvent(aIsTrusted, aEvent->message, aEvent->widget,
                     NS_TOUCH_EVENT)
  {
    modifiers = aEvent->modifiers;
    time = aEvent->time;
    touches.AppendElements(aEvent->touches);
    MOZ_COUNT_CTOR(WidgetTouchEvent);
  }

  WidgetTouchEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget) :
    WidgetInputEvent(aIsTrusted, aMessage, aWidget, NS_TOUCH_EVENT)
  {
    MOZ_COUNT_CTOR(WidgetTouchEvent);
  }

  ~WidgetTouchEvent()
  {
    MOZ_COUNT_DTOR(WidgetTouchEvent);
  }

  nsTArray<nsRefPtr<mozilla::dom::Touch>> touches;

  void AssignTouchEventData(const nsTouchEvent& aEvent, bool aCopyTargets)
  {
    AssignInputEventData(aEvent, aCopyTargets);

    
  }
};

} 


typedef mozilla::WidgetGestureNotifyEvent nsGestureNotifyEvent;
typedef mozilla::WidgetSimpleGestureEvent nsSimpleGestureEvent;
typedef mozilla::WidgetTouchEvent         nsTouchEvent;

#endif 

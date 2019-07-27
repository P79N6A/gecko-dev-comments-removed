




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
  virtual WidgetGestureNotifyEvent* AsGestureNotifyEvent() MOZ_OVERRIDE
  {
    return this;
  }

  WidgetGestureNotifyEvent(bool aIsTrusted, uint32_t aMessage,
                           nsIWidget *aWidget) :
    WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_GESTURENOTIFY_EVENT),
    panDirection(ePanNone), displayPanFeedback(false)
  {
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    
    
    
    
    
    MOZ_ASSERT(mClass == NS_GESTURENOTIFY_EVENT,
               "Duplicate() must be overridden by sub class");
    
    WidgetGestureNotifyEvent* result =
      new WidgetGestureNotifyEvent(false, message, nullptr);
    result->AssignGestureNotifyEventData(*this, true);
    result->mFlags = mFlags;
    return result;
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
  virtual WidgetSimpleGestureEvent* AsSimpleGestureEvent() MOZ_OVERRIDE
  {
    return this;
  }

  WidgetSimpleGestureEvent(bool aIsTrusted, uint32_t aMessage,
                           nsIWidget* aWidget)
    : WidgetMouseEventBase(aIsTrusted, aMessage, aWidget,
                           NS_SIMPLE_GESTURE_EVENT)
    , allowedDirections(0)
    , direction(0)
    , delta(0.0)
    , clickCount(0)
  {
  }

  WidgetSimpleGestureEvent(const WidgetSimpleGestureEvent& aOther)
    : WidgetMouseEventBase(aOther.mFlags.mIsTrusted, aOther.message,
                           aOther.widget, NS_SIMPLE_GESTURE_EVENT)
    , allowedDirections(aOther.allowedDirections)
    , direction(aOther.direction)
    , delta(aOther.delta)
    , clickCount(0)
  {
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    MOZ_ASSERT(mClass == NS_SIMPLE_GESTURE_EVENT,
               "Duplicate() must be overridden by sub class");
    
    WidgetSimpleGestureEvent* result =
      new WidgetSimpleGestureEvent(false, message, nullptr);
    result->AssignSimpleGestureEventData(*this, true);
    result->mFlags = mFlags;
    return result;
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
  typedef nsTArray<nsRefPtr<mozilla::dom::Touch>> TouchArray;
  typedef nsAutoTArray<nsRefPtr<mozilla::dom::Touch>, 10> AutoTouchArray;

  virtual WidgetTouchEvent* AsTouchEvent() MOZ_OVERRIDE { return this; }

  WidgetTouchEvent()
  {
  }

  WidgetTouchEvent(const WidgetTouchEvent& aOther) :
    WidgetInputEvent(aOther.mFlags.mIsTrusted, aOther.message, aOther.widget,
                     NS_TOUCH_EVENT)
  {
    modifiers = aOther.modifiers;
    time = aOther.time;
    timeStamp = aOther.timeStamp;
    touches.AppendElements(aOther.touches);
    mFlags.mCancelable = message != NS_TOUCH_CANCEL;
    MOZ_COUNT_CTOR(WidgetTouchEvent);
  }

  WidgetTouchEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget) :
    WidgetInputEvent(aIsTrusted, aMessage, aWidget, NS_TOUCH_EVENT)
  {
    MOZ_COUNT_CTOR(WidgetTouchEvent);
    mFlags.mCancelable = message != NS_TOUCH_CANCEL;
  }

  virtual ~WidgetTouchEvent()
  {
    MOZ_COUNT_DTOR(WidgetTouchEvent);
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    MOZ_ASSERT(mClass == NS_TOUCH_EVENT,
               "Duplicate() must be overridden by sub class");
    
    WidgetTouchEvent* result = new WidgetTouchEvent(false, message, nullptr);
    result->AssignTouchEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  TouchArray touches;

  void AssignTouchEventData(const WidgetTouchEvent& aEvent, bool aCopyTargets)
  {
    AssignInputEventData(aEvent, aCopyTargets);

    
    MOZ_ASSERT(touches.IsEmpty());
    touches.AppendElements(aEvent.touches);
  }
};

} 

#endif 

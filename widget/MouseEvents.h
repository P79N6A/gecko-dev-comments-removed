




#ifndef mozilla_MouseEvents_h__
#define mozilla_MouseEvents_h__

#include <stdint.h>

#include "mozilla/BasicEvents.h"
#include "mozilla/MathAlgorithms.h"
#include "nsCOMPtr.h"
#include "nsIDOMDataTransfer.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMWheelEvent.h"





enum nsDragDropEventStatus
{  
  
  nsDragDropEventStatus_eDragEntered,
  
  nsDragDropEventStatus_eDragExited,
  
  nsDragDropEventStatus_eDrop
};

namespace mozilla {

namespace dom {
  class PBrowserParent;
  class PBrowserChild;
} 





class WidgetMouseEventBase : public WidgetInputEvent
{
private:
  friend class dom::PBrowserParent;
  friend class dom::PBrowserChild;

protected:
  WidgetMouseEventBase()
  {
  }

  WidgetMouseEventBase(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget,
                       nsEventStructType aStructType) :
    WidgetInputEvent(aIsTrusted, aMessage, aWidget, aStructType),
    button(0), buttons(0), pressure(0),
    inputSource(nsIDOMMouseEvent::MOZ_SOURCE_MOUSE)
 {
 }

public:
  virtual WidgetMouseEventBase* AsMouseEventBase() MOZ_OVERRIDE { return this; }

  
  nsCOMPtr<nsISupports> relatedTarget;

  enum buttonType
  {
    eLeftButton   = 0,
    eMiddleButton = 1,
    eRightButton  = 2
  };
  
  
  int16_t button;

  enum buttonsFlag {
    eLeftButtonFlag   = 0x01,
    eRightButtonFlag  = 0x02,
    eMiddleButtonFlag = 0x04,
    
    
    e4thButtonFlag    = 0x08,
    
    
    e5thButtonFlag    = 0x10
  };

  
  
  int16_t buttons;

  
  float pressure;

  
  uint16_t inputSource;

  void AssignMouseEventBaseData(const WidgetMouseEventBase& aEvent,
                                bool aCopyTargets)
  {
    AssignInputEventData(aEvent, aCopyTargets);

    relatedTarget = aCopyTargets ? aEvent.relatedTarget : nullptr;
    button = aEvent.button;
    buttons = aEvent.buttons;
    pressure = aEvent.pressure;
    inputSource = aEvent.inputSource;
  }

  


  bool IsLeftClickEvent() const
  {
    return message == NS_MOUSE_CLICK && button == eLeftButton;
  }
};





class WidgetMouseEvent : public WidgetMouseEventBase
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

public:
  enum reasonType
  {
    eReal,
    eSynthesized
  };

  enum contextType
  {
    eNormal,
    eContextMenuKey
  };

  enum exitType
  {
    eChild,
    eTopLevel
  };

protected:
  WidgetMouseEvent()
  {
  }

  WidgetMouseEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget,
                   nsEventStructType aStructType, reasonType aReason) :
    WidgetMouseEventBase(aIsTrusted, aMessage, aWidget, aStructType),
    acceptActivation(false), ignoreRootScrollFrame(false),
    reason(aReason), context(eNormal), exit(eChild), clickCount(0)
  {
    switch (aMessage) {
      case NS_MOUSE_MOVE:
        mFlags.mCancelable = false;
        break;
      case NS_MOUSEENTER:
      case NS_MOUSELEAVE:
        mFlags.mBubbles = false;
        mFlags.mCancelable = false;
        break;
      default:
        break;
    }
  }

public:
  virtual WidgetMouseEvent* AsMouseEvent() MOZ_OVERRIDE { return this; }

  WidgetMouseEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget,
                   reasonType aReason, contextType aContext = eNormal) :
    WidgetMouseEventBase(aIsTrusted, aMessage, aWidget, NS_MOUSE_EVENT),
    acceptActivation(false), ignoreRootScrollFrame(false),
    reason(aReason), context(aContext), exit(eChild), clickCount(0)
  {
    switch (aMessage) {
      case NS_MOUSE_MOVE:
        mFlags.mCancelable = false;
        break;
      case NS_MOUSEENTER:
      case NS_MOUSELEAVE:
        mFlags.mBubbles = false;
        mFlags.mCancelable = false;
        break;
      case NS_CONTEXTMENU:
        button = (context == eNormal) ? eRightButton : eLeftButton;
        break;
      default:
        break;
    }
  }

#ifdef DEBUG
  virtual ~WidgetMouseEvent()
  {
    NS_WARN_IF_FALSE(message != NS_CONTEXTMENU ||
                     button ==
                       ((context == eNormal) ? eRightButton : eLeftButton),
                     "Wrong button set to NS_CONTEXTMENU event?");
  }
#endif

  
  
  bool acceptActivation;
  
  bool ignoreRootScrollFrame;

  reasonType reason : 4;
  contextType context : 4;
  exitType exit;

  
  uint32_t clickCount;

  void AssignMouseEventData(const WidgetMouseEvent& aEvent, bool aCopyTargets)
  {
    AssignMouseEventBaseData(aEvent, aCopyTargets);

    acceptActivation = aEvent.acceptActivation;
    ignoreRootScrollFrame = aEvent.ignoreRootScrollFrame;
    clickCount = aEvent.clickCount;
  }

  


  bool IsContextMenuKeyEvent() const
  {
    return message == NS_CONTEXTMENU && context == eContextMenuKey;
  }
};





class WidgetDragEvent : public WidgetMouseEvent
{
public:
  virtual WidgetDragEvent* AsDragEvent() MOZ_OVERRIDE { return this; }

  WidgetDragEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget) :
    WidgetMouseEvent(aIsTrusted, aMessage, aWidget, NS_DRAG_EVENT, eReal),
    userCancelled(false)
  {
    mFlags.mCancelable =
      (aMessage != NS_DRAGDROP_EXIT_SYNTH &&
       aMessage != NS_DRAGDROP_LEAVE_SYNTH &&
       aMessage != NS_DRAGDROP_END);
  }

  
  nsCOMPtr<nsIDOMDataTransfer> dataTransfer;

  
  bool userCancelled;

  
  void AssignDragEventData(const WidgetDragEvent& aEvent, bool aCopyTargets)
  {
    AssignMouseEventData(aEvent, aCopyTargets);

    dataTransfer = aEvent.dataTransfer;
    
    userCancelled = false;
  }
};









class WidgetMouseScrollEvent : public WidgetMouseEventBase
{
private:
  WidgetMouseScrollEvent()
  {
  }

public:
  virtual WidgetMouseScrollEvent* AsMouseScrollEvent() MOZ_OVERRIDE
  {
    return this;
  }

  WidgetMouseScrollEvent(bool aIsTrusted, uint32_t aMessage,
                         nsIWidget* aWidget) :
    WidgetMouseEventBase(aIsTrusted, aMessage, aWidget, NS_MOUSE_SCROLL_EVENT),
    delta(0), isHorizontal(false)
  {
  }

  
  
  
  
  
  
  int32_t delta;

  
  
  bool isHorizontal;

  void AssignMouseScrollEventData(const WidgetMouseScrollEvent& aEvent,
                                  bool aCopyTargets)
  {
    AssignMouseEventBaseData(aEvent, aCopyTargets);

    delta = aEvent.delta;
    isHorizontal = aEvent.isHorizontal;
  }
};





class WidgetWheelEvent : public WidgetMouseEventBase
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  WidgetWheelEvent()
  {
  }

public:
  virtual WidgetWheelEvent* AsWheelEvent() MOZ_OVERRIDE { return this; }

  WidgetWheelEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget) :
    WidgetMouseEventBase(aIsTrusted, aMessage, aWidget, NS_WHEEL_EVENT),
    deltaX(0.0), deltaY(0.0), deltaZ(0.0),
    deltaMode(nsIDOMWheelEvent::DOM_DELTA_PIXEL),
    customizedByUserPrefs(false), isMomentum(false), isPixelOnlyDevice(false),
    lineOrPageDeltaX(0), lineOrPageDeltaY(0), scrollType(SCROLL_DEFAULT),
    overflowDeltaX(0.0), overflowDeltaY(0.0),
    mViewPortIsOverscrolled(false)
  {
  }

  
  
  
  
  double deltaX;
  double deltaY;
  double deltaZ;

  
  uint32_t deltaMode;

  

  
  
  bool customizedByUserPrefs;

  
  bool isMomentum;

  
  
  
  
  bool isPixelOnlyDevice;

  
  
  
  int32_t lineOrPageDeltaX;
  int32_t lineOrPageDeltaY;

  
  
  int32_t GetPreferredIntDelta()
  {
    if (!lineOrPageDeltaX && !lineOrPageDeltaY) {
      return 0;
    }
    if (lineOrPageDeltaY && !lineOrPageDeltaX) {
      return lineOrPageDeltaY;
    }
    if (lineOrPageDeltaX && !lineOrPageDeltaY) {
      return lineOrPageDeltaX;
    }
    if ((lineOrPageDeltaX < 0 && lineOrPageDeltaY > 0) ||
        (lineOrPageDeltaX > 0 && lineOrPageDeltaY < 0)) {
      return 0; 
    }
    return (Abs(lineOrPageDeltaX) > Abs(lineOrPageDeltaY)) ?
             lineOrPageDeltaX : lineOrPageDeltaY;
  }

  
  
  
  enum ScrollType
  {
    SCROLL_DEFAULT,
    SCROLL_SYNCHRONOUSLY,
    SCROLL_ASYNCHRONOUSELY,
    SCROLL_SMOOTHLY
  };
  ScrollType scrollType;

  
  
  
  
  
  
  
  
  
  double overflowDeltaX;
  double overflowDeltaY;

  
  
  
  
  bool mViewPortIsOverscrolled;

  void AssignWheelEventData(const WidgetWheelEvent& aEvent, bool aCopyTargets)
  {
    AssignMouseEventBaseData(aEvent, aCopyTargets);

    deltaX = aEvent.deltaX;
    deltaY = aEvent.deltaY;
    deltaZ = aEvent.deltaZ;
    deltaMode = aEvent.deltaMode;
    customizedByUserPrefs = aEvent.customizedByUserPrefs;
    isMomentum = aEvent.isMomentum;
    isPixelOnlyDevice = aEvent.isPixelOnlyDevice;
    lineOrPageDeltaX = aEvent.lineOrPageDeltaX;
    lineOrPageDeltaY = aEvent.lineOrPageDeltaY;
    scrollType = aEvent.scrollType;
    overflowDeltaX = aEvent.overflowDeltaX;
    overflowDeltaY = aEvent.overflowDeltaY;
    mViewPortIsOverscrolled = aEvent.mViewPortIsOverscrolled;
  }
};

} 

#endif

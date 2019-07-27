




#ifndef mozilla_MouseEvents_h__
#define mozilla_MouseEvents_h__

#include <stdint.h>

#include "mozilla/BasicEvents.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/dom/DataTransfer.h"
#include "nsCOMPtr.h"
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





class WidgetPointerHelper
{
public:
  bool convertToPointer;
  uint32_t pointerId;
  uint32_t tiltX;
  uint32_t tiltY;
  bool retargetedByPointerCapture;

  WidgetPointerHelper() : convertToPointer(true), pointerId(0), tiltX(0), tiltY(0),
                          retargetedByPointerCapture(false) {}

  void AssignPointerHelperData(const WidgetPointerHelper& aEvent)
  {
    convertToPointer = aEvent.convertToPointer;
    pointerId = aEvent.pointerId;
    tiltX = aEvent.tiltX;
    tiltY = aEvent.tiltY;
    retargetedByPointerCapture = aEvent.retargetedByPointerCapture;
  }
};





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
                       EventClassID aEventClassID)
    : WidgetInputEvent(aIsTrusted, aMessage, aWidget, aEventClassID)
    , button(0)
    , buttons(0)
    , pressure(0)
    , hitCluster(false)
    , inputSource(nsIDOMMouseEvent::MOZ_SOURCE_MOUSE)
 {
 }

public:
  virtual WidgetMouseEventBase* AsMouseEventBase() override { return this; }

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_CRASH("WidgetMouseEventBase must not be most-subclass");
    return nullptr;
  }

  
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
  
  bool hitCluster;

  
  uint16_t inputSource;

  
  nsString region;

  void AssignMouseEventBaseData(const WidgetMouseEventBase& aEvent,
                                bool aCopyTargets)
  {
    AssignInputEventData(aEvent, aCopyTargets);

    relatedTarget = aCopyTargets ? aEvent.relatedTarget : nullptr;
    button = aEvent.button;
    buttons = aEvent.buttons;
    pressure = aEvent.pressure;
    hitCluster = aEvent.hitCluster;
    inputSource = aEvent.inputSource;
  }

  


  bool IsLeftClickEvent() const
  {
    return message == NS_MOUSE_CLICK && button == eLeftButton;
  }
};





class WidgetMouseEvent : public WidgetMouseEventBase, public WidgetPointerHelper
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
                   EventClassID aEventClassID, reasonType aReason)
    : WidgetMouseEventBase(aIsTrusted, aMessage, aWidget, aEventClassID)
    , acceptActivation(false)
    , ignoreRootScrollFrame(false)
    , reason(aReason)
    , context(eNormal)
    , exit(eChild)
    , clickCount(0)
  {
    switch (aMessage) {
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
  virtual WidgetMouseEvent* AsMouseEvent() override { return this; }

  WidgetMouseEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget,
                   reasonType aReason, contextType aContext = eNormal) :
    WidgetMouseEventBase(aIsTrusted, aMessage, aWidget, eMouseEventClass),
    acceptActivation(false), ignoreRootScrollFrame(false),
    reason(aReason), context(aContext), exit(eChild), clickCount(0)
  {
    switch (aMessage) {
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

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_ASSERT(mClass == eMouseEventClass,
               "Duplicate() must be overridden by sub class");
    
    WidgetMouseEvent* result =
      new WidgetMouseEvent(false, message, nullptr, reason, context);
    result->AssignMouseEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  
  
  bool acceptActivation;
  
  bool ignoreRootScrollFrame;

  reasonType reason : 4;
  contextType context : 4;
  exitType exit;

  
  uint32_t clickCount;

  void AssignMouseEventData(const WidgetMouseEvent& aEvent, bool aCopyTargets)
  {
    AssignMouseEventBaseData(aEvent, aCopyTargets);
    AssignPointerHelperData(aEvent);

    acceptActivation = aEvent.acceptActivation;
    ignoreRootScrollFrame = aEvent.ignoreRootScrollFrame;
    clickCount = aEvent.clickCount;
  }

  


  bool IsContextMenuKeyEvent() const
  {
    return message == NS_CONTEXTMENU && context == eContextMenuKey;
  }

  



  bool IsReal() const
  {
    return reason == eReal;
  }
};





class WidgetDragEvent : public WidgetMouseEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;
protected:
  WidgetDragEvent()
  {
  }
public:
  virtual WidgetDragEvent* AsDragEvent() override { return this; }

  WidgetDragEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget)
    : WidgetMouseEvent(aIsTrusted, aMessage, aWidget, eDragEventClass, eReal)
    , userCancelled(false)
    , mDefaultPreventedOnContent(false)
  {
    mFlags.mCancelable =
      (aMessage != NS_DRAGDROP_EXIT_SYNTH &&
       aMessage != NS_DRAGDROP_LEAVE_SYNTH &&
       aMessage != NS_DRAGDROP_END);
  }

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_ASSERT(mClass == eDragEventClass,
               "Duplicate() must be overridden by sub class");
    
    WidgetDragEvent* result = new WidgetDragEvent(false, message, nullptr);
    result->AssignDragEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  
  nsCOMPtr<dom::DataTransfer> dataTransfer;

  
  bool userCancelled;
  
  bool mDefaultPreventedOnContent;

  
  void AssignDragEventData(const WidgetDragEvent& aEvent, bool aCopyTargets)
  {
    AssignMouseEventData(aEvent, aCopyTargets);

    dataTransfer = aEvent.dataTransfer;
    
    userCancelled = false;
    mDefaultPreventedOnContent = aEvent.mDefaultPreventedOnContent;
  }
};









class WidgetMouseScrollEvent : public WidgetMouseEventBase
{
private:
  WidgetMouseScrollEvent()
  {
  }

public:
  virtual WidgetMouseScrollEvent* AsMouseScrollEvent() override
  {
    return this;
  }

  WidgetMouseScrollEvent(bool aIsTrusted, uint32_t aMessage,
                         nsIWidget* aWidget)
    : WidgetMouseEventBase(aIsTrusted, aMessage, aWidget,
                           eMouseScrollEventClass)
    , delta(0)
    , isHorizontal(false)
  {
  }

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_ASSERT(mClass == eMouseScrollEventClass,
               "Duplicate() must be overridden by sub class");
    
    WidgetMouseScrollEvent* result =
      new WidgetMouseScrollEvent(false, message, nullptr);
    result->AssignMouseScrollEventData(*this, true);
    result->mFlags = mFlags;
    return result;
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
  virtual WidgetWheelEvent* AsWheelEvent() override { return this; }

  WidgetWheelEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget)
    : WidgetMouseEventBase(aIsTrusted, aMessage, aWidget, eWheelEventClass)
    , deltaX(0.0)
    , deltaY(0.0)
    , deltaZ(0.0)
    , deltaMode(nsIDOMWheelEvent::DOM_DELTA_PIXEL)
    , customizedByUserPrefs(false)
    , isMomentum(false)
    , mIsNoLineOrPageDelta(false)
    , lineOrPageDeltaX(0)
    , lineOrPageDeltaY(0)
    , scrollType(SCROLL_DEFAULT)
    , overflowDeltaX(0.0)
    , overflowDeltaY(0.0)
    , mViewPortIsOverscrolled(false)
  {
  }

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_ASSERT(mClass == eWheelEventClass,
               "Duplicate() must be overridden by sub class");
    
    WidgetWheelEvent* result = new WidgetWheelEvent(false, message, nullptr);
    result->AssignWheelEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  
  
  
  
  double deltaX;
  double deltaY;
  double deltaZ;

  
  uint32_t deltaMode;

  

  
  
  bool customizedByUserPrefs;

  
  bool isMomentum;

  
  
  
  
  bool mIsNoLineOrPageDelta;

  
  
  
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
    mIsNoLineOrPageDelta = aEvent.mIsNoLineOrPageDelta;
    lineOrPageDeltaX = aEvent.lineOrPageDeltaX;
    lineOrPageDeltaY = aEvent.lineOrPageDeltaY;
    scrollType = aEvent.scrollType;
    overflowDeltaX = aEvent.overflowDeltaX;
    overflowDeltaY = aEvent.overflowDeltaY;
    mViewPortIsOverscrolled = aEvent.mViewPortIsOverscrolled;
  }
};





class WidgetPointerEvent : public WidgetMouseEvent
{
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  WidgetPointerEvent()
  {
  }

public:
  virtual WidgetPointerEvent* AsPointerEvent() override { return this; }

  WidgetPointerEvent(bool aIsTrusted, uint32_t aMsg, nsIWidget* w)
    : WidgetMouseEvent(aIsTrusted, aMsg, w, ePointerEventClass, eReal)
    , width(0)
    , height(0)
    , isPrimary(true)
  {
    UpdateFlags();
  }

  explicit WidgetPointerEvent(const WidgetMouseEvent& aEvent)
    : WidgetMouseEvent(aEvent)
    , width(0)
    , height(0)
    , isPrimary(true)
  {
    mClass = ePointerEventClass;
    UpdateFlags();
  }

  void UpdateFlags()
  {
    switch (message) {
      case NS_POINTER_ENTER:
      case NS_POINTER_LEAVE:
        mFlags.mBubbles = false;
        mFlags.mCancelable = false;
        break;
      case NS_POINTER_CANCEL:
      case NS_POINTER_GOT_CAPTURE:
      case NS_POINTER_LOST_CAPTURE:
        mFlags.mCancelable = false;
        break;
      default:
        break;
    }
  }

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_ASSERT(mClass == ePointerEventClass,
               "Duplicate() must be overridden by sub class");
    
    WidgetPointerEvent* result =
      new WidgetPointerEvent(false, message, nullptr);
    result->AssignPointerEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  uint32_t width;
  uint32_t height;
  bool isPrimary;

  
  void AssignPointerEventData(const WidgetPointerEvent& aEvent,
                              bool aCopyTargets)
  {
    AssignMouseEventData(aEvent, aCopyTargets);

    width = aEvent.width;
    height = aEvent.height;
    isPrimary = aEvent.isPrimary;
  }
};

} 

#endif 






#ifndef nsGUIEvent_h__
#define nsGUIEvent_h__

#include "mozilla/MathAlgorithms.h"

#include "nsCOMArray.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsRegion.h"
#include "nsEvent.h"
#include "nsStringGlue.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMWheelEvent.h"
#include "nsIDOMDataTransfer.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMTouchEvent.h"
#include "nsWeakPtr.h"
#include "nsIWidget.h"
#include "nsTArray.h"
#include "nsTraceRefcnt.h"
#include "nsITransferable.h"
#include "nsIVariant.h"
#include "nsStyleConsts.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {
  class PBrowserParent;
  class PBrowserChild;
}
namespace plugins {
  class PPluginInstanceChild;
}
}

class nsRenderingContext;
class nsIMenuItem;
class nsIContent;
class nsIURI;
class nsHashKey;




enum nsEventStructType {
  
  NS_EVENT,                          
  NS_GUI_EVENT,                      
  NS_INPUT_EVENT,                    

  
  NS_MOUSE_EVENT,                    
  NS_MOUSE_SCROLL_EVENT,             
  NS_DRAG_EVENT,                     
  NS_WHEEL_EVENT,                    

  
  NS_GESTURENOTIFY_EVENT,            
  NS_SIMPLE_GESTURE_EVENT,           
  NS_TOUCH_EVENT,                    

  
  NS_KEY_EVENT,                      
  NS_COMPOSITION_EVENT,              
  NS_TEXT_EVENT,                     

  
  NS_QUERY_CONTENT_EVENT,            
  NS_SELECTION_EVENT,                

  
  NS_SCROLLBAR_EVENT,                
  NS_SCROLLPORT_EVENT,               
  NS_SCROLLAREA_EVENT,               

  
  NS_UI_EVENT,                       
  NS_SCRIPT_ERROR_EVENT,             
  NS_MUTATION_EVENT,                 
  NS_FORM_EVENT,                     
  NS_FOCUS_EVENT,                    

  
  NS_SVG_EVENT,                      
  NS_SVGZOOM_EVENT,                  
  NS_SMIL_TIME_EVENT,                

  
  NS_TRANSITION_EVENT,               
  NS_ANIMATION_EVENT,                

  
  NS_COMMAND_EVENT,                  
  NS_CONTENT_COMMAND_EVENT,          

  
  NS_PLUGIN_EVENT                    
};

#define NS_EVENT_TYPE_NULL                   0
#define NS_EVENT_TYPE_ALL                  1 // Not a real event type




 
#define NS_EVENT_NULL                   0


#define NS_WINDOW_START                 100


#define NS_XUL_CLOSE                    (NS_WINDOW_START + 1)

#define NS_KEY_PRESS                    (NS_WINDOW_START + 31)

#define NS_KEY_UP                       (NS_WINDOW_START + 32)

#define NS_KEY_DOWN                     (NS_WINDOW_START + 33)

#define NS_RESIZE_EVENT                 (NS_WINDOW_START + 60)
#define NS_SCROLL_EVENT                 (NS_WINDOW_START + 61)





#define NS_PLUGIN_ACTIVATE               (NS_WINDOW_START + 62)
#define NS_PLUGIN_FOCUS                  (NS_WINDOW_START + 63)

#define NS_OFFLINE                       (NS_WINDOW_START + 64)
#define NS_ONLINE                        (NS_WINDOW_START + 65)


#define NS_BEFORERESIZE_EVENT            (NS_WINDOW_START + 66)


#define NS_MOZ_USER_IDLE                 (NS_WINDOW_START + 67)
#define NS_MOZ_USER_ACTIVE               (NS_WINDOW_START + 68)




#define NS_PLUGIN_RESOLUTION_CHANGED     (NS_WINDOW_START + 69)

#define NS_MOUSE_MESSAGE_START          300
#define NS_MOUSE_MOVE                   (NS_MOUSE_MESSAGE_START)
#define NS_MOUSE_BUTTON_UP              (NS_MOUSE_MESSAGE_START + 1)
#define NS_MOUSE_BUTTON_DOWN            (NS_MOUSE_MESSAGE_START + 2)
#define NS_MOUSE_ENTER                  (NS_MOUSE_MESSAGE_START + 22)
#define NS_MOUSE_EXIT                   (NS_MOUSE_MESSAGE_START + 23)
#define NS_MOUSE_DOUBLECLICK            (NS_MOUSE_MESSAGE_START + 24)
#define NS_MOUSE_CLICK                  (NS_MOUSE_MESSAGE_START + 27)
#define NS_MOUSE_ACTIVATE               (NS_MOUSE_MESSAGE_START + 30)
#define NS_MOUSE_ENTER_SYNTH            (NS_MOUSE_MESSAGE_START + 31)
#define NS_MOUSE_EXIT_SYNTH             (NS_MOUSE_MESSAGE_START + 32)
#define NS_MOUSE_MOZHITTEST             (NS_MOUSE_MESSAGE_START + 33)
#define NS_MOUSEENTER                   (NS_MOUSE_MESSAGE_START + 34)
#define NS_MOUSELEAVE                   (NS_MOUSE_MESSAGE_START + 35)

#define NS_CONTEXTMENU_MESSAGE_START    500
#define NS_CONTEXTMENU                  (NS_CONTEXTMENU_MESSAGE_START)

#define NS_STREAM_EVENT_START           1100
#define NS_LOAD                         (NS_STREAM_EVENT_START)
#define NS_PAGE_UNLOAD                  (NS_STREAM_EVENT_START + 1)
#define NS_HASHCHANGE                   (NS_STREAM_EVENT_START + 2)
#define NS_IMAGE_ABORT                  (NS_STREAM_EVENT_START + 3)
#define NS_LOAD_ERROR                   (NS_STREAM_EVENT_START + 4)
#define NS_POPSTATE                     (NS_STREAM_EVENT_START + 5)
#define NS_BEFORE_PAGE_UNLOAD           (NS_STREAM_EVENT_START + 6)
#define NS_PAGE_RESTORE                 (NS_STREAM_EVENT_START + 7)
#define NS_READYSTATECHANGE             (NS_STREAM_EVENT_START + 8)
 
#define NS_FORM_EVENT_START             1200
#define NS_FORM_SUBMIT                  (NS_FORM_EVENT_START)
#define NS_FORM_RESET                   (NS_FORM_EVENT_START + 1)
#define NS_FORM_CHANGE                  (NS_FORM_EVENT_START + 2)
#define NS_FORM_SELECTED                (NS_FORM_EVENT_START + 3)
#define NS_FORM_INPUT                   (NS_FORM_EVENT_START + 4)
#define NS_FORM_INVALID                 (NS_FORM_EVENT_START + 5)


#define NS_FOCUS_EVENT_START            1300
#define NS_FOCUS_CONTENT                (NS_FOCUS_EVENT_START)
#define NS_BLUR_CONTENT                 (NS_FOCUS_EVENT_START + 1)

#define NS_DRAGDROP_EVENT_START         1400
#define NS_DRAGDROP_ENTER               (NS_DRAGDROP_EVENT_START)
#define NS_DRAGDROP_OVER                (NS_DRAGDROP_EVENT_START + 1)
#define NS_DRAGDROP_EXIT                (NS_DRAGDROP_EVENT_START + 2)
#define NS_DRAGDROP_DRAGDROP            (NS_DRAGDROP_EVENT_START + 3)
#define NS_DRAGDROP_GESTURE             (NS_DRAGDROP_EVENT_START + 4)
#define NS_DRAGDROP_DRAG                (NS_DRAGDROP_EVENT_START + 5)
#define NS_DRAGDROP_END                 (NS_DRAGDROP_EVENT_START + 6)
#define NS_DRAGDROP_START               (NS_DRAGDROP_EVENT_START + 7)
#define NS_DRAGDROP_DROP                (NS_DRAGDROP_EVENT_START + 8)
#define NS_DRAGDROP_OVER_SYNTH          (NS_DRAGDROP_EVENT_START + 1)
#define NS_DRAGDROP_EXIT_SYNTH          (NS_DRAGDROP_EVENT_START + 2)
#define NS_DRAGDROP_LEAVE_SYNTH         (NS_DRAGDROP_EVENT_START + 9)


#define NS_XUL_EVENT_START            1500
#define NS_XUL_POPUP_SHOWING          (NS_XUL_EVENT_START)
#define NS_XUL_POPUP_SHOWN            (NS_XUL_EVENT_START+1)
#define NS_XUL_POPUP_HIDING           (NS_XUL_EVENT_START+2)
#define NS_XUL_POPUP_HIDDEN           (NS_XUL_EVENT_START+3)

#define NS_XUL_BROADCAST              (NS_XUL_EVENT_START+5)
#define NS_XUL_COMMAND_UPDATE         (NS_XUL_EVENT_START+6)



#define NS_MOUSE_SCROLL_START         1600
#define NS_MOUSE_SCROLL               (NS_MOUSE_SCROLL_START)
#define NS_MOUSE_PIXEL_SCROLL         (NS_MOUSE_SCROLL_START + 1)

#define NS_SCROLLPORT_START           1700
#define NS_SCROLLPORT_UNDERFLOW       (NS_SCROLLPORT_START)
#define NS_SCROLLPORT_OVERFLOW        (NS_SCROLLPORT_START+1)



#define NS_USER_DEFINED_EVENT         2000
 

#define NS_COMPOSITION_EVENT_START    2200
#define NS_COMPOSITION_START          (NS_COMPOSITION_EVENT_START)
#define NS_COMPOSITION_END            (NS_COMPOSITION_EVENT_START + 1)
#define NS_COMPOSITION_UPDATE         (NS_COMPOSITION_EVENT_START + 2)


#define NS_TEXT_START                 2400
#define NS_TEXT_TEXT                  (NS_TEXT_START)


#define NS_UI_EVENT_START          2500

#define NS_UI_ACTIVATE             (NS_UI_EVENT_START)
#define NS_UI_FOCUSIN              (NS_UI_EVENT_START + 1)
#define NS_UI_FOCUSOUT             (NS_UI_EVENT_START + 2)


#define NS_PAGETRANSITION_START    2700
#define NS_PAGE_SHOW               (NS_PAGETRANSITION_START + 1)
#define NS_PAGE_HIDE               (NS_PAGETRANSITION_START + 2)


#define NS_SVG_EVENT_START              2800
#define NS_SVG_LOAD                     (NS_SVG_EVENT_START)
#define NS_SVG_UNLOAD                   (NS_SVG_EVENT_START + 1)
#define NS_SVG_ABORT                    (NS_SVG_EVENT_START + 2)
#define NS_SVG_ERROR                    (NS_SVG_EVENT_START + 3)
#define NS_SVG_RESIZE                   (NS_SVG_EVENT_START + 4)
#define NS_SVG_SCROLL                   (NS_SVG_EVENT_START + 5)


#define NS_SVGZOOM_EVENT_START          2900
#define NS_SVG_ZOOM                     (NS_SVGZOOM_EVENT_START)


#define NS_XULCOMMAND_EVENT_START       3000
#define NS_XUL_COMMAND                  (NS_XULCOMMAND_EVENT_START)


#define NS_CUTCOPYPASTE_EVENT_START     3100
#define NS_COPY             (NS_CUTCOPYPASTE_EVENT_START)
#define NS_CUT              (NS_CUTCOPYPASTE_EVENT_START + 1)
#define NS_PASTE            (NS_CUTCOPYPASTE_EVENT_START + 2)


#define NS_QUERY_CONTENT_EVENT_START    3200


#define NS_QUERY_SELECTED_TEXT          (NS_QUERY_CONTENT_EVENT_START)



#define NS_QUERY_TEXT_CONTENT           (NS_QUERY_CONTENT_EVENT_START + 1)


#define NS_QUERY_CARET_RECT             (NS_QUERY_CONTENT_EVENT_START + 3)



#define NS_QUERY_TEXT_RECT              (NS_QUERY_CONTENT_EVENT_START + 4)


#define NS_QUERY_EDITOR_RECT            (NS_QUERY_CONTENT_EVENT_START + 5)


#define NS_QUERY_CONTENT_STATE          (NS_QUERY_CONTENT_EVENT_START + 6)

#define NS_QUERY_SELECTION_AS_TRANSFERABLE (NS_QUERY_CONTENT_EVENT_START + 7)


#define NS_QUERY_CHARACTER_AT_POINT     (NS_QUERY_CONTENT_EVENT_START + 8)


#define NS_QUERY_DOM_WIDGET_HITTEST     (NS_QUERY_CONTENT_EVENT_START + 9)


#define NS_MEDIA_EVENT_START            3300
#define NS_LOADSTART           (NS_MEDIA_EVENT_START)
#define NS_PROGRESS            (NS_MEDIA_EVENT_START+1)
#define NS_SUSPEND             (NS_MEDIA_EVENT_START+2)
#define NS_EMPTIED             (NS_MEDIA_EVENT_START+3)
#define NS_STALLED             (NS_MEDIA_EVENT_START+4)
#define NS_PLAY                (NS_MEDIA_EVENT_START+5)
#define NS_PAUSE               (NS_MEDIA_EVENT_START+6)
#define NS_LOADEDMETADATA      (NS_MEDIA_EVENT_START+7)
#define NS_LOADEDDATA          (NS_MEDIA_EVENT_START+8)
#define NS_WAITING             (NS_MEDIA_EVENT_START+9)
#define NS_PLAYING             (NS_MEDIA_EVENT_START+10)
#define NS_CANPLAY             (NS_MEDIA_EVENT_START+11)
#define NS_CANPLAYTHROUGH      (NS_MEDIA_EVENT_START+12)
#define NS_SEEKING             (NS_MEDIA_EVENT_START+13)
#define NS_SEEKED              (NS_MEDIA_EVENT_START+14)
#define NS_TIMEUPDATE          (NS_MEDIA_EVENT_START+15)
#define NS_ENDED               (NS_MEDIA_EVENT_START+16)
#define NS_RATECHANGE          (NS_MEDIA_EVENT_START+17)
#define NS_DURATIONCHANGE      (NS_MEDIA_EVENT_START+18)
#define NS_VOLUMECHANGE        (NS_MEDIA_EVENT_START+19)
#define NS_MOZAUDIOAVAILABLE   (NS_MEDIA_EVENT_START+20)


#define NS_NOTIFYPAINT_START    3400
#define NS_AFTERPAINT           (NS_NOTIFYPAINT_START)


#define NS_SIMPLE_GESTURE_EVENT_START    3500
#define NS_SIMPLE_GESTURE_SWIPE          (NS_SIMPLE_GESTURE_EVENT_START)
#define NS_SIMPLE_GESTURE_MAGNIFY_START  (NS_SIMPLE_GESTURE_EVENT_START+1)
#define NS_SIMPLE_GESTURE_MAGNIFY_UPDATE (NS_SIMPLE_GESTURE_EVENT_START+2)
#define NS_SIMPLE_GESTURE_MAGNIFY        (NS_SIMPLE_GESTURE_EVENT_START+3)
#define NS_SIMPLE_GESTURE_ROTATE_START   (NS_SIMPLE_GESTURE_EVENT_START+4)
#define NS_SIMPLE_GESTURE_ROTATE_UPDATE  (NS_SIMPLE_GESTURE_EVENT_START+5)
#define NS_SIMPLE_GESTURE_ROTATE         (NS_SIMPLE_GESTURE_EVENT_START+6)
#define NS_SIMPLE_GESTURE_TAP            (NS_SIMPLE_GESTURE_EVENT_START+7)
#define NS_SIMPLE_GESTURE_PRESSTAP       (NS_SIMPLE_GESTURE_EVENT_START+8)
#define NS_SIMPLE_GESTURE_EDGEUI         (NS_SIMPLE_GESTURE_EVENT_START+9)


#define NS_PLUGIN_EVENT_START            3600
#define NS_PLUGIN_INPUT_EVENT            (NS_PLUGIN_EVENT_START)
#define NS_PLUGIN_FOCUS_EVENT            (NS_PLUGIN_EVENT_START+1)


#define NS_SELECTION_EVENT_START        3700

#define NS_SELECTION_SET                (NS_SELECTION_EVENT_START)


#define NS_CONTENT_COMMAND_EVENT_START  3800
#define NS_CONTENT_COMMAND_CUT          (NS_CONTENT_COMMAND_EVENT_START)
#define NS_CONTENT_COMMAND_COPY         (NS_CONTENT_COMMAND_EVENT_START+1)
#define NS_CONTENT_COMMAND_PASTE        (NS_CONTENT_COMMAND_EVENT_START+2)
#define NS_CONTENT_COMMAND_DELETE       (NS_CONTENT_COMMAND_EVENT_START+3)
#define NS_CONTENT_COMMAND_UNDO         (NS_CONTENT_COMMAND_EVENT_START+4)
#define NS_CONTENT_COMMAND_REDO         (NS_CONTENT_COMMAND_EVENT_START+5)
#define NS_CONTENT_COMMAND_PASTE_TRANSFERABLE (NS_CONTENT_COMMAND_EVENT_START+6)






#define NS_CONTENT_COMMAND_SCROLL       (NS_CONTENT_COMMAND_EVENT_START+7)


#define NS_GESTURENOTIFY_EVENT_START 3900

#define NS_ORIENTATION_EVENT         4000

#define NS_SCROLLAREA_EVENT_START    4100
#define NS_SCROLLEDAREACHANGED       (NS_SCROLLAREA_EVENT_START)

#define NS_TRANSITION_EVENT_START    4200
#define NS_TRANSITION_END            (NS_TRANSITION_EVENT_START)

#define NS_ANIMATION_EVENT_START     4250
#define NS_ANIMATION_START           (NS_ANIMATION_EVENT_START)
#define NS_ANIMATION_END             (NS_ANIMATION_EVENT_START + 1)
#define NS_ANIMATION_ITERATION       (NS_ANIMATION_EVENT_START + 2)

#define NS_SMIL_TIME_EVENT_START     4300
#define NS_SMIL_BEGIN                (NS_SMIL_TIME_EVENT_START)
#define NS_SMIL_END                  (NS_SMIL_TIME_EVENT_START + 1)
#define NS_SMIL_REPEAT               (NS_SMIL_TIME_EVENT_START + 2)


#define NS_NOTIFYSCRIPT_START        4500
#define NS_BEFORE_SCRIPT_EXECUTE     (NS_NOTIFYSCRIPT_START)
#define NS_AFTER_SCRIPT_EXECUTE      (NS_NOTIFYSCRIPT_START+1)

#define NS_PRINT_EVENT_START         4600
#define NS_BEFOREPRINT               (NS_PRINT_EVENT_START)
#define NS_AFTERPRINT                (NS_PRINT_EVENT_START + 1)

#define NS_MESSAGE_EVENT_START       4700
#define NS_MESSAGE                   (NS_MESSAGE_EVENT_START)


#define NS_OPENCLOSE_EVENT_START     4800
#define NS_OPEN                      (NS_OPENCLOSE_EVENT_START)
#define NS_CLOSE                     (NS_OPENCLOSE_EVENT_START+1)


#define NS_DEVICE_ORIENTATION_START  4900
#define NS_DEVICE_ORIENTATION        (NS_DEVICE_ORIENTATION_START)
#define NS_DEVICE_MOTION             (NS_DEVICE_ORIENTATION_START+1)
#define NS_DEVICE_PROXIMITY          (NS_DEVICE_ORIENTATION_START+2)
#define NS_USER_PROXIMITY            (NS_DEVICE_ORIENTATION_START+3)
#define NS_DEVICE_LIGHT              (NS_DEVICE_ORIENTATION_START+4)

#define NS_SHOW_EVENT                5000


#define NS_FULL_SCREEN_START         5100
#define NS_FULLSCREENCHANGE          (NS_FULL_SCREEN_START)
#define NS_FULLSCREENERROR           (NS_FULL_SCREEN_START + 1)

#define NS_TOUCH_EVENT_START         5200
#define NS_TOUCH_START               (NS_TOUCH_EVENT_START)
#define NS_TOUCH_MOVE                (NS_TOUCH_EVENT_START+1)
#define NS_TOUCH_END                 (NS_TOUCH_EVENT_START+2)
#define NS_TOUCH_ENTER               (NS_TOUCH_EVENT_START+3)
#define NS_TOUCH_LEAVE               (NS_TOUCH_EVENT_START+4)
#define NS_TOUCH_CANCEL              (NS_TOUCH_EVENT_START+5)


#define NS_POINTERLOCK_START         5300
#define NS_POINTERLOCKCHANGE         (NS_POINTERLOCK_START)
#define NS_POINTERLOCKERROR          (NS_POINTERLOCK_START + 1)

#define NS_WHEEL_EVENT_START         5400
#define NS_WHEEL_WHEEL               (NS_WHEEL_EVENT_START)


#define NS_MOZ_TIME_CHANGE_EVENT     5500


#define NS_NETWORK_EVENT_START       5600
#define NS_NETWORK_UPLOAD_EVENT      (NS_NETWORK_EVENT_START + 1)
#define NS_NETWORK_DOWNLOAD_EVENT    (NS_NETWORK_EVENT_START + 2)









enum nsWindowZ {
  nsWindowZTop = 0,   
  nsWindowZBottom,    
  nsWindowZRelative   
};

namespace mozilla {
namespace widget {





struct BaseEventFlags
{
public:
  
  
  bool    mIsTrusted : 1;
  
  
  bool    mInBubblingPhase : 1;
  
  bool    mInCapturePhase : 1;
  
  bool    mInSystemGroup: 1;
  
  
  bool    mCancelable : 1;
  
  
  bool    mBubbles : 1;
  
  
  bool    mPropagationStopped : 1;
  
  
  
  bool    mImmediatePropagationStopped : 1;
  
  
  
  bool    mDefaultPrevented : 1;
  
  
  
  bool    mDefaultPreventedByContent : 1;
  
  
  
  
  
  bool    mMultipleActionsPrevented : 1;
  
  
  bool    mIsBeingDispatched : 1;
  
  
  bool    mDispatchedAtLeastOnce : 1;
  
  
  bool    mIsSynthesizedForTests : 1;
  
  
  bool    mExceptionHasBeenRisen : 1;
  
  
  bool    mRetargetToNonNativeAnonymous : 1;
  
  
  bool    mNoCrossProcessBoundaryForwarding : 1;
  
  
  
  
  
  
  
  bool    mNoContentDispatch : 1;
  
  bool    mOnlyChromeDispatch : 1;

  
  inline bool InTargetPhase() const
  {
    return (mInBubblingPhase && mInCapturePhase);
  }

  inline void Clear()
  {
    SetRawFlags(0);
  }
  
  
  
  inline void Union(const BaseEventFlags& aOther)
  {
    RawFlags rawFlags = GetRawFlags() | aOther.GetRawFlags();
    SetRawFlags(rawFlags);
  }

private:
  typedef uint32_t RawFlags;

  inline void SetRawFlags(RawFlags aRawFlags)
  {
    MOZ_STATIC_ASSERT(sizeof(BaseEventFlags) <= sizeof(RawFlags),
      "mozilla::widget::EventFlags must not be bigger than the RawFlags");
    memcpy(this, &aRawFlags, sizeof(BaseEventFlags));
  }
  inline RawFlags GetRawFlags() const
  {
    RawFlags result = 0;
    memcpy(&result, this, sizeof(BaseEventFlags));
    return result;
  }
};

struct EventFlags : public BaseEventFlags
{
  EventFlags()
  {
    Clear();
  }
};

} 
} 





class nsEvent
{
protected:
  nsEvent(bool isTrusted, uint32_t msg, nsEventStructType structType)
    : eventStructType(structType),
      message(msg),
      refPoint(0, 0),
      lastRefPoint(0, 0),
      time(0),
      userType(0)
  {
    MOZ_COUNT_CTOR(nsEvent);
    mFlags.Clear();
    mFlags.mIsTrusted = isTrusted;
    mFlags.mCancelable = true;
    mFlags.mBubbles = true;
  }

  nsEvent()
  {
    MOZ_COUNT_CTOR(nsEvent);
  }

public:
  nsEvent(bool isTrusted, uint32_t msg)
    : eventStructType(NS_EVENT),
      message(msg),
      refPoint(0, 0),
      lastRefPoint(0, 0),
      time(0),
      userType(0)
  {
    MOZ_COUNT_CTOR(nsEvent);
    mFlags.Clear();
    mFlags.mIsTrusted = isTrusted;
    mFlags.mCancelable = true;
    mFlags.mBubbles = true;
  }

  ~nsEvent()
  {
    MOZ_COUNT_DTOR(nsEvent);
  }

  nsEvent(const nsEvent& aOther)
  {
    MOZ_COUNT_CTOR(nsEvent);
    *this = aOther;
  }

  
  nsEventStructType eventStructType;
  
  uint32_t    message;
  
  
  nsIntPoint  refPoint;
  
  nsIntPoint  lastRefPoint;
  
  
  uint64_t    time;
  
  mozilla::widget::BaseEventFlags mFlags;

  
  nsCOMPtr<nsIAtom>     userType;
  
  nsCOMPtr<nsIDOMEventTarget> target;
  nsCOMPtr<nsIDOMEventTarget> currentTarget;
  nsCOMPtr<nsIDOMEventTarget> originalTarget;
};





class nsGUIEvent : public nsEvent
{
protected:
  nsGUIEvent(bool isTrusted, uint32_t msg, nsIWidget *w,
             nsEventStructType structType)
    : nsEvent(isTrusted, msg, structType),
      widget(w), pluginEvent(nullptr)
  {
  }

  nsGUIEvent()
    : pluginEvent(nullptr)
  {
  }

public:
  nsGUIEvent(bool isTrusted, uint32_t msg, nsIWidget *w)
    : nsEvent(isTrusted, msg, NS_GUI_EVENT),
      widget(w), pluginEvent(nullptr)
  {
  }

  
  nsCOMPtr<nsIWidget> widget;

  
  void* pluginEvent;
};





class nsScriptErrorEvent : public nsEvent
{
public:
  nsScriptErrorEvent(bool isTrusted, uint32_t msg)
    : nsEvent(isTrusted, msg, NS_SCRIPT_ERROR_EVENT),
      lineNr(0), errorMsg(nullptr), fileName(nullptr)
  {
  }

  int32_t           lineNr;
  const PRUnichar*  errorMsg;
  const PRUnichar*  fileName;
};





class nsScrollbarEvent : public nsGUIEvent
{
public:
  nsScrollbarEvent(bool isTrusted, uint32_t msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_SCROLLBAR_EVENT),
      position(0)
  {
  }

  
  uint32_t        position; 
};

class nsScrollPortEvent : public nsGUIEvent
{
public:
  enum orientType {
    vertical   = 0,
    horizontal = 1,
    both       = 2
  };

  nsScrollPortEvent(bool isTrusted, uint32_t msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_SCROLLPORT_EVENT),
      orient(vertical)
  {
  }

  orientType orient;
};

class nsScrollAreaEvent : public nsGUIEvent
{
public:
  nsScrollAreaEvent(bool isTrusted, uint32_t msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_SCROLLAREA_EVENT)
  {
  }

  nsRect mArea;
};

class nsInputEvent : public nsGUIEvent
{
protected:
  nsInputEvent(bool isTrusted, uint32_t msg, nsIWidget *w,
               nsEventStructType structType)
    : nsGUIEvent(isTrusted, msg, w, structType),
      modifiers(0)
  {
  }

  nsInputEvent()
  {
  }

public:
  nsInputEvent(bool isTrusted, uint32_t msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_INPUT_EVENT),
      modifiers(0)
  {
  }

  
  bool IsShift() const
  {
    return ((modifiers & mozilla::widget::MODIFIER_SHIFT) != 0);
  }
  
  bool IsControl() const
  {
    return ((modifiers & mozilla::widget::MODIFIER_CONTROL) != 0);
  }
  
  bool IsAlt() const
  {
    return ((modifiers & mozilla::widget::MODIFIER_ALT) != 0);
  }
  
  bool IsMeta() const
  {
    return ((modifiers & mozilla::widget::MODIFIER_META) != 0);
  }
  
  
  bool IsOS() const
  {
    return ((modifiers & mozilla::widget::MODIFIER_OS) != 0);
  }
  
  
  
  bool IsAltGraph() const
  {
    return ((modifiers & mozilla::widget::MODIFIER_ALTGRAPH) != 0);
  }
  
  bool IsCapsLocked() const
  {
    return ((modifiers & mozilla::widget::MODIFIER_CAPSLOCK) != 0);
  }
  
  bool IsNumLocked() const
  {
    return ((modifiers & mozilla::widget::MODIFIER_NUMLOCK) != 0);
  }
  
  bool IsScrollLocked() const
  {
    return ((modifiers & mozilla::widget::MODIFIER_SCROLLLOCK) != 0);
  }

  
  
  bool IsFn() const
  {
    return ((modifiers & mozilla::widget::MODIFIER_FN) != 0);
  }
  
  bool IsSymbolLocked() const
  {
    return ((modifiers & mozilla::widget::MODIFIER_SYMBOLLOCK) != 0);
  }

  void InitBasicModifiers(bool aCtrlKey,
                          bool aAltKey,
                          bool aShiftKey,
                          bool aMetaKey)
  {
    modifiers = 0;
    if (aCtrlKey) {
      modifiers |= mozilla::widget::MODIFIER_CONTROL;
    }
    if (aAltKey) {
      modifiers |= mozilla::widget::MODIFIER_ALT;
    }
    if (aShiftKey) {
      modifiers |= mozilla::widget::MODIFIER_SHIFT;
    }
    if (aMetaKey) {
      modifiers |= mozilla::widget::MODIFIER_META;
    }
  }

  mozilla::widget::Modifiers modifiers;
};





class nsMouseEvent_base : public nsInputEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

public:

  nsMouseEvent_base()
  {
  }

  nsMouseEvent_base(bool isTrusted, uint32_t msg, nsIWidget *w,
                    nsEventStructType type)
    : nsInputEvent(isTrusted, msg, w, type), button(0), buttons(0),
      pressure(0), inputSource(nsIDOMMouseEvent::MOZ_SOURCE_MOUSE) {}

  
  nsCOMPtr<nsISupports> relatedTarget;

  int16_t               button;
  int16_t               buttons;

  
  
  float                 pressure;

  
  uint16_t              inputSource;
};

class nsMouseEvent : public nsMouseEvent_base
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

public:
  enum buttonType  { eLeftButton = 0, eMiddleButton = 1, eRightButton = 2 };
  enum buttonsFlag { eLeftButtonFlag   = 0x01,
                     eRightButtonFlag  = 0x02,
                     eMiddleButtonFlag = 0x04,
                     
                     
                     e4thButtonFlag    = 0x08,
                     
                     
                     e5thButtonFlag    = 0x10 };
  enum reasonType  { eReal, eSynthesized };
  enum contextType { eNormal, eContextMenuKey };
  enum exitType    { eChild, eTopLevel };

  nsMouseEvent()
  {
  }

protected:
  nsMouseEvent(bool isTrusted, uint32_t msg, nsIWidget *w,
               nsEventStructType structType, reasonType aReason)
    : nsMouseEvent_base(isTrusted, msg, w, structType),
      acceptActivation(false), ignoreRootScrollFrame(false),
      reason(aReason), context(eNormal), exit(eChild), clickCount(0)
  {
    switch (msg) {
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

  nsMouseEvent(bool isTrusted, uint32_t msg, nsIWidget *w,
               reasonType aReason, contextType aContext = eNormal)
    : nsMouseEvent_base(isTrusted, msg, w, NS_MOUSE_EVENT),
      acceptActivation(false), ignoreRootScrollFrame(false),
      reason(aReason), context(aContext), exit(eChild), clickCount(0)
  {
    switch (msg) {
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
  ~nsMouseEvent() {
    NS_WARN_IF_FALSE(message != NS_CONTEXTMENU ||
                     button ==
                       ((context == eNormal) ? eRightButton : eLeftButton),
                     "Wrong button set to NS_CONTEXTMENU event?");
  }
#endif

  
  
  bool acceptActivation;
  
  
  bool ignoreRootScrollFrame;

  reasonType   reason : 4;
  contextType  context : 4;
  exitType     exit;

  
  uint32_t     clickCount;
};





class nsDragEvent : public nsMouseEvent
{
public:
  nsDragEvent(bool isTrusted, uint32_t msg, nsIWidget *w)
    : nsMouseEvent(isTrusted, msg, w, NS_DRAG_EVENT, eReal),
      userCancelled(false)
  {
    mFlags.mCancelable =
      (msg != NS_DRAGDROP_EXIT_SYNTH &&
       msg != NS_DRAGDROP_LEAVE_SYNTH &&
       msg != NS_DRAGDROP_END);
  }

  nsCOMPtr<nsIDOMDataTransfer> dataTransfer;
  bool userCancelled;
};





struct nsAlternativeCharCode {
  nsAlternativeCharCode(uint32_t aUnshiftedCharCode,
                        uint32_t aShiftedCharCode) :
    mUnshiftedCharCode(aUnshiftedCharCode), mShiftedCharCode(aShiftedCharCode)
  {
  }
  uint32_t mUnshiftedCharCode;
  uint32_t mShiftedCharCode;
};

class nsKeyEvent : public nsInputEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

public:
  nsKeyEvent()
  {
  }

  nsKeyEvent(bool isTrusted, uint32_t msg, nsIWidget *w)
    : nsInputEvent(isTrusted, msg, w, NS_KEY_EVENT),
      keyCode(0), charCode(0),
      location(nsIDOMKeyEvent::DOM_KEY_LOCATION_STANDARD), isChar(0)
  {
  }

  
  uint32_t        keyCode;   
  
  uint32_t        charCode;
  
  uint32_t        location;
  
  
  nsTArray<nsAlternativeCharCode> alternativeCharCodes;
  
  bool            isChar;
};




 
struct nsTextRangeStyle
{
  enum {
    LINESTYLE_NONE   = NS_STYLE_TEXT_DECORATION_STYLE_NONE,
    LINESTYLE_SOLID  = NS_STYLE_TEXT_DECORATION_STYLE_SOLID,
    LINESTYLE_DOTTED = NS_STYLE_TEXT_DECORATION_STYLE_DOTTED,
    LINESTYLE_DASHED = NS_STYLE_TEXT_DECORATION_STYLE_DASHED,
    LINESTYLE_DOUBLE = NS_STYLE_TEXT_DECORATION_STYLE_DOUBLE,
    LINESTYLE_WAVY   = NS_STYLE_TEXT_DECORATION_STYLE_WAVY
  };

  enum {
    DEFINED_NONE             = 0x00,
    DEFINED_LINESTYLE        = 0x01,
    DEFINED_FOREGROUND_COLOR = 0x02,
    DEFINED_BACKGROUND_COLOR = 0x04,
    DEFINED_UNDERLINE_COLOR  = 0x08
  };

  
  
  nsTextRangeStyle()
  {
    Clear();
  }

  void Clear()
  {
    mDefinedStyles = DEFINED_NONE;
    mLineStyle = LINESTYLE_NONE;
    mIsBoldLine = false;
    mForegroundColor = mBackgroundColor = mUnderlineColor = NS_RGBA(0, 0, 0, 0);
  }

  bool IsDefined() const { return mDefinedStyles != DEFINED_NONE; }

  bool IsLineStyleDefined() const
  {
    return (mDefinedStyles & DEFINED_LINESTYLE) != 0;
  }

  bool IsForegroundColorDefined() const
  {
    return (mDefinedStyles & DEFINED_FOREGROUND_COLOR) != 0;
  }

  bool IsBackgroundColorDefined() const
  {
    return (mDefinedStyles & DEFINED_BACKGROUND_COLOR) != 0;
  }

  bool IsUnderlineColorDefined() const
  {
    return (mDefinedStyles & DEFINED_UNDERLINE_COLOR) != 0;
  }

  bool IsNoChangeStyle() const
  {
    return !IsForegroundColorDefined() && !IsBackgroundColorDefined() &&
           IsLineStyleDefined() && mLineStyle == LINESTYLE_NONE;
  }

  bool Equals(const nsTextRangeStyle& aOther)
  {
    if (mDefinedStyles != aOther.mDefinedStyles)
      return false;
    if (IsLineStyleDefined() && (mLineStyle != aOther.mLineStyle ||
                                 !mIsBoldLine != !aOther.mIsBoldLine))
      return false;
    if (IsForegroundColorDefined() &&
        (mForegroundColor != aOther.mForegroundColor))
      return false;
    if (IsBackgroundColorDefined() &&
        (mBackgroundColor != aOther.mBackgroundColor))
      return false;
    if (IsUnderlineColorDefined() &&
        (mUnderlineColor != aOther.mUnderlineColor))
      return false;
    return true;
  }

  bool operator !=(const nsTextRangeStyle &aOther)
  {
    return !Equals(aOther);
  }

  bool operator ==(const nsTextRangeStyle &aOther)
  {
    return Equals(aOther);
  }

  uint8_t mDefinedStyles;
  uint8_t mLineStyle;        

  bool mIsBoldLine;  

  nscolor mForegroundColor;  
  nscolor mBackgroundColor;  
  nscolor mUnderlineColor;   
};

struct nsTextRange
{
  nsTextRange()
    : mStartOffset(0), mEndOffset(0), mRangeType(0)
  {
  }

  uint32_t mStartOffset;
  uint32_t mEndOffset;
  uint32_t mRangeType;

  nsTextRangeStyle mRangeStyle;
};

typedef nsTextRange* nsTextRangeArray;

class nsTextEvent : public nsInputEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;
  friend class mozilla::plugins::PPluginInstanceChild;

  nsTextEvent()
  {
  }

public:
  uint32_t seqno;

public:
  nsTextEvent(bool isTrusted, uint32_t msg, nsIWidget *w)
    : nsInputEvent(isTrusted, msg, w, NS_TEXT_EVENT),
      rangeCount(0), rangeArray(nullptr), isChar(false)
  {
  }

  nsString          theText;
  uint32_t          rangeCount;
  
  
  
  nsTextRangeArray  rangeArray;
  bool              isChar;
};

class nsCompositionEvent : public nsGUIEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  nsCompositionEvent()
  {
  }

public:
  uint32_t seqno;

public:
  nsCompositionEvent(bool isTrusted, uint32_t msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_COMPOSITION_EVENT)
  {
    
    
    
    mFlags.mCancelable = false;
  }

  nsString data;
};







class nsMouseScrollEvent : public nsMouseEvent_base
{
private:
  nsMouseScrollEvent()
  {
  }

public:
  nsMouseScrollEvent(bool isTrusted, uint32_t msg, nsIWidget *w)
    : nsMouseEvent_base(isTrusted, msg, w, NS_MOUSE_SCROLL_EVENT),
      delta(0), isHorizontal(false)
  {
  }

  int32_t               delta;
  bool                  isHorizontal;
};





namespace mozilla {
namespace widget {

class WheelEvent : public nsMouseEvent_base
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  WheelEvent()
  {
  }

public:
  WheelEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget) :
    nsMouseEvent_base(aIsTrusted, aMessage, aWidget, NS_WHEEL_EVENT),
    deltaX(0.0), deltaY(0.0), deltaZ(0.0),
    deltaMode(nsIDOMWheelEvent::DOM_DELTA_PIXEL),
    customizedByUserPrefs(false), isMomentum(false), isPixelOnlyDevice(false),
    lineOrPageDeltaX(0), lineOrPageDeltaY(0), scrollType(SCROLL_DEFAULT),
    overflowDeltaX(0.0), overflowDeltaY(0.0)
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

  
  
  
  enum ScrollType {
    SCROLL_DEFAULT,
    SCROLL_SYNCHRONOUSLY,
    SCROLL_ASYNCHRONOUSELY,
    SCROLL_SMOOTHLY
  };
  ScrollType scrollType;

  
  
  
  
  
  
  
  
  
  double overflowDeltaX;
  double overflowDeltaY;
};

} 
} 












class nsGestureNotifyEvent : public nsGUIEvent
{
public:
  enum ePanDirection {
    ePanNone,
    ePanVertical,
    ePanHorizontal,
    ePanBoth
  };
  
  ePanDirection panDirection;
  bool          displayPanFeedback;
  
  nsGestureNotifyEvent(bool aIsTrusted, uint32_t aMsg, nsIWidget *aWidget):
    nsGUIEvent(aIsTrusted, aMsg, aWidget, NS_GESTURENOTIFY_EVENT),
    panDirection(ePanNone),
    displayPanFeedback(false)
  {
  }
};

class nsQueryContentEvent : public nsGUIEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  nsQueryContentEvent()
  {
    mReply.mContentsRoot = nullptr;
    mReply.mFocusedWidget = nullptr;
  }

public:
  nsQueryContentEvent(bool aIsTrusted, uint32_t aMsg, nsIWidget *aWidget) :
    nsGUIEvent(aIsTrusted, aMsg, aWidget, NS_QUERY_CONTENT_EVENT),
    mSucceeded(false), mWasAsync(false)
  {
  }

  void InitForQueryTextContent(uint32_t aOffset, uint32_t aLength)
  {
    NS_ASSERTION(message == NS_QUERY_TEXT_CONTENT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
    mInput.mLength = aLength;
  }

  void InitForQueryCaretRect(uint32_t aOffset)
  {
    NS_ASSERTION(message == NS_QUERY_CARET_RECT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
  }

  void InitForQueryTextRect(uint32_t aOffset, uint32_t aLength)
  {
    NS_ASSERTION(message == NS_QUERY_TEXT_RECT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
    mInput.mLength = aLength;
  }

  void InitForQueryDOMWidgetHittest(nsIntPoint& aPoint)
  {
    NS_ASSERTION(message == NS_QUERY_DOM_WIDGET_HITTEST,
                 "wrong initializer is called");
    refPoint = aPoint;
  }

  uint32_t GetSelectionStart(void) const
  {
    NS_ASSERTION(message == NS_QUERY_SELECTED_TEXT,
                 "not querying selection");
    return mReply.mOffset + (mReply.mReversed ? mReply.mString.Length() : 0);
  }

  uint32_t GetSelectionEnd(void) const
  {
    NS_ASSERTION(message == NS_QUERY_SELECTED_TEXT,
                 "not querying selection");
    return mReply.mOffset + (mReply.mReversed ? 0 : mReply.mString.Length());
  }

  bool mSucceeded;
  bool mWasAsync;
  struct {
    uint32_t mOffset;
    uint32_t mLength;
  } mInput;
  struct {
    void* mContentsRoot;
    uint32_t mOffset;
    nsString mString;
    nsIntRect mRect; 
    
    nsIWidget* mFocusedWidget;
    bool mReversed; 
    bool mHasSelection; 
    bool mWidgetIsHit; 
    
    nsCOMPtr<nsITransferable> mTransferable;
  } mReply;

  enum {
    NOT_FOUND = UINT32_MAX
  };

  
  enum {
    SCROLL_ACTION_NONE,
    SCROLL_ACTION_LINE,
    SCROLL_ACTION_PAGE
  };
};

class nsFocusEvent : public nsEvent
{
public:
  nsFocusEvent(bool isTrusted, uint32_t msg)
    : nsEvent(isTrusted, msg, NS_FOCUS_EVENT),
      fromRaise(false),
      isRefocus(false)
  {
  }

  bool fromRaise;
  bool isRefocus;
};

class nsSelectionEvent : public nsGUIEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  nsSelectionEvent()
  {
  }

public:
  uint32_t seqno;

public:
  nsSelectionEvent(bool aIsTrusted, uint32_t aMsg, nsIWidget *aWidget) :
    nsGUIEvent(aIsTrusted, aMsg, aWidget, NS_SELECTION_EVENT),
    mExpandToClusterBoundary(true), mSucceeded(false)
  {
  }

  uint32_t mOffset; 
  uint32_t mLength; 
  bool mReversed; 
  bool mExpandToClusterBoundary; 
  bool mSucceeded;
};

class nsContentCommandEvent : public nsGUIEvent
{
public:
  nsContentCommandEvent(bool aIsTrusted, uint32_t aMsg, nsIWidget *aWidget,
                        bool aOnlyEnabledCheck = false) :
    nsGUIEvent(aIsTrusted, aMsg, aWidget, NS_CONTENT_COMMAND_EVENT),
    mOnlyEnabledCheck(bool(aOnlyEnabledCheck)),
    mSucceeded(false), mIsEnabled(false)
  {
  }

  
  nsCOMPtr<nsITransferable> mTransferable;                 

  
  
  enum {
    eCmdScrollUnit_Line,
    eCmdScrollUnit_Page,
    eCmdScrollUnit_Whole
  };

  struct ScrollInfo {
    ScrollInfo() :
      mAmount(0), mUnit(eCmdScrollUnit_Line), mIsHorizontal(false)
    {
    }

    int32_t      mAmount;                                  
    uint8_t      mUnit;                                    
    bool mIsHorizontal;                            
  } mScroll;

  bool mOnlyEnabledCheck;                          

  bool mSucceeded;                                 
  bool mIsEnabled;                                 
};

class nsTouchEvent : public nsInputEvent
{
public:
  nsTouchEvent()
  {
  }
  nsTouchEvent(bool isTrusted, nsTouchEvent *aEvent)
    : nsInputEvent(isTrusted,
                   aEvent->message,
                   aEvent->widget,
                   NS_TOUCH_EVENT)
  {
    modifiers = aEvent->modifiers;
    time = aEvent->time;
    touches.AppendElements(aEvent->touches);
    MOZ_COUNT_CTOR(nsTouchEvent);
  }
  nsTouchEvent(bool isTrusted, uint32_t msg, nsIWidget* w)
    : nsInputEvent(isTrusted, msg, w, NS_TOUCH_EVENT)
  {
    MOZ_COUNT_CTOR(nsTouchEvent);
  }
  ~nsTouchEvent()
  {
    MOZ_COUNT_DTOR(nsTouchEvent);
  }

  nsTArray<nsCOMPtr<nsIDOMTouch> > touches;
};








class nsFormEvent : public nsEvent
{
public:
  nsFormEvent(bool isTrusted, uint32_t msg)
    : nsEvent(isTrusted, msg, NS_FORM_EVENT),
      originator(nullptr)
  {
  }

  nsIContent *originator;
};







class nsCommandEvent : public nsGUIEvent
{
public:
  nsCommandEvent(bool isTrusted, nsIAtom* aEventType,
                 nsIAtom* aCommand, nsIWidget* w)
    : nsGUIEvent(isTrusted, NS_USER_DEFINED_EVENT, w, NS_COMMAND_EVENT)
  {
    userType = aEventType;
    command = aCommand;
  }

  nsCOMPtr<nsIAtom> command;
};




class nsUIEvent : public nsEvent
{
public:
  nsUIEvent(bool isTrusted, uint32_t msg, int32_t d)
    : nsEvent(isTrusted, msg, NS_UI_EVENT),
      detail(d)
  {
  }

  int32_t detail;
};




class nsSimpleGestureEvent : public nsMouseEvent_base
{
public:
  nsSimpleGestureEvent(bool isTrusted, uint32_t msg, nsIWidget* w,
                         uint32_t directionArg, double deltaArg)
    : nsMouseEvent_base(isTrusted, msg, w, NS_SIMPLE_GESTURE_EVENT),
      direction(directionArg), delta(deltaArg), clickCount(0)
  {
  }

  nsSimpleGestureEvent(const nsSimpleGestureEvent& other)
    : nsMouseEvent_base(other.mFlags.mIsTrusted,
                        other.message, other.widget, NS_SIMPLE_GESTURE_EVENT),
      direction(other.direction), delta(other.delta), clickCount(0)
  {
  }

  uint32_t direction;   
  double delta;         
  uint32_t clickCount;  
};

class nsTransitionEvent : public nsEvent
{
public:
  nsTransitionEvent(bool isTrusted, uint32_t msg,
                    const nsString &propertyNameArg, float elapsedTimeArg)
    : nsEvent(isTrusted, msg, NS_TRANSITION_EVENT),
      propertyName(propertyNameArg), elapsedTime(elapsedTimeArg)
  {
  }

  nsString propertyName;
  float elapsedTime;
};

class nsAnimationEvent : public nsEvent
{
public:
  nsAnimationEvent(bool isTrusted, uint32_t msg,
                   const nsString &animationNameArg, float elapsedTimeArg)
    : nsEvent(isTrusted, msg, NS_ANIMATION_EVENT),
      animationName(animationNameArg), elapsedTime(elapsedTimeArg)
  {
  }

  nsString animationName;
  float elapsedTime;
};





class nsPluginEvent : public nsGUIEvent
{
public:
  nsPluginEvent(bool isTrusted, uint32_t msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_PLUGIN_EVENT),
      retargetToFocusedDocument(false)
  {
  }

  
  
  
  bool retargetToFocusedDocument;
};




enum nsDragDropEventStatus {  
  
  nsDragDropEventStatus_eDragEntered,            
  
  nsDragDropEventStatus_eDragExited, 
  
  nsDragDropEventStatus_eDrop  
};

#define NS_IS_INPUT_EVENT(evnt) \
       (((evnt)->eventStructType == NS_INPUT_EVENT) || \
        ((evnt)->eventStructType == NS_MOUSE_EVENT) || \
        ((evnt)->eventStructType == NS_KEY_EVENT) || \
        ((evnt)->eventStructType == NS_TEXT_EVENT) || \
        ((evnt)->eventStructType == NS_TOUCH_EVENT) || \
        ((evnt)->eventStructType == NS_DRAG_EVENT) || \
        ((evnt)->eventStructType == NS_MOUSE_SCROLL_EVENT) || \
        ((evnt)->eventStructType == NS_SIMPLE_GESTURE_EVENT))

#define NS_IS_MOUSE_EVENT(evnt) \
       (((evnt)->message == NS_MOUSE_BUTTON_DOWN) || \
        ((evnt)->message == NS_MOUSE_BUTTON_UP) || \
        ((evnt)->message == NS_MOUSE_CLICK) || \
        ((evnt)->message == NS_MOUSE_DOUBLECLICK) || \
        ((evnt)->message == NS_MOUSE_ENTER) || \
        ((evnt)->message == NS_MOUSE_EXIT) || \
        ((evnt)->message == NS_MOUSE_ACTIVATE) || \
        ((evnt)->message == NS_MOUSE_ENTER_SYNTH) || \
        ((evnt)->message == NS_MOUSE_EXIT_SYNTH) || \
        ((evnt)->message == NS_MOUSE_MOZHITTEST) || \
        ((evnt)->message == NS_MOUSE_MOVE))

#define NS_IS_MOUSE_EVENT_STRUCT(evnt) \
       ((evnt)->eventStructType == NS_MOUSE_EVENT || \
        (evnt)->eventStructType == NS_DRAG_EVENT)

#define NS_IS_MOUSE_LEFT_CLICK(evnt) \
       ((evnt)->eventStructType == NS_MOUSE_EVENT && \
        (evnt)->message == NS_MOUSE_CLICK && \
        static_cast<nsMouseEvent*>((evnt))->button == nsMouseEvent::eLeftButton)

#define NS_IS_CONTEXT_MENU_KEY(evnt) \
       ((evnt)->eventStructType == NS_MOUSE_EVENT && \
        (evnt)->message == NS_CONTEXTMENU && \
        static_cast<nsMouseEvent*>((evnt))->context == nsMouseEvent::eContextMenuKey)

#define NS_IS_DRAG_EVENT(evnt) \
       (((evnt)->message == NS_DRAGDROP_ENTER) || \
        ((evnt)->message == NS_DRAGDROP_OVER) || \
        ((evnt)->message == NS_DRAGDROP_EXIT) || \
        ((evnt)->message == NS_DRAGDROP_DRAGDROP) || \
        ((evnt)->message == NS_DRAGDROP_GESTURE) || \
        ((evnt)->message == NS_DRAGDROP_DRAG) || \
        ((evnt)->message == NS_DRAGDROP_END) || \
        ((evnt)->message == NS_DRAGDROP_START) || \
        ((evnt)->message == NS_DRAGDROP_DROP) || \
        ((evnt)->message == NS_DRAGDROP_LEAVE_SYNTH))

#define NS_IS_KEY_EVENT(evnt) \
       (((evnt)->message == NS_KEY_DOWN) ||  \
        ((evnt)->message == NS_KEY_PRESS) || \
        ((evnt)->message == NS_KEY_UP))

#define NS_IS_IME_EVENT(evnt) \
       (((evnt)->message == NS_TEXT_TEXT) ||  \
        ((evnt)->message == NS_COMPOSITION_START) ||  \
        ((evnt)->message == NS_COMPOSITION_END) || \
        ((evnt)->message == NS_COMPOSITION_UPDATE))

#define NS_IS_ACTIVATION_EVENT(evnt) \
        (((evnt)->message == NS_PLUGIN_ACTIVATE) || \
        ((evnt)->message == NS_PLUGIN_FOCUS))

#define NS_IS_QUERY_CONTENT_EVENT(evnt) \
       ((evnt)->eventStructType == NS_QUERY_CONTENT_EVENT)

#define NS_IS_SELECTION_EVENT(evnt) \
       (((evnt)->message == NS_SELECTION_SET))

#define NS_IS_CONTENT_COMMAND_EVENT(evnt) \
       ((evnt)->eventStructType == NS_CONTENT_COMMAND_EVENT)

#define NS_IS_PLUGIN_EVENT(evnt) \
       (((evnt)->message == NS_PLUGIN_INPUT_EVENT) || \
        ((evnt)->message == NS_PLUGIN_FOCUS_EVENT))

#define NS_IS_RETARGETED_PLUGIN_EVENT(evnt) \
       (NS_IS_PLUGIN_EVENT(evnt) && \
        (static_cast<nsPluginEvent*>(evnt)->retargetToFocusedDocument))

#define NS_IS_NON_RETARGETED_PLUGIN_EVENT(evnt) \
       (NS_IS_PLUGIN_EVENT(evnt) && \
        !(static_cast<nsPluginEvent*>(evnt)->retargetToFocusedDocument))




#define NS_IS_IME_RELATED_EVENT(evnt) \
  (NS_IS_IME_EVENT(evnt) || \
   NS_IS_QUERY_CONTENT_EVENT(evnt) || \
   NS_IS_SELECTION_EVENT(evnt))








enum {
#define NS_DEFINE_VK(aDOMKeyName, aDOMKeyCode) NS_##aDOMKeyName = aDOMKeyCode
#include "nsVKList.h"
#undef NS_DEFINE_VK
};


#define NS_TEXTRANGE_CARETPOSITION         0x01
#define NS_TEXTRANGE_RAWINPUT              0x02
#define NS_TEXTRANGE_SELECTEDRAWTEXT       0x03
#define NS_TEXTRANGE_CONVERTEDTEXT         0x04
#define NS_TEXTRANGE_SELECTEDCONVERTEDTEXT 0x05






inline bool NS_IsEventUsingCoordinates(nsEvent* aEvent)
{
  return !NS_IS_KEY_EVENT(aEvent) && !NS_IS_IME_RELATED_EVENT(aEvent) &&
         !NS_IS_CONTEXT_MENU_KEY(aEvent) && !NS_IS_ACTIVATION_EVENT(aEvent) &&
         !NS_IS_PLUGIN_EVENT(aEvent) &&
         !NS_IS_CONTENT_COMMAND_EVENT(aEvent) &&
         aEvent->message != NS_PLUGIN_RESOLUTION_CHANGED;
}













inline bool NS_IsEventTargetedAtFocusedWindow(nsEvent* aEvent)
{
  return NS_IS_KEY_EVENT(aEvent) || NS_IS_IME_RELATED_EVENT(aEvent) ||
         NS_IS_CONTEXT_MENU_KEY(aEvent) ||
         NS_IS_CONTENT_COMMAND_EVENT(aEvent) ||
         NS_IS_RETARGETED_PLUGIN_EVENT(aEvent);
}












inline bool NS_IsEventTargetedAtFocusedContent(nsEvent* aEvent)
{
  return NS_IS_KEY_EVENT(aEvent) || NS_IS_IME_RELATED_EVENT(aEvent) ||
         NS_IS_CONTEXT_MENU_KEY(aEvent) ||
         NS_IS_RETARGETED_PLUGIN_EVENT(aEvent);
}




inline bool NS_IsAllowedToDispatchDOMEvent(nsEvent* aEvent)
{
  switch (aEvent->eventStructType) {
    case NS_MOUSE_EVENT:
      
      
      
      
      
      return static_cast<nsMouseEvent*>(aEvent)->reason == nsMouseEvent::eReal;

    case NS_WHEEL_EVENT: {
      
      
      mozilla::widget::WheelEvent* wheelEvent =
        static_cast<mozilla::widget::WheelEvent*>(aEvent);
      return wheelEvent->deltaX != 0.0 || wheelEvent->deltaY != 0.0 ||
             wheelEvent->deltaZ != 0.0;
    }

    default:
      return true;
  }
}

#endif 

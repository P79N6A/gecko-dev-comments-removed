




#ifndef mozilla_BasicEvents_h__
#define mozilla_BasicEvents_h__

#include <stdint.h>

#include "mozilla/dom/EventTarget.h"
#include "mozilla/EventForwards.h"
#include "mozilla/TimeStamp.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsISupportsImpl.h"
#include "nsIWidget.h"
#include "nsString.h"
#include "Units.h"







#define NS_EVENT_NULL                   0



#define NS_EVENT_ALL                    1

#define NS_WINDOW_START                 100


#define NS_XUL_CLOSE                    (NS_WINDOW_START + 1)

#define NS_KEY_PRESS                    (NS_WINDOW_START + 31)

#define NS_KEY_UP                       (NS_WINDOW_START + 32)

#define NS_KEY_DOWN                     (NS_WINDOW_START + 33)

#define NS_KEY_BEFORE_DOWN              (NS_WINDOW_START + 34)
#define NS_KEY_AFTER_DOWN               (NS_WINDOW_START + 35)
#define NS_KEY_BEFORE_UP                (NS_WINDOW_START + 36)
#define NS_KEY_AFTER_UP                 (NS_WINDOW_START + 37)

#define NS_RESIZE_EVENT                 (NS_WINDOW_START + 60)
#define NS_SCROLL_EVENT                 (NS_WINDOW_START + 61)





#define NS_PLUGIN_ACTIVATE               (NS_WINDOW_START + 62)
#define NS_PLUGIN_FOCUS                  (NS_WINDOW_START + 63)

#define NS_OFFLINE                       (NS_WINDOW_START + 64)
#define NS_ONLINE                        (NS_WINDOW_START + 65)




#define NS_MOZ_USER_IDLE                 (NS_WINDOW_START + 67)
#define NS_MOZ_USER_ACTIVE               (NS_WINDOW_START + 68)

#define NS_LANGUAGECHANGE                (NS_WINDOW_START + 70)

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
#define NS_MOUSE_MOZLONGTAP             (NS_MOUSE_MESSAGE_START + 36)


#define NS_POINTER_EVENT_START          4400
#define NS_POINTER_MOVE                 (NS_POINTER_EVENT_START)
#define NS_POINTER_UP                   (NS_POINTER_EVENT_START + 1)
#define NS_POINTER_DOWN                 (NS_POINTER_EVENT_START + 2)
#define NS_POINTER_OVER                 (NS_POINTER_EVENT_START + 22)
#define NS_POINTER_OUT                  (NS_POINTER_EVENT_START + 23)
#define NS_POINTER_ENTER                (NS_POINTER_EVENT_START + 24)
#define NS_POINTER_LEAVE                (NS_POINTER_EVENT_START + 25)
#define NS_POINTER_CANCEL               (NS_POINTER_EVENT_START + 26)
#define NS_POINTER_GOT_CAPTURE          (NS_POINTER_EVENT_START + 27)
#define NS_POINTER_LOST_CAPTURE         (NS_POINTER_EVENT_START + 28)

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
#define NS_FORM_INVALID                 (NS_FORM_EVENT_START + 4)


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

#define NS_MUTATION_START                    1800
#define NS_MUTATION_SUBTREEMODIFIED          (NS_MUTATION_START)
#define NS_MUTATION_NODEINSERTED             (NS_MUTATION_START+1)
#define NS_MUTATION_NODEREMOVED              (NS_MUTATION_START+2)
#define NS_MUTATION_NODEREMOVEDFROMDOCUMENT  (NS_MUTATION_START+3)
#define NS_MUTATION_NODEINSERTEDINTODOCUMENT (NS_MUTATION_START+4)
#define NS_MUTATION_ATTRMODIFIED             (NS_MUTATION_START+5)
#define NS_MUTATION_CHARACTERDATAMODIFIED    (NS_MUTATION_START+6)
#define NS_MUTATION_END                      (NS_MUTATION_START+6)

#define NS_USER_DEFINED_EVENT         2000
 

#define NS_COMPOSITION_EVENT_START    2200
#define NS_COMPOSITION_START          (NS_COMPOSITION_EVENT_START)



#define NS_COMPOSITION_END            (NS_COMPOSITION_EVENT_START + 1)




#define NS_COMPOSITION_UPDATE         (NS_COMPOSITION_EVENT_START + 2)




#define NS_COMPOSITION_CHANGE         (NS_COMPOSITION_EVENT_START + 3)






#define NS_COMPOSITION_COMMIT_AS_IS   (NS_COMPOSITION_EVENT_START + 4)





#define NS_COMPOSITION_COMMIT         (NS_COMPOSITION_EVENT_START + 5)


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
#define NS_NEED_KEY            (NS_MEDIA_EVENT_START+20)


#define NS_NOTIFYPAINT_START    3400
#define NS_AFTERPAINT           (NS_NOTIFYPAINT_START)


#define NS_SIMPLE_GESTURE_EVENT_START    3500
#define NS_SIMPLE_GESTURE_SWIPE_START    (NS_SIMPLE_GESTURE_EVENT_START)
#define NS_SIMPLE_GESTURE_SWIPE_UPDATE   (NS_SIMPLE_GESTURE_EVENT_START+1)
#define NS_SIMPLE_GESTURE_SWIPE_END      (NS_SIMPLE_GESTURE_EVENT_START+2)
#define NS_SIMPLE_GESTURE_SWIPE          (NS_SIMPLE_GESTURE_EVENT_START+3)
#define NS_SIMPLE_GESTURE_MAGNIFY_START  (NS_SIMPLE_GESTURE_EVENT_START+4)
#define NS_SIMPLE_GESTURE_MAGNIFY_UPDATE (NS_SIMPLE_GESTURE_EVENT_START+5)
#define NS_SIMPLE_GESTURE_MAGNIFY        (NS_SIMPLE_GESTURE_EVENT_START+6)
#define NS_SIMPLE_GESTURE_ROTATE_START   (NS_SIMPLE_GESTURE_EVENT_START+7)
#define NS_SIMPLE_GESTURE_ROTATE_UPDATE  (NS_SIMPLE_GESTURE_EVENT_START+8)
#define NS_SIMPLE_GESTURE_ROTATE         (NS_SIMPLE_GESTURE_EVENT_START+9)
#define NS_SIMPLE_GESTURE_TAP            (NS_SIMPLE_GESTURE_EVENT_START+10)
#define NS_SIMPLE_GESTURE_PRESSTAP       (NS_SIMPLE_GESTURE_EVENT_START+11)
#define NS_SIMPLE_GESTURE_EDGE_STARTED   (NS_SIMPLE_GESTURE_EVENT_START+12)
#define NS_SIMPLE_GESTURE_EDGE_CANCELED  (NS_SIMPLE_GESTURE_EVENT_START+13)
#define NS_SIMPLE_GESTURE_EDGE_COMPLETED (NS_SIMPLE_GESTURE_EVENT_START+14)


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

#define NS_WEBAUDIO_EVENT_START      4350
#define NS_AUDIO_PROCESS             (NS_WEBAUDIO_EVENT_START)
#define NS_AUDIO_COMPLETE            (NS_WEBAUDIO_EVENT_START + 1)


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
#define NS_TOUCH_CANCEL              (NS_TOUCH_EVENT_START+3)


#define NS_POINTERLOCK_START         5300
#define NS_POINTERLOCKCHANGE         (NS_POINTERLOCK_START)
#define NS_POINTERLOCKERROR          (NS_POINTERLOCK_START + 1)

#define NS_WHEEL_EVENT_START         5400
#define NS_WHEEL_WHEEL               (NS_WHEEL_EVENT_START)
#define NS_WHEEL_START               (NS_WHEEL_EVENT_START + 1)
#define NS_WHEEL_STOP                (NS_WHEEL_EVENT_START + 2)


#define NS_MOZ_TIME_CHANGE_EVENT     5500


#define NS_NETWORK_EVENT_START       5600
#define NS_NETWORK_UPLOAD_EVENT      (NS_NETWORK_EVENT_START + 1)
#define NS_NETWORK_DOWNLOAD_EVENT    (NS_NETWORK_EVENT_START + 2)


#define NS_MEDIARECORDER_EVENT_START 5700
#define NS_MEDIARECORDER_DATAAVAILABLE  (NS_MEDIARECORDER_EVENT_START + 1)
#define NS_MEDIARECORDER_WARNING        (NS_MEDIARECORDER_EVENT_START + 2)
#define NS_MEDIARECORDER_STOP           (NS_MEDIARECORDER_EVENT_START + 3)


#define NS_SPEAKERMANAGER_EVENT_START 5800
#define NS_SPEAKERMANAGER_SPEAKERFORCEDCHANGE (NS_SPEAKERMANAGER_EVENT_START + 1)

#ifdef MOZ_GAMEPAD

#define NS_GAMEPAD_START         6000
#define NS_GAMEPAD_BUTTONDOWN    (NS_GAMEPAD_START)
#define NS_GAMEPAD_BUTTONUP      (NS_GAMEPAD_START+1)
#define NS_GAMEPAD_AXISMOVE      (NS_GAMEPAD_START+2)
#define NS_GAMEPAD_CONNECTED     (NS_GAMEPAD_START+3)
#define NS_GAMEPAD_DISCONNECTED  (NS_GAMEPAD_START+4)

#define NS_GAMEPAD_END           (NS_GAMEPAD_START+4)
#endif


#define NS_EDITOR_EVENT_START    6100
#define NS_EDITOR_INPUT          (NS_EDITOR_EVENT_START)

namespace IPC {
template<typename T>
struct ParamTraits;
}

namespace mozilla {










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
  
  
  
  bool    mDefaultPreventedByChrome : 1;
  
  
  
  
  
  bool    mMultipleActionsPrevented : 1;
  
  
  bool    mIsBeingDispatched : 1;
  
  
  bool    mDispatchedAtLeastOnce : 1;
  
  
  bool    mIsSynthesizedForTests : 1;
  
  
  bool    mExceptionHasBeenRisen : 1;
  
  
  bool    mRetargetToNonNativeAnonymous : 1;
  
  
  bool    mNoCrossProcessBoundaryForwarding : 1;
  
  
  
  
  
  
  
  bool    mNoContentDispatch : 1;
  
  bool    mOnlyChromeDispatch : 1;
  
  
  
  
  bool mWantReplyFromContentProcess : 1;
  
  
  
  bool mHandledByAPZ : 1;

  
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
    static_assert(sizeof(BaseEventFlags) <= sizeof(RawFlags),
      "mozilla::EventFlags must not be bigger than the RawFlags");
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





class WidgetEvent
{
protected:
  WidgetEvent(bool aIsTrusted, uint32_t aMessage, EventClassID aEventClassID)
    : mClass(aEventClassID)
    , message(aMessage)
    , refPoint(0, 0)
    , lastRefPoint(0, 0)
    , time(0)
    , timeStamp(TimeStamp::Now())
    , userType(nullptr)
  {
    MOZ_COUNT_CTOR(WidgetEvent);
    mFlags.Clear();
    mFlags.mIsTrusted = aIsTrusted;
    mFlags.mCancelable = true;
    mFlags.mBubbles = true;
  }

  WidgetEvent()
  {
    MOZ_COUNT_CTOR(WidgetEvent);
  }

public:
  WidgetEvent(bool aIsTrusted, uint32_t aMessage)
    : mClass(eBasicEventClass)
    , message(aMessage)
    , refPoint(0, 0)
    , lastRefPoint(0, 0)
    , time(0)
    , timeStamp(TimeStamp::Now())
    , userType(nullptr)
  {
    MOZ_COUNT_CTOR(WidgetEvent);
    mFlags.Clear();
    mFlags.mIsTrusted = aIsTrusted;
    mFlags.mCancelable = true;
    mFlags.mBubbles = true;
  }

  virtual ~WidgetEvent()
  {
    MOZ_COUNT_DTOR(WidgetEvent);
  }

  WidgetEvent(const WidgetEvent& aOther)
  {
    MOZ_COUNT_CTOR(WidgetEvent);
    *this = aOther;
  }

  virtual WidgetEvent* Duplicate() const
  {
    MOZ_ASSERT(mClass == eBasicEventClass,
               "Duplicate() must be overridden by sub class");
    WidgetEvent* result = new WidgetEvent(false, message);
    result->AssignEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  EventClassID mClass;
  
  uint32_t message;
  
  
  LayoutDeviceIntPoint refPoint;
  
  LayoutDeviceIntPoint lastRefPoint;
  
  
  uint64_t time;
  
  
  mozilla::TimeStamp timeStamp;
  
  BaseEventFlags mFlags;

  
  nsCOMPtr<nsIAtom> userType;

  nsString typeString; 

  
  nsCOMPtr<dom::EventTarget> target;
  nsCOMPtr<dom::EventTarget> currentTarget;
  nsCOMPtr<dom::EventTarget> originalTarget;

  void AssignEventData(const WidgetEvent& aEvent, bool aCopyTargets)
  {
    
    
    refPoint = aEvent.refPoint;
    
    time = aEvent.time;
    timeStamp = aEvent.timeStamp;
    
    userType = aEvent.userType;
    
    target = aCopyTargets ? aEvent.target : nullptr;
    currentTarget = aCopyTargets ? aEvent.currentTarget : nullptr;
    originalTarget = aCopyTargets ? aEvent.originalTarget : nullptr;
  }

  



  



#define NS_ROOT_EVENT_CLASS(aPrefix, aName)
#define NS_EVENT_CLASS(aPrefix, aName) \
  virtual aPrefix##aName* As##aName(); \
  const aPrefix##aName* As##aName() const;

#include "mozilla/EventClassList.h"

#undef NS_EVENT_CLASS
#undef NS_ROOT_EVENT_CLASS

  


  bool IsQueryContentEvent() const;
  


  bool IsSelectionEvent() const;
  


  bool IsContentCommandEvent() const;
  


  bool IsNativeEventDelivererForPlugin() const;

  


  bool HasMouseEventMessage() const;
  


  bool HasDragEventMessage() const;
  


  bool HasKeyEventMessage() const;
  



  bool HasIMEEventMessage() const;
  


  bool HasPluginActivationEventMessage() const;

  



  bool IsRetargetedNativeEventDelivererForPlugin() const;
  



  bool IsNonRetargetedNativeEventDelivererForPlugin() const;
  




  bool IsIMERelatedEvent() const;

  




  bool IsUsingCoordinates() const;
  











  bool IsTargetedAtFocusedWindow() const;
  










  bool IsTargetedAtFocusedContent() const;
  


  bool IsAllowedToDispatchDOMEvent() const;
};





class WidgetGUIEvent : public WidgetEvent
{
protected:
  WidgetGUIEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget,
                 EventClassID aEventClassID)
    : WidgetEvent(aIsTrusted, aMessage, aEventClassID)
    , widget(aWidget)
  {
  }

  WidgetGUIEvent()
  {
  }

public:
  virtual WidgetGUIEvent* AsGUIEvent() override { return this; }

  WidgetGUIEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget) :
    WidgetEvent(aIsTrusted, aMessage, eGUIEventClass),
    widget(aWidget)
  {
  }

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_ASSERT(mClass == eGUIEventClass,
               "Duplicate() must be overridden by sub class");
    
    WidgetGUIEvent* result = new WidgetGUIEvent(false, message, nullptr);
    result->AssignGUIEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  
  nsCOMPtr<nsIWidget> widget;

  

























  class PluginEvent final
  {
    nsTArray<uint8_t> mBuffer;

    friend struct IPC::ParamTraits<mozilla::WidgetGUIEvent>;

  public:

    MOZ_EXPLICIT_CONVERSION operator bool() const
    {
      return !mBuffer.IsEmpty();
    }

    template<typename T>
    MOZ_EXPLICIT_CONVERSION operator const T*() const
    {
      return mBuffer.IsEmpty()
             ? nullptr
             : reinterpret_cast<const T*>(mBuffer.Elements());
    }

    template <typename T>
    void Copy(const T& other)
    {
      static_assert(!mozilla::IsPointer<T>::value, "Don't want a pointer!");
      mBuffer.SetLength(sizeof(T));
      memcpy(mBuffer.Elements(), &other, mBuffer.Length());
    }

    void Clear()
    {
      mBuffer.Clear();
    }
  };

  
  PluginEvent mPluginEvent;

  void AssignGUIEventData(const WidgetGUIEvent& aEvent, bool aCopyTargets)
  {
    AssignEventData(aEvent, aCopyTargets);

    

    mPluginEvent = aEvent.mPluginEvent;
  }
};








enum Modifier
{
  MODIFIER_NONE       = 0x0000,
  MODIFIER_ALT        = 0x0001,
  MODIFIER_ALTGRAPH   = 0x0002,
  MODIFIER_CAPSLOCK   = 0x0004,
  MODIFIER_CONTROL    = 0x0008,
  MODIFIER_FN         = 0x0010,
  MODIFIER_FNLOCK     = 0x0020,
  MODIFIER_META       = 0x0040,
  MODIFIER_NUMLOCK    = 0x0080,
  MODIFIER_SCROLLLOCK = 0x0100,
  MODIFIER_SHIFT      = 0x0200,
  MODIFIER_SYMBOL     = 0x0400,
  MODIFIER_SYMBOLLOCK = 0x0800,
  MODIFIER_OS         = 0x1000
};





#define NS_DOM_KEYNAME_ALT        "Alt"
#define NS_DOM_KEYNAME_ALTGRAPH   "AltGraph"
#define NS_DOM_KEYNAME_CAPSLOCK   "CapsLock"
#define NS_DOM_KEYNAME_CONTROL    "Control"
#define NS_DOM_KEYNAME_FN         "Fn"
#define NS_DOM_KEYNAME_FNLOCK     "FnLock"
#define NS_DOM_KEYNAME_META       "Meta"
#define NS_DOM_KEYNAME_NUMLOCK    "NumLock"
#define NS_DOM_KEYNAME_SCROLLLOCK "ScrollLock"
#define NS_DOM_KEYNAME_SHIFT      "Shift"
#define NS_DOM_KEYNAME_SYMBOL     "Symbol"
#define NS_DOM_KEYNAME_SYMBOLLOCK "SymbolLock"
#define NS_DOM_KEYNAME_OS         "OS"





typedef uint16_t Modifiers;





class WidgetInputEvent : public WidgetGUIEvent
{
protected:
  WidgetInputEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget,
                   EventClassID aEventClassID)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, aEventClassID)
    , modifiers(0)
  {
  }

  WidgetInputEvent()
  {
  }

public:
  virtual WidgetInputEvent* AsInputEvent() override { return this; }

  WidgetInputEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, eInputEventClass)
    , modifiers(0)
  {
  }

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_ASSERT(mClass == eInputEventClass,
               "Duplicate() must be overridden by sub class");
    
    WidgetInputEvent* result = new WidgetInputEvent(false, message, nullptr);
    result->AssignInputEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }


  



  static Modifier AccelModifier();

  


  static Modifier GetModifier(const nsAString& aDOMKeyName);

  
  bool IsAccel() const
  {
    return ((modifiers & AccelModifier()) != 0);
  }

  
  bool IsShift() const
  {
    return ((modifiers & MODIFIER_SHIFT) != 0);
  }
  
  bool IsControl() const
  {
    return ((modifiers & MODIFIER_CONTROL) != 0);
  }
  
  bool IsAlt() const
  {
    return ((modifiers & MODIFIER_ALT) != 0);
  }
  
  bool IsMeta() const
  {
    return ((modifiers & MODIFIER_META) != 0);
  }
  
  
  bool IsOS() const
  {
    return ((modifiers & MODIFIER_OS) != 0);
  }
  
  
  
  bool IsAltGraph() const
  {
    return ((modifiers & MODIFIER_ALTGRAPH) != 0);
  }
  
  bool IsCapsLocked() const
  {
    return ((modifiers & MODIFIER_CAPSLOCK) != 0);
  }
  
  bool IsNumLocked() const
  {
    return ((modifiers & MODIFIER_NUMLOCK) != 0);
  }
  
  bool IsScrollLocked() const
  {
    return ((modifiers & MODIFIER_SCROLLLOCK) != 0);
  }

  
  
  bool IsFn() const
  {
    return ((modifiers & MODIFIER_FN) != 0);
  }
  
  
  bool IsFnLocked() const
  {
    return ((modifiers & MODIFIER_FNLOCK) != 0);
  }
  
  
  bool IsSymbol() const
  {
    return ((modifiers & MODIFIER_SYMBOL) != 0);
  }
  
  
  bool IsSymbolLocked() const
  {
    return ((modifiers & MODIFIER_SYMBOLLOCK) != 0);
  }

  void InitBasicModifiers(bool aCtrlKey,
                          bool aAltKey,
                          bool aShiftKey,
                          bool aMetaKey)
  {
    modifiers = 0;
    if (aCtrlKey) {
      modifiers |= MODIFIER_CONTROL;
    }
    if (aAltKey) {
      modifiers |= MODIFIER_ALT;
    }
    if (aShiftKey) {
      modifiers |= MODIFIER_SHIFT;
    }
    if (aMetaKey) {
      modifiers |= MODIFIER_META;
    }
  }

  Modifiers modifiers;

  void AssignInputEventData(const WidgetInputEvent& aEvent, bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    modifiers = aEvent.modifiers;
  }
};







class InternalUIEvent : public WidgetGUIEvent
{
protected:
  InternalUIEvent()
    : detail(0)
  {
  }

  InternalUIEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget,
                  EventClassID aEventClassID)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, aEventClassID)
    , detail(0)
  {
  }

  InternalUIEvent(bool aIsTrusted, uint32_t aMessage,
                  EventClassID aEventClassID)
    : WidgetGUIEvent(aIsTrusted, aMessage, nullptr, aEventClassID)
    , detail(0)
  {
  }

public:
  virtual InternalUIEvent* AsUIEvent() override { return this; }

  InternalUIEvent(bool aIsTrusted, uint32_t aMessage)
    : WidgetGUIEvent(aIsTrusted, aMessage, nullptr, eUIEventClass)
    , detail(0)
  {
  }

  virtual WidgetEvent* Duplicate() const override
  {
    MOZ_ASSERT(mClass == eUIEventClass,
               "Duplicate() must be overridden by sub class");
    InternalUIEvent* result = new InternalUIEvent(false, message);
    result->AssignUIEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  int32_t detail;

  void AssignUIEventData(const InternalUIEvent& aEvent, bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    detail = aEvent.detail;
  }
};

} 

#endif 










































#ifndef nsGUIEvent_h__
#define nsGUIEvent_h__

#include "nsPoint.h"
#include "nsRect.h"
#include "nsRegion.h"
#include "nsEvent.h"
#include "nsStringGlue.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMNSMouseEvent.h"
#include "nsIDOMDataTransfer.h"
#include "nsIDOMEventTarget.h"
#include "nsWeakPtr.h"
#include "nsIWidget.h"
#include "nsTArray.h"
#include "nsTraceRefcnt.h"
#include "nsITransferable.h"
#include "nsIVariant.h"
#include "nsStyleConsts.h"

namespace mozilla {
namespace dom {
  class PBrowserParent;
  class PBrowserChild;
}
}

#ifdef ACCESSIBILITY
class nsAccessible;
#endif
class nsRenderingContext;
class nsIMenuItem;
class nsIContent;
class nsIURI;
class nsHashKey;




#define NS_EVENT                           1
#define NS_GUI_EVENT                       2
#define NS_SIZE_EVENT                      3
#define NS_SIZEMODE_EVENT                  4
#define NS_ZLEVEL_EVENT                    5
#define NS_PAINT_EVENT                     6
#define NS_SCROLLBAR_EVENT                 7
#define NS_INPUT_EVENT                     8
#define NS_KEY_EVENT                       9
#define NS_MOUSE_EVENT                    10
#define NS_SCRIPT_ERROR_EVENT             12
#define NS_TEXT_EVENT                     13
#define NS_COMPOSITION_EVENT              14
#define NS_MOUSE_SCROLL_EVENT             16
#define NS_SCROLLPORT_EVENT               18
#define NS_MUTATION_EVENT                 19 // |nsMutationEvent| in content
#define NS_ACCESSIBLE_EVENT               20
#define NS_FORM_EVENT                     21
#define NS_FOCUS_EVENT                    22
#define NS_POPUP_EVENT                    23
#define NS_COMMAND_EVENT                  24
#define NS_SCROLLAREA_EVENT               25
#define NS_TRANSITION_EVENT               26
#define NS_ANIMATION_EVENT                27

#define NS_UI_EVENT                       28
#define NS_SVG_EVENT                      30
#define NS_SVGZOOM_EVENT                  31
#ifdef MOZ_SMIL
#define NS_SMIL_TIME_EVENT                32
#endif 

#define NS_QUERY_CONTENT_EVENT            33

#define NS_DRAG_EVENT                     35
#define NS_NOTIFYPAINT_EVENT              36
#define NS_SIMPLE_GESTURE_EVENT           37
#define NS_SELECTION_EVENT                38
#define NS_CONTENT_COMMAND_EVENT          39
#define NS_GESTURENOTIFY_EVENT            40
#define NS_UISTATECHANGE_EVENT            41
#define NS_MOZTOUCH_EVENT                 42
#define NS_PLUGIN_EVENT                   43




#define NS_EVENT_FLAG_NONE                0x0000
#define NS_EVENT_FLAG_TRUSTED             0x0001
#define NS_EVENT_FLAG_BUBBLE              0x0002
#define NS_EVENT_FLAG_CAPTURE             0x0004
#define NS_EVENT_FLAG_STOP_DISPATCH       0x0008
#define NS_EVENT_FLAG_NO_DEFAULT          0x0010
#define NS_EVENT_FLAG_CANT_CANCEL         0x0020
#define NS_EVENT_FLAG_CANT_BUBBLE         0x0040
#define NS_PRIV_EVENT_FLAG_SCRIPT         0x0080
#define NS_EVENT_FLAG_NO_CONTENT_DISPATCH 0x0100
#define NS_EVENT_FLAG_SYSTEM_EVENT        0x0200

#define NS_EVENT_DISPATCHED               0x0400
#define NS_EVENT_FLAG_DISPATCHING         0x0800




#define NS_EVENT_FLAG_SYNTHETIC_TEST_EVENT 0x1000


#define NS_EVENT_FLAG_ONLY_CHROME_DISPATCH 0x2000


#define NS_EVENT_FLAG_NO_DEFAULT_CALLED_IN_CONTENT 0x4000

#define NS_PRIV_EVENT_UNTRUSTED_PERMITTED 0x8000

#define NS_EVENT_FLAG_EXCEPTION_THROWN    0x10000

#define NS_EVENT_FLAG_PREVENT_MULTIPLE_ACTIONS 0x20000

#define NS_EVENT_RETARGET_TO_NON_NATIVE_ANONYMOUS 0x40000

#define NS_EVENT_CAPTURE_MASK             (~(NS_EVENT_FLAG_BUBBLE | NS_EVENT_FLAG_NO_CONTENT_DISPATCH))
#define NS_EVENT_BUBBLE_MASK              (~(NS_EVENT_FLAG_CAPTURE | NS_EVENT_FLAG_NO_CONTENT_DISPATCH))

#define NS_EVENT_TYPE_NULL                   0




 
#define NS_EVENT_NULL                   0


#define NS_WINDOW_START                 100


#define NS_CREATE                       (NS_WINDOW_START)

#define NS_XUL_CLOSE                    (NS_WINDOW_START + 1)

#define NS_DESTROY                      (NS_WINDOW_START + 2)

#define NS_SIZE                         (NS_WINDOW_START + 3)

#define NS_SIZEMODE                     (NS_WINDOW_START + 4)

#define NS_ACTIVATE                     (NS_WINDOW_START + 7)

#define NS_DEACTIVATE                   (NS_WINDOW_START + 8)

#define NS_SETZLEVEL                    (NS_WINDOW_START + 9)


#define NS_DID_PAINT                   (NS_WINDOW_START + 28)

#define NS_WILL_PAINT                   (NS_WINDOW_START + 29)

#define NS_PAINT                        (NS_WINDOW_START + 30)

#define NS_KEY_PRESS                    (NS_WINDOW_START + 31)

#define NS_KEY_UP                       (NS_WINDOW_START + 32)

#define NS_KEY_DOWN                     (NS_WINDOW_START + 33)


#define NS_MOVE                         (NS_WINDOW_START + 34) 


#define NS_TABCHANGE                    (NS_WINDOW_START + 35)

#define NS_OS_TOOLBAR                   (NS_WINDOW_START + 36)


#define NS_DISPLAYCHANGED                (NS_WINDOW_START + 40)


#define NS_THEMECHANGED                 (NS_WINDOW_START + 41)




#define NS_SYSCOLORCHANGED              (NS_WINDOW_START + 42)



#define NS_UISTATECHANGED               (NS_WINDOW_START + 43)


#define NS_DONESIZEMOVE                 (NS_WINDOW_START + 44)

#define NS_RESIZE_EVENT                 (NS_WINDOW_START + 60)
#define NS_SCROLL_EVENT                 (NS_WINDOW_START + 61)





#define NS_PLUGIN_ACTIVATE               (NS_WINDOW_START + 62)
#define NS_PLUGIN_FOCUS                  (NS_WINDOW_START + 63)

#define NS_OFFLINE                       (NS_WINDOW_START + 64)
#define NS_ONLINE                        (NS_WINDOW_START + 65)


#define NS_BEFORERESIZE_EVENT            (NS_WINDOW_START + 66)

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

#define NS_CONTEXTMENU_MESSAGE_START    500
#define NS_CONTEXTMENU                  (NS_CONTEXTMENU_MESSAGE_START)

#define NS_SCROLLBAR_MESSAGE_START      1000
#define NS_SCROLLBAR_POS                (NS_SCROLLBAR_MESSAGE_START)
#define NS_SCROLLBAR_PAGE_NEXT          (NS_SCROLLBAR_MESSAGE_START + 1)
#define NS_SCROLLBAR_PAGE_PREV          (NS_SCROLLBAR_MESSAGE_START + 2)
#define NS_SCROLLBAR_LINE_NEXT          (NS_SCROLLBAR_MESSAGE_START + 3)
#define NS_SCROLLBAR_LINE_PREV          (NS_SCROLLBAR_MESSAGE_START + 4)

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
#define NS_SCROLLPORT_OVERFLOWCHANGED (NS_SCROLLPORT_START+2)




#define NS_ACCESSIBLE_START           1900
#define NS_GETACCESSIBLE              (NS_ACCESSIBLE_START)

#define NS_USER_DEFINED_EVENT         2000
 

#define NS_COMPOSITION_EVENT_START    2200
#define NS_COMPOSITION_START          (NS_COMPOSITION_EVENT_START)
#define NS_COMPOSITION_END            (NS_COMPOSITION_EVENT_START + 1)


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





#define NS_QUERY_SCROLL_TARGET_INFO     (NS_QUERY_CONTENT_EVENT_START + 99)


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
#define NS_BEFOREPAINT          (NS_NOTIFYPAINT_START+1)


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

#ifdef MOZ_SMIL
#define NS_SMIL_TIME_EVENT_START     4300
#define NS_SMIL_BEGIN                (NS_SMIL_TIME_EVENT_START)
#define NS_SMIL_END                  (NS_SMIL_TIME_EVENT_START + 1)
#define NS_SMIL_REPEAT               (NS_SMIL_TIME_EVENT_START + 2)
#endif 

#define NS_MOZTOUCH_EVENT_START      4400
#define NS_MOZTOUCH_DOWN             (NS_MOZTOUCH_EVENT_START)
#define NS_MOZTOUCH_MOVE             (NS_MOZTOUCH_EVENT_START+1)
#define NS_MOZTOUCH_UP               (NS_MOZTOUCH_EVENT_START+2)


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

#define NS_SHOW_EVENT                5000









enum nsWindowZ {
  nsWindowZTop = 0,   
  nsWindowZBottom,    
  nsWindowZRelative   
};





class nsEvent
{
protected:
  nsEvent(PRBool isTrusted, PRUint32 msg, PRUint8 structType)
    : eventStructType(structType),
      message(msg),
      refPoint(0, 0),
      time(0),
      flags(isTrusted ? NS_EVENT_FLAG_TRUSTED : NS_EVENT_FLAG_NONE),
      userType(0)
  {
    MOZ_COUNT_CTOR(nsEvent);
  }

  nsEvent()
  {
  }

public:
  nsEvent(PRBool isTrusted, PRUint32 msg)
    : eventStructType(NS_EVENT),
      message(msg),
      refPoint(0, 0),
      time(0),
      flags(isTrusted ? NS_EVENT_FLAG_TRUSTED : NS_EVENT_FLAG_NONE),
      userType(0)
  {
    MOZ_COUNT_CTOR(nsEvent);
  }

  ~nsEvent()
  {
    MOZ_COUNT_DTOR(nsEvent);
  }

  
  PRUint8     eventStructType;
  
  PRUint32    message;
  
  
  nsIntPoint  refPoint;
  
  
  PRUint64    time;
  
  
  PRUint32    flags;
  
  nsCOMPtr<nsIAtom>     userType;
  
  nsCOMPtr<nsIDOMEventTarget> target;
  nsCOMPtr<nsIDOMEventTarget> currentTarget;
  nsCOMPtr<nsIDOMEventTarget> originalTarget;
};





class nsGUIEvent : public nsEvent
{
protected:
  nsGUIEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w, PRUint8 structType)
    : nsEvent(isTrusted, msg, structType),
      widget(w), pluginEvent(nsnull)
  {
  }

  nsGUIEvent()
    : pluginEvent(nsnull)
  {
  }

public:
  nsGUIEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsEvent(isTrusted, msg, NS_GUI_EVENT),
      widget(w), pluginEvent(nsnull)
  {
  }

  
  nsCOMPtr<nsIWidget> widget;

  
  void* pluginEvent;
};





class nsScriptErrorEvent : public nsEvent
{
public:
  nsScriptErrorEvent(PRBool isTrusted, PRUint32 msg)
    : nsEvent(isTrusted, msg, NS_SCRIPT_ERROR_EVENT),
      lineNr(0), errorMsg(nsnull), fileName(nsnull)
  {
  }

  PRInt32           lineNr;
  const PRUnichar*  errorMsg;
  const PRUnichar*  fileName;
};





class nsSizeEvent : public nsGUIEvent
{
public:
  nsSizeEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_SIZE_EVENT),
      windowSize(nsnull), mWinWidth(0), mWinHeight(0)
  {
  }

  
  nsIntRect       *windowSize;
  
  PRInt32         mWinWidth;    
  
  PRInt32         mWinHeight;    
};





class nsSizeModeEvent : public nsGUIEvent
{
public:
  nsSizeModeEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_SIZEMODE_EVENT),
      mSizeMode(nsSizeMode_Normal)
  {
  }

  nsSizeMode      mSizeMode;
};





class nsZLevelEvent : public nsGUIEvent
{
public:
  nsZLevelEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_ZLEVEL_EVENT),
      mPlacement(nsWindowZTop), mReqBelow(nsnull), mActualBelow(nsnull),
      mImmediate(PR_FALSE), mAdjusted(PR_FALSE)
  {
  }

  nsWindowZ  mPlacement;
  nsIWidget *mReqBelow,    
            *mActualBelow; 
  PRBool     mImmediate,   
             mAdjusted;    
};





class nsPaintEvent : public nsGUIEvent
{
public:
  nsPaintEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_PAINT_EVENT),
      willSendDidPaint(PR_FALSE)
  {
  }

  
  nsIntRegion region;
  PRPackedBool willSendDidPaint;
};





class nsScrollbarEvent : public nsGUIEvent
{
public:
  nsScrollbarEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_SCROLLBAR_EVENT),
      position(0)
  {
  }

  
  PRUint32        position; 
};

class nsScrollPortEvent : public nsGUIEvent
{
public:
  enum orientType {
    vertical   = 0,
    horizontal = 1,
    both       = 2
  };

  nsScrollPortEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_SCROLLPORT_EVENT),
      orient(vertical)
  {
  }

  orientType orient;
};

class nsScrollAreaEvent : public nsGUIEvent
{
public:
  nsScrollAreaEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_SCROLLAREA_EVENT)
  {
  }

  nsRect mArea;
};

class nsInputEvent : public nsGUIEvent
{
protected:
  nsInputEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w,
               PRUint8 structType)
    : nsGUIEvent(isTrusted, msg, w, structType),
      isShift(PR_FALSE), isControl(PR_FALSE), isAlt(PR_FALSE), isMeta(PR_FALSE)
  {
  }

  nsInputEvent()
  {
  }

public:
  nsInputEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_INPUT_EVENT),
      isShift(PR_FALSE), isControl(PR_FALSE), isAlt(PR_FALSE), isMeta(PR_FALSE)
  {
  }

  
  PRBool          isShift;        
  
  PRBool          isControl;      
  
  PRBool          isAlt;          
  
  PRBool          isMeta;
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

  nsMouseEvent_base(PRBool isTrusted, PRUint32 msg, nsIWidget *w, PRUint8 type)
  : nsInputEvent(isTrusted, msg, w, type), button(0), pressure(0),
    inputSource(nsIDOMNSMouseEvent::MOZ_SOURCE_MOUSE) {}

  
  nsCOMPtr<nsISupports> relatedTarget;

  PRInt16               button;

  
  
  float                 pressure;

  
  PRUint16              inputSource;
};

class nsMouseEvent : public nsMouseEvent_base
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

public:
  enum buttonType  { eLeftButton = 0, eMiddleButton = 1, eRightButton = 2 };
  enum reasonType  { eReal, eSynthesized };
  enum contextType { eNormal, eContextMenuKey };
  enum exitType    { eChild, eTopLevel };

  nsMouseEvent()
  {
  }

protected:
  nsMouseEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w,
               PRUint8 structType, reasonType aReason)
    : nsMouseEvent_base(isTrusted, msg, w, structType),
      acceptActivation(PR_FALSE), ignoreRootScrollFrame(PR_FALSE),
      reason(aReason), context(eNormal), exit(eChild), clickCount(0)
  {
    if (msg == NS_MOUSE_MOVE) {
      flags |= NS_EVENT_FLAG_CANT_CANCEL;
    }
  }

public:

  nsMouseEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w,
               reasonType aReason, contextType aContext = eNormal)
    : nsMouseEvent_base(isTrusted, msg, w, NS_MOUSE_EVENT),
      acceptActivation(PR_FALSE), ignoreRootScrollFrame(PR_FALSE),
      reason(aReason), context(aContext), exit(eChild), clickCount(0)
  {
    if (msg == NS_MOUSE_MOVE) {
      flags |= NS_EVENT_FLAG_CANT_CANCEL;
    } else if (msg == NS_CONTEXTMENU) {
      button = (context == eNormal) ? eRightButton : eLeftButton;
    }
  }
#ifdef NS_DEBUG
  ~nsMouseEvent() {
    NS_WARN_IF_FALSE(message != NS_CONTEXTMENU ||
                     button ==
                       ((context == eNormal) ? eRightButton : eLeftButton),
                     "Wrong button set to NS_CONTEXTMENU event?");
  }
#endif

  
  
  PRPackedBool acceptActivation;
  
  
  PRPackedBool ignoreRootScrollFrame;

  reasonType   reason : 4;
  contextType  context : 4;
  exitType     exit;

  
  PRUint32     clickCount;
};





class nsDragEvent : public nsMouseEvent
{
public:
  nsDragEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsMouseEvent(isTrusted, msg, w, NS_DRAG_EVENT, eReal),
      userCancelled(PR_FALSE)
  {
    if (msg == NS_DRAGDROP_EXIT_SYNTH ||
        msg == NS_DRAGDROP_LEAVE_SYNTH ||
        msg == NS_DRAGDROP_END) {
      flags |= NS_EVENT_FLAG_CANT_CANCEL;
    }
  }

  nsCOMPtr<nsIDOMDataTransfer> dataTransfer;
  PRPackedBool userCancelled;
};

#ifdef ACCESSIBILITY




class nsAccessibleEvent : public nsInputEvent
{
public:
  nsAccessibleEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsInputEvent(isTrusted, msg, w, NS_ACCESSIBLE_EVENT),
      mAccessible(nsnull)
  {
  }

  nsAccessible *mAccessible;
};
#endif





struct nsAlternativeCharCode {
  nsAlternativeCharCode(PRUint32 aUnshiftedCharCode,
                        PRUint32 aShiftedCharCode) :
    mUnshiftedCharCode(aUnshiftedCharCode), mShiftedCharCode(aShiftedCharCode)
  {
  }
  PRUint32 mUnshiftedCharCode;
  PRUint32 mShiftedCharCode;
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

  nsKeyEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsInputEvent(isTrusted, msg, w, NS_KEY_EVENT),
      keyCode(0), charCode(0), isChar(0)
  {
  }

  
  PRUint32        keyCode;   
  
  PRUint32        charCode;
  
  
  nsTArray<nsAlternativeCharCode> alternativeCharCodes;
  
  PRBool          isChar;
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
    mIsBoldLine = PR_FALSE;
    mForegroundColor = mBackgroundColor = mUnderlineColor = NS_RGBA(0, 0, 0, 0);
  }

  PRBool IsDefined() const { return mDefinedStyles != DEFINED_NONE; }

  PRBool IsLineStyleDefined() const
  {
    return (mDefinedStyles & DEFINED_LINESTYLE) != 0;
  }

  PRBool IsForegroundColorDefined() const
  {
    return (mDefinedStyles & DEFINED_FOREGROUND_COLOR) != 0;
  }

  PRBool IsBackgroundColorDefined() const
  {
    return (mDefinedStyles & DEFINED_BACKGROUND_COLOR) != 0;
  }

  PRBool IsUnderlineColorDefined() const
  {
    return (mDefinedStyles & DEFINED_UNDERLINE_COLOR) != 0;
  }

  PRBool IsNoChangeStyle() const
  {
    return !IsForegroundColorDefined() && !IsBackgroundColorDefined() &&
           IsLineStyleDefined() && mLineStyle == LINESTYLE_NONE;
  }

  PRBool Equals(const nsTextRangeStyle& aOther)
  {
    if (mDefinedStyles != aOther.mDefinedStyles)
      return PR_FALSE;
    if (IsLineStyleDefined() && (mLineStyle != aOther.mLineStyle ||
                                 !mIsBoldLine != !aOther.mIsBoldLine))
      return PR_FALSE;
    if (IsForegroundColorDefined() &&
        (mForegroundColor != aOther.mForegroundColor))
      return PR_FALSE;
    if (IsBackgroundColorDefined() &&
        (mBackgroundColor != aOther.mBackgroundColor))
      return PR_FALSE;
    if (IsUnderlineColorDefined() &&
        (mUnderlineColor != aOther.mUnderlineColor))
      return PR_FALSE;
    return PR_TRUE;
  }

  PRBool operator !=(const nsTextRangeStyle &aOther)
  {
    return !Equals(aOther);
  }

  PRBool operator ==(const nsTextRangeStyle &aOther)
  {
    return Equals(aOther);
  }

  PRUint8 mDefinedStyles;
  PRUint8 mLineStyle;        

  PRPackedBool mIsBoldLine;  

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

  PRUint32 mStartOffset;
  PRUint32 mEndOffset;
  PRUint32 mRangeType;

  nsTextRangeStyle mRangeStyle;
};

typedef nsTextRange* nsTextRangeArray;

class nsTextEvent : public nsInputEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  nsTextEvent()
  {
  }

public:
  PRUint32 seqno;

public:
  nsTextEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsInputEvent(isTrusted, msg, w, NS_TEXT_EVENT),
      rangeCount(0), rangeArray(nsnull), isChar(PR_FALSE)
  {
  }

  nsString          theText;
  PRUint32          rangeCount;
  
  
  
  nsTextRangeArray  rangeArray;
  PRBool            isChar;
};

class nsCompositionEvent : public nsInputEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  nsCompositionEvent()
  {
  }

public:
  PRUint32 seqno;

public:
  nsCompositionEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsInputEvent(isTrusted, msg, w, NS_COMPOSITION_EVENT)
  {
  }
};






































class nsMouseScrollEvent : public nsMouseEvent_base
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  nsMouseScrollEvent()
  {
  }

public:
  enum nsMouseScrollFlags {
    kIsFullPage =   1 << 0,
    kIsVertical =   1 << 1,
    kIsHorizontal = 1 << 2,
    kHasPixels =    1 << 3, 
                            
                            
                            
                            
                            
                            
                            
                            
    kNoLines =      1 << 4, 
                            
                            
                            
                            
    kNoDefer =      1 << 5, 
                            
    kIsMomentum =   1 << 6, 
                            
                            
    kAllowSmoothScroll = 1 << 7 
                                
  };

  nsMouseScrollEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsMouseEvent_base(isTrusted, msg, w, NS_MOUSE_SCROLL_EVENT),
      scrollFlags(0), delta(0), scrollOverflow(0)
  {
  }

  PRInt32               scrollFlags;
  PRInt32               delta;
  PRInt32               scrollOverflow;
};












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
  PRPackedBool  displayPanFeedback;
  
  nsGestureNotifyEvent(PRBool aIsTrusted, PRUint32 aMsg, nsIWidget *aWidget):
    nsGUIEvent(aIsTrusted, aMsg, aWidget, NS_GESTURENOTIFY_EVENT),
    panDirection(ePanNone),
    displayPanFeedback(PR_FALSE)
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
    mReply.mContentsRoot = nsnull;
    mReply.mFocusedWidget = nsnull;
  }

public:
  nsQueryContentEvent(PRBool aIsTrusted, PRUint32 aMsg, nsIWidget *aWidget) :
    nsGUIEvent(aIsTrusted, aMsg, aWidget, NS_QUERY_CONTENT_EVENT),
    mSucceeded(PR_FALSE), mWasAsync(PR_FALSE)
  {
  }

  void InitForQueryTextContent(PRUint32 aOffset, PRUint32 aLength)
  {
    NS_ASSERTION(message == NS_QUERY_TEXT_CONTENT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
    mInput.mLength = aLength;
  }

  void InitForQueryCaretRect(PRUint32 aOffset)
  {
    NS_ASSERTION(message == NS_QUERY_CARET_RECT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
  }

  void InitForQueryTextRect(PRUint32 aOffset, PRUint32 aLength)
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

  void InitForQueryScrollTargetInfo(nsMouseScrollEvent* aEvent)
  {
    NS_ASSERTION(message == NS_QUERY_SCROLL_TARGET_INFO,
                 "wrong initializer is called");
    mInput.mMouseScrollEvent = aEvent;
  }

  PRUint32 GetSelectionStart(void) const
  {
    NS_ASSERTION(message == NS_QUERY_SELECTED_TEXT,
                 "not querying selection");
    return mReply.mOffset + (mReply.mReversed ? mReply.mString.Length() : 0);
  }

  PRUint32 GetSelectionEnd(void) const
  {
    NS_ASSERTION(message == NS_QUERY_SELECTED_TEXT,
                 "not querying selection");
    return mReply.mOffset + (mReply.mReversed ? 0 : mReply.mString.Length());
  }

  PRBool mSucceeded;
  PRPackedBool mWasAsync;
  struct {
    PRUint32 mOffset;
    PRUint32 mLength;
    
    nsMouseScrollEvent* mMouseScrollEvent;
  } mInput;
  struct {
    void* mContentsRoot;
    PRUint32 mOffset;
    nsString mString;
    nsIntRect mRect; 
    
    nsIWidget* mFocusedWidget;
    PRPackedBool mReversed; 
    PRPackedBool mHasSelection; 
    PRPackedBool mWidgetIsHit; 
    
    nsCOMPtr<nsITransferable> mTransferable;
    
    PRInt32 mLineHeight;
    PRInt32 mPageWidth;
    PRInt32 mPageHeight;
    
    
    
    
    
    
    PRInt32 mComputedScrollAmount;
    PRInt32 mComputedScrollAction;
  } mReply;

  enum {
    NOT_FOUND = PR_UINT32_MAX
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
  nsFocusEvent(PRBool isTrusted, PRUint32 msg)
    : nsEvent(isTrusted, msg, NS_FOCUS_EVENT),
      fromRaise(PR_FALSE),
      isRefocus(PR_FALSE)
  {
  }

  PRPackedBool fromRaise;
  PRPackedBool isRefocus;
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
  PRUint32 seqno;

public:
  nsSelectionEvent(PRBool aIsTrusted, PRUint32 aMsg, nsIWidget *aWidget) :
    nsGUIEvent(aIsTrusted, aMsg, aWidget, NS_SELECTION_EVENT),
    mExpandToClusterBoundary(PR_TRUE), mSucceeded(PR_FALSE)
  {
  }

  PRUint32 mOffset; 
  PRUint32 mLength; 
  PRPackedBool mReversed; 
  PRPackedBool mExpandToClusterBoundary; 
  PRPackedBool mSucceeded;
};

class nsContentCommandEvent : public nsGUIEvent
{
public:
  nsContentCommandEvent(PRBool aIsTrusted, PRUint32 aMsg, nsIWidget *aWidget,
                        PRBool aOnlyEnabledCheck = PR_FALSE) :
    nsGUIEvent(aIsTrusted, aMsg, aWidget, NS_CONTENT_COMMAND_EVENT),
    mOnlyEnabledCheck(PRPackedBool(aOnlyEnabledCheck)),
    mSucceeded(PR_FALSE), mIsEnabled(PR_FALSE)
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
      mAmount(0), mUnit(eCmdScrollUnit_Line), mIsHorizontal(PR_FALSE)
    {
    }

    PRInt32      mAmount;                                  
    PRUint8      mUnit;                                    
    PRPackedBool mIsHorizontal;                            
  } mScroll;

  PRPackedBool mOnlyEnabledCheck;                          

  PRPackedBool mSucceeded;                                 
  PRPackedBool mIsEnabled;                                 
};

class nsMozTouchEvent : public nsMouseEvent_base
{
public:
  nsMozTouchEvent(PRBool isTrusted, PRUint32 msg, nsIWidget* w,
                  PRUint32 streamIdArg)
    : nsMouseEvent_base(isTrusted, msg, w, NS_MOZTOUCH_EVENT),
      streamId(streamIdArg)
  {
  }

  PRUint32 streamId;
};








class nsFormEvent : public nsEvent
{
public:
  nsFormEvent(PRBool isTrusted, PRUint32 msg)
    : nsEvent(isTrusted, msg, NS_FORM_EVENT),
      originator(nsnull)
  {
  }

  nsIContent *originator;
};







class nsCommandEvent : public nsGUIEvent
{
public:
  nsCommandEvent(PRBool isTrusted, nsIAtom* aEventType,
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
  nsUIEvent(PRBool isTrusted, PRUint32 msg, PRInt32 d)
    : nsEvent(isTrusted, msg, NS_UI_EVENT),
      detail(d)
  {
  }

  PRInt32 detail;
};




class nsSimpleGestureEvent : public nsMouseEvent_base
{
public:
  nsSimpleGestureEvent(PRBool isTrusted, PRUint32 msg, nsIWidget* w,
                         PRUint32 directionArg, PRFloat64 deltaArg)
    : nsMouseEvent_base(isTrusted, msg, w, NS_SIMPLE_GESTURE_EVENT),
      direction(directionArg), delta(deltaArg)
  {
  }

  PRUint32 direction;   
  PRFloat64 delta;      
};

class nsTransitionEvent : public nsEvent
{
public:
  nsTransitionEvent(PRBool isTrusted, PRUint32 msg,
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
  nsAnimationEvent(PRBool isTrusted, PRUint32 msg,
                   const nsString &animationNameArg, float elapsedTimeArg)
    : nsEvent(isTrusted, msg, NS_ANIMATION_EVENT),
      animationName(animationNameArg), elapsedTime(elapsedTimeArg)
  {
  }

  nsString animationName;
  float elapsedTime;
};

class nsUIStateChangeEvent : public nsGUIEvent
{
public:
  nsUIStateChangeEvent(PRBool isTrusted, PRUint32 msg, nsIWidget* w)
    : nsGUIEvent(isTrusted, msg, w, NS_UISTATECHANGE_EVENT),
      showAccelerators(UIStateChangeType_NoChange),
      showFocusRings(UIStateChangeType_NoChange)
  {
  }

  UIStateChangeType showAccelerators;
  UIStateChangeType showFocusRings;
};





class nsPluginEvent : public nsGUIEvent
{
public:
  nsPluginEvent(PRBool isTrusted, PRUint32 msg, nsIWidget *w)
    : nsGUIEvent(isTrusted, msg, w, NS_PLUGIN_EVENT),
      retargetToFocusedDocument(PR_FALSE)
  {
  }

  
  
  
  PRBool retargetToFocusedDocument;
};




enum nsDragDropEventStatus {  
  
  nsDragDropEventStatus_eDragEntered,            
  
  nsDragDropEventStatus_eDragExited, 
  
  nsDragDropEventStatus_eDrop  
};


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
        ((evnt)->message == NS_COMPOSITION_END))

#define NS_IS_ACTIVATION_EVENT(evnt) \
       (((evnt)->message == NS_ACTIVATE) || \
        ((evnt)->message == NS_DEACTIVATE) || \
        ((evnt)->message == NS_PLUGIN_ACTIVATE) || \
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

#define NS_IS_TRUSTED_EVENT(event) \
  (((event)->flags & NS_EVENT_FLAG_TRUSTED) != 0)


#define NS_MARK_EVENT_DISPATCH_STARTED(event) \
  (event)->flags |= NS_EVENT_FLAG_DISPATCHING;

#define NS_IS_EVENT_IN_DISPATCH(event) \
  (((event)->flags & NS_EVENT_FLAG_DISPATCHING) != 0)


#define NS_MARK_EVENT_DISPATCH_DONE(event) \
  NS_ASSERTION(NS_IS_EVENT_IN_DISPATCH(event), \
               "Event never got marked for dispatch!"); \
  (event)->flags &= ~NS_EVENT_FLAG_DISPATCHING; \
  (event)->flags |= NS_EVENT_DISPATCHED;




#define NS_IS_IME_RELATED_EVENT(evnt) \
  (NS_IS_IME_EVENT(evnt) || \
   (NS_IS_QUERY_CONTENT_EVENT(evnt) && \
    evnt->message != NS_QUERY_SCROLL_TARGET_INFO) || \
   NS_IS_SELECTION_EVENT(evnt))








#define NS_VK_CANCEL         nsIDOMKeyEvent::DOM_VK_CANCEL
#define NS_VK_HELP           nsIDOMKeyEvent::DOM_VK_HELP
#define NS_VK_BACK           nsIDOMKeyEvent::DOM_VK_BACK_SPACE
#define NS_VK_TAB            nsIDOMKeyEvent::DOM_VK_TAB
#define NS_VK_CLEAR          nsIDOMKeyEvent::DOM_VK_CLEAR
#define NS_VK_RETURN         nsIDOMKeyEvent::DOM_VK_RETURN
#define NS_VK_ENTER          nsIDOMKeyEvent::DOM_VK_ENTER
#define NS_VK_SHIFT          nsIDOMKeyEvent::DOM_VK_SHIFT
#define NS_VK_CONTROL        nsIDOMKeyEvent::DOM_VK_CONTROL
#define NS_VK_ALT            nsIDOMKeyEvent::DOM_VK_ALT
#define NS_VK_PAUSE          nsIDOMKeyEvent::DOM_VK_PAUSE
#define NS_VK_CAPS_LOCK      nsIDOMKeyEvent::DOM_VK_CAPS_LOCK
#define NS_VK_KANA           nsIDOMKeyEvent::DOM_VK_KANA
#define NS_VK_HANGUL         nsIDOMKeyEvent::DOM_VK_HANGUL
#define NS_VK_JUNJA          nsIDOMKeyEvent::DOM_VK_JUNJA
#define NS_VK_FINAL          nsIDOMKeyEvent::DOM_VK_FINAL
#define NS_VK_HANJA          nsIDOMKeyEvent::DOM_VK_HANJA
#define NS_VK_KANJI          nsIDOMKeyEvent::DOM_VK_KANJI
#define NS_VK_ESCAPE         nsIDOMKeyEvent::DOM_VK_ESCAPE
#define NS_VK_CONVERT        nsIDOMKeyEvent::DOM_VK_CONVERT
#define NS_VK_NONCONVERT     nsIDOMKeyEvent::DOM_VK_NONCONVERT
#define NS_VK_ACCEPT         nsIDOMKeyEvent::DOM_VK_ACCEPT
#define NS_VK_MODECHANGE     nsIDOMKeyEvent::DOM_VK_MODECHANGE
#define NS_VK_SPACE          nsIDOMKeyEvent::DOM_VK_SPACE
#define NS_VK_PAGE_UP        nsIDOMKeyEvent::DOM_VK_PAGE_UP
#define NS_VK_PAGE_DOWN      nsIDOMKeyEvent::DOM_VK_PAGE_DOWN
#define NS_VK_END            nsIDOMKeyEvent::DOM_VK_END
#define NS_VK_HOME           nsIDOMKeyEvent::DOM_VK_HOME
#define NS_VK_LEFT           nsIDOMKeyEvent::DOM_VK_LEFT
#define NS_VK_UP             nsIDOMKeyEvent::DOM_VK_UP
#define NS_VK_RIGHT          nsIDOMKeyEvent::DOM_VK_RIGHT
#define NS_VK_DOWN           nsIDOMKeyEvent::DOM_VK_DOWN
#define NS_VK_SELECT         nsIDOMKeyEvent::DOM_VK_SELECT
#define NS_VK_PRINT          nsIDOMKeyEvent::DOM_VK_PRINT
#define NS_VK_EXECUTE        nsIDOMKeyEvent::DOM_VK_EXECUTE
#define NS_VK_PRINTSCREEN    nsIDOMKeyEvent::DOM_VK_PRINTSCREEN
#define NS_VK_INSERT         nsIDOMKeyEvent::DOM_VK_INSERT
#define NS_VK_DELETE         nsIDOMKeyEvent::DOM_VK_DELETE


#define NS_VK_0              nsIDOMKeyEvent::DOM_VK_0
#define NS_VK_1              nsIDOMKeyEvent::DOM_VK_1
#define NS_VK_2              nsIDOMKeyEvent::DOM_VK_2
#define NS_VK_3              nsIDOMKeyEvent::DOM_VK_3
#define NS_VK_4              nsIDOMKeyEvent::DOM_VK_4
#define NS_VK_5              nsIDOMKeyEvent::DOM_VK_5
#define NS_VK_6              nsIDOMKeyEvent::DOM_VK_6
#define NS_VK_7              nsIDOMKeyEvent::DOM_VK_7
#define NS_VK_8              nsIDOMKeyEvent::DOM_VK_8
#define NS_VK_9              nsIDOMKeyEvent::DOM_VK_9

#define NS_VK_SEMICOLON      nsIDOMKeyEvent::DOM_VK_SEMICOLON
#define NS_VK_EQUALS         nsIDOMKeyEvent::DOM_VK_EQUALS


#define NS_VK_A              nsIDOMKeyEvent::DOM_VK_A
#define NS_VK_B              nsIDOMKeyEvent::DOM_VK_B
#define NS_VK_C              nsIDOMKeyEvent::DOM_VK_C
#define NS_VK_D              nsIDOMKeyEvent::DOM_VK_D
#define NS_VK_E              nsIDOMKeyEvent::DOM_VK_E
#define NS_VK_F              nsIDOMKeyEvent::DOM_VK_F
#define NS_VK_G              nsIDOMKeyEvent::DOM_VK_G
#define NS_VK_H              nsIDOMKeyEvent::DOM_VK_H
#define NS_VK_I              nsIDOMKeyEvent::DOM_VK_I
#define NS_VK_J              nsIDOMKeyEvent::DOM_VK_J
#define NS_VK_K              nsIDOMKeyEvent::DOM_VK_K
#define NS_VK_L              nsIDOMKeyEvent::DOM_VK_L
#define NS_VK_M              nsIDOMKeyEvent::DOM_VK_M
#define NS_VK_N              nsIDOMKeyEvent::DOM_VK_N
#define NS_VK_O              nsIDOMKeyEvent::DOM_VK_O
#define NS_VK_P              nsIDOMKeyEvent::DOM_VK_P
#define NS_VK_Q              nsIDOMKeyEvent::DOM_VK_Q
#define NS_VK_R              nsIDOMKeyEvent::DOM_VK_R
#define NS_VK_S              nsIDOMKeyEvent::DOM_VK_S
#define NS_VK_T              nsIDOMKeyEvent::DOM_VK_T
#define NS_VK_U              nsIDOMKeyEvent::DOM_VK_U
#define NS_VK_V              nsIDOMKeyEvent::DOM_VK_V
#define NS_VK_W              nsIDOMKeyEvent::DOM_VK_W
#define NS_VK_X              nsIDOMKeyEvent::DOM_VK_X
#define NS_VK_Y              nsIDOMKeyEvent::DOM_VK_Y
#define NS_VK_Z              nsIDOMKeyEvent::DOM_VK_Z

#define NS_VK_CONTEXT_MENU   nsIDOMKeyEvent::DOM_VK_CONTEXT_MENU
#define NS_VK_SLEEP          nsIDOMKeyEvent::DOM_VK_SLEEP

#define NS_VK_NUMPAD0        nsIDOMKeyEvent::DOM_VK_NUMPAD0
#define NS_VK_NUMPAD1        nsIDOMKeyEvent::DOM_VK_NUMPAD1
#define NS_VK_NUMPAD2        nsIDOMKeyEvent::DOM_VK_NUMPAD2
#define NS_VK_NUMPAD3        nsIDOMKeyEvent::DOM_VK_NUMPAD3
#define NS_VK_NUMPAD4        nsIDOMKeyEvent::DOM_VK_NUMPAD4
#define NS_VK_NUMPAD5        nsIDOMKeyEvent::DOM_VK_NUMPAD5
#define NS_VK_NUMPAD6        nsIDOMKeyEvent::DOM_VK_NUMPAD6
#define NS_VK_NUMPAD7        nsIDOMKeyEvent::DOM_VK_NUMPAD7
#define NS_VK_NUMPAD8        nsIDOMKeyEvent::DOM_VK_NUMPAD8
#define NS_VK_NUMPAD9        nsIDOMKeyEvent::DOM_VK_NUMPAD9
#define NS_VK_MULTIPLY       nsIDOMKeyEvent::DOM_VK_MULTIPLY
#define NS_VK_ADD            nsIDOMKeyEvent::DOM_VK_ADD
#define NS_VK_SEPARATOR      nsIDOMKeyEvent::DOM_VK_SEPARATOR
#define NS_VK_SUBTRACT       nsIDOMKeyEvent::DOM_VK_SUBTRACT
#define NS_VK_DECIMAL        nsIDOMKeyEvent::DOM_VK_DECIMAL
#define NS_VK_DIVIDE         nsIDOMKeyEvent::DOM_VK_DIVIDE
#define NS_VK_F1             nsIDOMKeyEvent::DOM_VK_F1
#define NS_VK_F2             nsIDOMKeyEvent::DOM_VK_F2
#define NS_VK_F3             nsIDOMKeyEvent::DOM_VK_F3
#define NS_VK_F4             nsIDOMKeyEvent::DOM_VK_F4
#define NS_VK_F5             nsIDOMKeyEvent::DOM_VK_F5
#define NS_VK_F6             nsIDOMKeyEvent::DOM_VK_F6
#define NS_VK_F7             nsIDOMKeyEvent::DOM_VK_F7
#define NS_VK_F8             nsIDOMKeyEvent::DOM_VK_F8
#define NS_VK_F9             nsIDOMKeyEvent::DOM_VK_F9
#define NS_VK_F10            nsIDOMKeyEvent::DOM_VK_F10
#define NS_VK_F11            nsIDOMKeyEvent::DOM_VK_F11
#define NS_VK_F12            nsIDOMKeyEvent::DOM_VK_F12
#define NS_VK_F13            nsIDOMKeyEvent::DOM_VK_F13
#define NS_VK_F14            nsIDOMKeyEvent::DOM_VK_F14
#define NS_VK_F15            nsIDOMKeyEvent::DOM_VK_F15
#define NS_VK_F16            nsIDOMKeyEvent::DOM_VK_F16
#define NS_VK_F17            nsIDOMKeyEvent::DOM_VK_F17
#define NS_VK_F18            nsIDOMKeyEvent::DOM_VK_F18
#define NS_VK_F19            nsIDOMKeyEvent::DOM_VK_F19
#define NS_VK_F20            nsIDOMKeyEvent::DOM_VK_F20
#define NS_VK_F21            nsIDOMKeyEvent::DOM_VK_F21
#define NS_VK_F22            nsIDOMKeyEvent::DOM_VK_F22
#define NS_VK_F23            nsIDOMKeyEvent::DOM_VK_F23
#define NS_VK_F24            nsIDOMKeyEvent::DOM_VK_F24

#define NS_VK_NUM_LOCK       nsIDOMKeyEvent::DOM_VK_NUM_LOCK
#define NS_VK_SCROLL_LOCK    nsIDOMKeyEvent::DOM_VK_SCROLL_LOCK

#define NS_VK_COMMA          nsIDOMKeyEvent::DOM_VK_COMMA
#define NS_VK_PERIOD         nsIDOMKeyEvent::DOM_VK_PERIOD
#define NS_VK_SLASH          nsIDOMKeyEvent::DOM_VK_SLASH
#define NS_VK_BACK_QUOTE     nsIDOMKeyEvent::DOM_VK_BACK_QUOTE
#define NS_VK_OPEN_BRACKET   nsIDOMKeyEvent::DOM_VK_OPEN_BRACKET
#define NS_VK_BACK_SLASH     nsIDOMKeyEvent::DOM_VK_BACK_SLASH
#define NS_VK_CLOSE_BRACKET  nsIDOMKeyEvent::DOM_VK_CLOSE_BRACKET
#define NS_VK_QUOTE          nsIDOMKeyEvent::DOM_VK_QUOTE

#define NS_VK_META           nsIDOMKeyEvent::DOM_VK_META


#define NS_TEXTRANGE_CARETPOSITION         0x01
#define NS_TEXTRANGE_RAWINPUT              0x02
#define NS_TEXTRANGE_SELECTEDRAWTEXT       0x03
#define NS_TEXTRANGE_CONVERTEDTEXT         0x04
#define NS_TEXTRANGE_SELECTEDCONVERTEDTEXT 0x05






inline PRBool NS_IsEventUsingCoordinates(nsEvent* aEvent)
{
  return !NS_IS_KEY_EVENT(aEvent) && !NS_IS_IME_RELATED_EVENT(aEvent) &&
         !NS_IS_CONTEXT_MENU_KEY(aEvent) && !NS_IS_ACTIVATION_EVENT(aEvent) &&
         !NS_IS_PLUGIN_EVENT(aEvent) &&
         !NS_IS_CONTENT_COMMAND_EVENT(aEvent) &&
         aEvent->eventStructType != NS_ACCESSIBLE_EVENT;
}













inline PRBool NS_IsEventTargetedAtFocusedWindow(nsEvent* aEvent)
{
  return NS_IS_KEY_EVENT(aEvent) || NS_IS_IME_RELATED_EVENT(aEvent) ||
         NS_IS_CONTEXT_MENU_KEY(aEvent) ||
         NS_IS_CONTENT_COMMAND_EVENT(aEvent) ||
         NS_IS_RETARGETED_PLUGIN_EVENT(aEvent);
}












inline PRBool NS_IsEventTargetedAtFocusedContent(nsEvent* aEvent)
{
  return NS_IS_KEY_EVENT(aEvent) || NS_IS_IME_RELATED_EVENT(aEvent) ||
         NS_IS_CONTEXT_MENU_KEY(aEvent) ||
         NS_IS_RETARGETED_PLUGIN_EVENT(aEvent);
}

#endif

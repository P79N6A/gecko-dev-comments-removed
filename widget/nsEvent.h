




#ifndef nsEvent_h__
#define nsEvent_h__

#include "mozilla/StandardInteger.h"







enum UIStateChangeType {
  UIStateChangeType_NoChange,
  UIStateChangeType_Set,
  UIStateChangeType_Clear
};





enum nsEventStatus {  
    
  nsEventStatus_eIgnore,            
    
  nsEventStatus_eConsumeNoDefault, 
    
  nsEventStatus_eConsumeDoDefault  
};




enum nsSizeMode {
  nsSizeMode_Normal = 0,
  nsSizeMode_Minimized,
  nsSizeMode_Maximized,
  nsSizeMode_Fullscreen
};

struct nsAlternativeCharCode;
struct nsTextRangeStyle;
struct nsTextRange;

class nsEvent;
class nsGUIEvent;
class nsScriptErrorEvent;
class nsScrollbarEvent;
class nsScrollPortEvent;
class nsScrollAreaEvent;
class nsInputEvent;
class nsMouseEvent_base;
class nsMouseEvent;
class nsDragEvent;
class nsKeyEvent;
class nsTextEvent;
class nsCompositionEvent;
class nsMouseScrollEvent;
class nsGestureNotifyEvent;
class nsQueryContentEvent;
class nsFocusEvent;
class nsSelectionEvent;
class nsContentCommandEvent;
class nsTouchEvent;
class nsFormEvent;
class nsCommandEvent;
class nsUIEvent;
class nsSimpleGestureEvent;
class nsTransitionEvent;
class nsAnimationEvent;
class nsPluginEvent;

namespace mozilla {
namespace widget {

struct EventFlags;

class WheelEvent;



enum Modifier {
  MODIFIER_ALT        = 0x0001,
  MODIFIER_ALTGRAPH   = 0x0002,
  MODIFIER_CAPSLOCK   = 0x0004,
  MODIFIER_CONTROL    = 0x0008,
  MODIFIER_FN         = 0x0010,
  MODIFIER_META       = 0x0020,
  MODIFIER_NUMLOCK    = 0x0040,
  MODIFIER_SCROLLLOCK = 0x0080,
  MODIFIER_SHIFT      = 0x0100,
  MODIFIER_SYMBOLLOCK = 0x0200,
  MODIFIER_OS         = 0x0400
};

typedef uint16_t Modifiers;


enum NotificationToIME {
  
  
  NOTIFY_IME_OF_CURSOR_POS_CHANGED,
  
  NOTIFY_IME_OF_FOCUS,
  
  NOTIFY_IME_OF_BLUR,
  
  NOTIFY_IME_OF_SELECTION_CHANGE,
  REQUEST_TO_COMMIT_COMPOSITION,
  REQUEST_TO_CANCEL_COMPOSITION
};

} 
} 

#define NS_DOM_KEYNAME_ALT        "Alt"
#define NS_DOM_KEYNAME_ALTGRAPH   "AltGraph"
#define NS_DOM_KEYNAME_CAPSLOCK   "CapsLock"
#define NS_DOM_KEYNAME_CONTROL    "Control"
#define NS_DOM_KEYNAME_FN         "Fn"
#define NS_DOM_KEYNAME_META       "Meta"
#define NS_DOM_KEYNAME_NUMLOCK    "NumLock"
#define NS_DOM_KEYNAME_SCROLLLOCK "ScrollLock"
#define NS_DOM_KEYNAME_SHIFT      "Shift"
#define NS_DOM_KEYNAME_SYMBOLLOCK "SymbolLock"
#define NS_DOM_KEYNAME_OS         "OS"

#endif 

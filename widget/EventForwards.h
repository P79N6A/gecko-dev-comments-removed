




#ifndef mozilla_EventForwards_h__
#define mozilla_EventForwards_h__

#include <stdint.h>









enum nsEventStatus
{
  
  nsEventStatus_eIgnore,
  
  nsEventStatus_eConsumeNoDefault,
  
  nsEventStatus_eConsumeDoDefault
};

namespace mozilla {





enum Modifier
{
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

#define NS_DEFINE_KEYNAME(aCPPName, aDOMKeyName) \
  KEY_NAME_INDEX_##aCPPName,

enum KeyNameIndex
{
#include "nsDOMKeyNameList.h"
  
  
  NUMBER_OF_KEY_NAME_INDEX
};

#undef NS_DEFINE_KEYNAME

} 






namespace mozilla {
struct EventFlags;
} 

class nsEvent;
class nsGUIEvent;
class nsInputEvent;
class nsUIEvent;


struct nsAlternativeCharCode;
struct nsTextRangeStyle;
struct nsTextRange;

class nsKeyEvent;
class nsTextEvent;
class nsCompositionEvent;
class nsQueryContentEvent;
class nsSelectionEvent;


class nsMouseEvent_base;
class nsMouseEvent;
class nsDragEvent;
class nsMouseScrollEvent;

namespace mozilla {
class WheelEvent;
} 


class nsGestureNotifyEvent;
class nsTouchEvent;
class nsSimpleGestureEvent;


class nsScriptErrorEvent;
class nsScrollPortEvent;
class nsScrollAreaEvent;
class nsFormEvent;
class nsClipboardEvent;
class nsFocusEvent;
class nsTransitionEvent;
class nsAnimationEvent;


class nsCommandEvent;
class nsContentCommandEvent;
class nsPluginEvent;


class nsMutationEvent;

#endif 

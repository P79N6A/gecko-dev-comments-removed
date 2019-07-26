




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

class WidgetEvent;
class WidgetGUIEvent;
class WidgetInputEvent;
class InternalUIEvent;
} 


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


typedef mozilla::WidgetEvent      nsEvent;
typedef mozilla::WidgetGUIEvent   nsGUIEvent;
typedef mozilla::WidgetInputEvent nsInputEvent;
typedef mozilla::InternalUIEvent  nsUIEvent;

#endif 

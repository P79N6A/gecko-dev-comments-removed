




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


struct AlternativeCharCode;
struct TextRangeStyle;
struct TextRange;

typedef TextRange* TextRangeArray;

class WidgetKeyboardEvent;
class WidgetTextEvent;
class WidgetCompositionEvent;
class WidgetQueryContentEvent;
class WidgetSelectionEvent;


class WidgetMouseEventBase;
class WidgetMouseEvent;
class WidgetDragEvent;
class WidgetMouseScrollEvent;
class WidgetWheelEvent;
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


typedef mozilla::WidgetEvent              nsEvent;
typedef mozilla::WidgetGUIEvent           nsGUIEvent;
typedef mozilla::WidgetInputEvent         nsInputEvent;
typedef mozilla::InternalUIEvent          nsUIEvent;
typedef mozilla::AlternativeCharCode      nsAlternativeCharCode;
typedef mozilla::WidgetKeyboardEvent      nsKeyEvent;
typedef mozilla::TextRangeStyle           nsTextRangeStyle;
typedef mozilla::TextRange                nsTextRange;
typedef mozilla::TextRangeArray           nsTextRangeArray;
typedef mozilla::WidgetTextEvent          nsTextEvent;
typedef mozilla::WidgetCompositionEvent   nsCompositionEvent;
typedef mozilla::WidgetQueryContentEvent  nsQueryContentEvent;
typedef mozilla::WidgetSelectionEvent     nsSelectionEvent;
typedef mozilla::WidgetMouseEventBase     nsMouseEvent_base;
typedef mozilla::WidgetMouseEvent         nsMouseEvent;
typedef mozilla::WidgetDragEvent          nsDragEvent;
typedef mozilla::WidgetMouseScrollEvent   nsMouseScrollEvent;

namespace mozilla {
typedef WidgetWheelEvent                  WheelEvent;
}

#endif 

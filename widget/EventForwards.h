




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


class WidgetGestureNotifyEvent;
class WidgetSimpleGestureEvent;
class WidgetTouchEvent;


class InternalScriptErrorEvent;
class InternalScrollPortEvent;
class InternalScrollAreaEvent;
class InternalFormEvent;
class InternalClipboardEvent;
class InternalFocusEvent;
class InternalTransitionEvent;
class InternalAnimationEvent;


class WidgetCommandEvent;
class WidgetContentCommandEvent;
class WidgetPluginEvent;


class InternalMutationEvent;
} 


typedef mozilla::WidgetEvent               nsEvent;
typedef mozilla::WidgetGUIEvent            nsGUIEvent;
typedef mozilla::WidgetInputEvent          nsInputEvent;
typedef mozilla::InternalUIEvent           nsUIEvent;
typedef mozilla::AlternativeCharCode       nsAlternativeCharCode;
typedef mozilla::WidgetKeyboardEvent       nsKeyEvent;
typedef mozilla::TextRangeStyle            nsTextRangeStyle;
typedef mozilla::TextRange                 nsTextRange;
typedef mozilla::TextRangeArray            nsTextRangeArray;
typedef mozilla::WidgetTextEvent           nsTextEvent;
typedef mozilla::WidgetCompositionEvent    nsCompositionEvent;
typedef mozilla::WidgetQueryContentEvent   nsQueryContentEvent;
typedef mozilla::WidgetSelectionEvent      nsSelectionEvent;
typedef mozilla::WidgetMouseEventBase      nsMouseEvent_base;
typedef mozilla::WidgetMouseEvent          nsMouseEvent;
typedef mozilla::WidgetDragEvent           nsDragEvent;
typedef mozilla::WidgetMouseScrollEvent    nsMouseScrollEvent;

namespace mozilla {
typedef WidgetWheelEvent                   WheelEvent;
}

typedef mozilla::WidgetGestureNotifyEvent  nsGestureNotifyEvent;
typedef mozilla::WidgetSimpleGestureEvent  nsSimpleGestureEvent;

#endif 

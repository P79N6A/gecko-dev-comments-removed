




#ifndef mozilla_ContentEvents_h__
#define mozilla_ContentEvents_h__

#include <stdint.h>

#include "mozilla/BasicEvents.h"
#include "mozilla/dom/EventTarget.h"
#include "nsCOMPtr.h"
#include "nsIDOMDataTransfer.h"
#include "nsRect.h"
#include "nsStringGlue.h"

class nsIContent;

namespace mozilla {





class InternalScriptErrorEvent : public WidgetEvent
{
public:
  InternalScriptErrorEvent(bool aIsTrusted, uint32_t aMessage) :
    WidgetEvent(aIsTrusted, aMessage, NS_SCRIPT_ERROR_EVENT),
    lineNr(0), errorMsg(nullptr), fileName(nullptr)
  {
  }

  int32_t           lineNr;
  const PRUnichar*  errorMsg;
  const PRUnichar*  fileName;

  
  void AssignScriptErrorEventData(const InternalScriptErrorEvent& aEvent,
                                  bool aCopyTargets)
  {
    AssignEventData(aEvent, aCopyTargets);

    lineNr = aEvent.lineNr;

    
    
    errorMsg = nullptr;
    fileName = nullptr;
  }
};





class InternalScrollPortEvent : public WidgetGUIEvent
{
public:
  enum orientType
  {
    vertical   = 0,
    horizontal = 1,
    both       = 2
  };

  InternalScrollPortEvent(bool aIsTrusted, uint32_t aMessage,
                          nsIWidget* aWidget) :
    WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_SCROLLPORT_EVENT),
    orient(vertical)
  {
  }

  orientType orient;

  void AssignScrollPortEventData(const InternalScrollPortEvent& aEvent,
                                 bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    orient = aEvent.orient;
  }
};





class InternalScrollAreaEvent : public WidgetGUIEvent
{
public:
  InternalScrollAreaEvent(bool aIsTrusted, uint32_t aMessage,
                          nsIWidget* aWidget) :
    WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_SCROLLAREA_EVENT)
  {
  }

  nsRect mArea;

  void AssignScrollAreaEventData(const InternalScrollAreaEvent& aEvent,
                                 bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    mArea = aEvent.mArea;
  }
};








class InternalFormEvent : public WidgetEvent
{
public:
  InternalFormEvent(bool aIsTrusted, uint32_t aMessage) :
    WidgetEvent(aIsTrusted, aMessage, NS_FORM_EVENT),
    originator(nullptr)
  {
  }

  nsIContent *originator;

  void AssignFormEventData(const InternalFormEvent& aEvent, bool aCopyTargets)
  {
    AssignEventData(aEvent, aCopyTargets);

    
  }
};





class InternalClipboardEvent : public WidgetEvent
{
public:
  InternalClipboardEvent(bool aIsTrusted, uint32_t aMessage) :
    WidgetEvent(aIsTrusted, aMessage, NS_CLIPBOARD_EVENT)
  {
  }

  nsCOMPtr<nsIDOMDataTransfer> clipboardData;

  void AssignClipboardEventData(const InternalClipboardEvent& aEvent,
                                bool aCopyTargets)
  {
    AssignEventData(aEvent, aCopyTargets);

    clipboardData = aEvent.clipboardData;
  }
};





class InternalFocusEvent : public InternalUIEvent
{
public:
  InternalFocusEvent(bool aIsTrusted, uint32_t aMessage) :
    InternalUIEvent(aIsTrusted, aMessage, NS_FOCUS_EVENT, 0),
    fromRaise(false), isRefocus(false)
  {
  }

  
  nsCOMPtr<dom::EventTarget> relatedTarget;

  bool fromRaise;
  bool isRefocus;

  void AssignFocusEventData(const InternalFocusEvent& aEvent, bool aCopyTargets)
  {
    AssignUIEventData(aEvent, aCopyTargets);

    relatedTarget = aCopyTargets ? aEvent.relatedTarget : nullptr;
    fromRaise = aEvent.fromRaise;
    isRefocus = aEvent.isRefocus;
  }
};





class InternalTransitionEvent : public WidgetEvent
{
public:
  InternalTransitionEvent(bool aIsTrusted, uint32_t aMessage,
                          const nsAString& aPropertyName, float aElapsedTime,
                          const nsAString& aPseudoElement) :
    nsEvent(aIsTrusted, aMessage, NS_TRANSITION_EVENT),
    propertyName(aPropertyName), elapsedTime(aElapsedTime),
    pseudoElement(aPseudoElement)
  {
  }

  nsString propertyName;
  float elapsedTime;
  nsString pseudoElement;

  void AssignTransitionEventData(const InternalTransitionEvent& aEvent,
                                 bool aCopyTargets)
  {
    AssignEventData(aEvent, aCopyTargets);

    
    
  }
};





class InternalAnimationEvent : public WidgetEvent
{
public:
  InternalAnimationEvent(bool aIsTrusted, uint32_t aMessage,
                         const nsAString& aAnimationName, float aElapsedTime,
                         const nsAString& aPseudoElement) :
    WidgetEvent(aIsTrusted, aMessage, NS_ANIMATION_EVENT),
    animationName(aAnimationName), elapsedTime(aElapsedTime),
    pseudoElement(aPseudoElement)
  {
  }

  nsString animationName;
  float elapsedTime;
  nsString pseudoElement;

  void AssignAnimationEventData(const InternalAnimationEvent& aEvent,
                                bool aCopyTargets)
  {
    AssignEventData(aEvent, aCopyTargets);

    
    
  }
};

} 


typedef mozilla::InternalScriptErrorEvent nsScriptErrorEvent;
typedef mozilla::InternalScrollPortEvent  nsScrollPortEvent;
typedef mozilla::InternalScrollAreaEvent  nsScrollAreaEvent;
typedef mozilla::InternalFormEvent        nsFormEvent;
typedef mozilla::InternalClipboardEvent   nsClipboardEvent;
typedef mozilla::InternalFocusEvent       nsFocusEvent;
typedef mozilla::InternalTransitionEvent  nsTransitionEvent;
typedef mozilla::InternalAnimationEvent   nsAnimationEvent;

#endif

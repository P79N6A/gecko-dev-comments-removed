




#ifndef mozilla_dom_KeyboardEvent_h_
#define mozilla_dom_KeyboardEvent_h_

#include "nsIDOMKeyEvent.h"
#include "nsDOMUIEvent.h"
#include "mozilla/EventForwards.h"
#include "mozilla/dom/KeyboardEventBinding.h"

namespace mozilla {
namespace dom {

class KeyboardEvent : public nsDOMUIEvent,
                      public nsIDOMKeyEvent
{
public:
  KeyboardEvent(EventTarget* aOwner,
                nsPresContext* aPresContext,
                WidgetKeyboardEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMKEYEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return KeyboardEventBinding::Wrap(aCx, aScope, this);
  }

  bool AltKey();
  bool CtrlKey();
  bool ShiftKey();
  bool MetaKey();

  bool GetModifierState(const nsAString& aKey)
  {
    return GetModifierStateInternal(aKey);
  }

  bool Repeat();
  uint32_t CharCode();
  uint32_t KeyCode();
  virtual uint32_t Which() MOZ_OVERRIDE;
  uint32_t Location();

  void InitKeyEvent(const nsAString& aType, bool aCanBubble, bool aCancelable,
                    nsIDOMWindow* aView, bool aCtrlKey, bool aAltKey,
                    bool aShiftKey, bool aMetaKey,
                    uint32_t aKeyCode, uint32_t aCharCode,
                    ErrorResult& aRv)
  {
    aRv = InitKeyEvent(aType, aCanBubble, aCancelable, aView,
                       aCtrlKey, aAltKey, aShiftKey,aMetaKey,
                       aKeyCode, aCharCode);
  }
};

} 
} 

#endif 

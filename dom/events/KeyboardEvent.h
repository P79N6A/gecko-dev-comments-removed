




#ifndef mozilla_dom_KeyboardEvent_h_
#define mozilla_dom_KeyboardEvent_h_

#include "mozilla/dom/UIEvent.h"
#include "mozilla/dom/KeyboardEventBinding.h"
#include "mozilla/EventForwards.h"
#include "nsIDOMKeyEvent.h"

namespace mozilla {
namespace dom {

class KeyboardEvent : public UIEvent,
                      public nsIDOMKeyEvent
{
public:
  KeyboardEvent(EventTarget* aOwner,
                nsPresContext* aPresContext,
                WidgetKeyboardEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMKEYEVENT

  
  NS_FORWARD_TO_UIEVENT

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return KeyboardEventBinding::Wrap(aCx, this);
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

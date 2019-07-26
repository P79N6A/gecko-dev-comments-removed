




#ifndef nsDOMKeyboardEvent_h__
#define nsDOMKeyboardEvent_h__

#include "nsIDOMKeyEvent.h"
#include "nsDOMUIEvent.h"
#include "mozilla/dom/KeyEventBinding.h"

class nsDOMKeyboardEvent : public nsDOMUIEvent,
                           public nsIDOMKeyEvent
{
public:
  nsDOMKeyboardEvent(mozilla::dom::EventTarget* aOwner,
                     nsPresContext* aPresContext, nsKeyEvent* aEvent);
  virtual ~nsDOMKeyboardEvent();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMKEYEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::KeyEventBinding::Wrap(aCx, aScope, this);
  }

  bool AltKey()
  {
    return static_cast<nsInputEvent*>(mEvent)->IsAlt();
  }

  bool CtrlKey()
  {
    return static_cast<nsInputEvent*>(mEvent)->IsControl();
  }

  bool ShiftKey()
  {
    return static_cast<nsInputEvent*>(mEvent)->IsShift();
  }

  bool MetaKey()
  {
    return static_cast<nsInputEvent*>(mEvent)->IsMeta();
  }

  bool GetModifierState(const nsAString& aKey)
  {
    return GetModifierStateInternal(aKey);
  }

  uint32_t CharCode();
  uint32_t KeyCode();
  virtual uint32_t Which() MOZ_OVERRIDE;

  uint32_t Location()
  {
    return static_cast<nsKeyEvent*>(mEvent)->location;
  }

  void InitKeyEvent(const nsAString& aType, bool aCanBubble, bool aCancelable,
                    nsIDOMWindow* aView, bool aCtrlKey, bool aAltKey,
                    bool aShiftKey, bool aMetaKey,
                    uint32_t aKeyCode, uint32_t aCharCode,
                    mozilla::ErrorResult& aRv)
  {
    aRv = InitKeyEvent(aType, aCanBubble, aCancelable, aView,
                       aCtrlKey, aAltKey, aShiftKey,aMetaKey,
                       aKeyCode, aCharCode);
  }

protected:
  
  virtual nsresult Which(uint32_t* aWhich);
};


#endif 

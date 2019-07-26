



#ifndef mozilla_dom_FocusEvent_h_
#define mozilla_dom_FocusEvent_h_

#include "nsDOMUIEvent.h"
#include "nsIDOMFocusEvent.h"
#include "mozilla/EventForwards.h"
#include "mozilla/dom/FocusEventBinding.h"

namespace mozilla {
namespace dom {

class FocusEvent : public nsDOMUIEvent,
                   public nsIDOMFocusEvent
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFOCUSEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return FocusEventBinding::Wrap(aCx, aScope, this);
  }

  FocusEvent(EventTarget* aOwner,
             nsPresContext* aPresContext,
             InternalFocusEvent* aEvent);

  EventTarget* GetRelatedTarget();

  static already_AddRefed<FocusEvent> Constructor(const GlobalObject& aGlobal,
                                                  const nsAString& aType,
                                                  const FocusEventInit& aParam,
                                                  ErrorResult& aRv);
protected:
  nsresult InitFocusEvent(const nsAString& aType,
                          bool aCanBubble,
                          bool aCancelable,
                          nsIDOMWindow* aView,
                          int32_t aDetail,
                          EventTarget* aRelatedTarget);
};

} 
} 

#endif 

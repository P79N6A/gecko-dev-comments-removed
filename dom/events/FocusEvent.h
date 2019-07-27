



#ifndef mozilla_dom_FocusEvent_h_
#define mozilla_dom_FocusEvent_h_

#include "mozilla/dom/FocusEventBinding.h"
#include "mozilla/dom/UIEvent.h"
#include "mozilla/EventForwards.h"
#include "nsIDOMFocusEvent.h"

namespace mozilla {
namespace dom {

class FocusEvent : public UIEvent,
                   public nsIDOMFocusEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMFOCUSEVENT

  
  NS_FORWARD_TO_UIEVENT

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return FocusEventBinding::Wrap(aCx, this);
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
  ~FocusEvent() {}

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

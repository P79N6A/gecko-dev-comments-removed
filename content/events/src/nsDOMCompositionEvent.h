





#ifndef nsDOMCompositionEvent_h__
#define nsDOMCompositionEvent_h__

#include "nsDOMUIEvent.h"
#include "nsIDOMCompositionEvent.h"
#include "mozilla/dom/CompositionEventBinding.h"

class nsDOMCompositionEvent : public nsDOMUIEvent,
                              public nsIDOMCompositionEvent
{
public:
  nsDOMCompositionEvent(mozilla::dom::EventTarget* aOwner,
                        nsPresContext* aPresContext,
                        nsCompositionEvent* aEvent);
  virtual ~nsDOMCompositionEvent();

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_NSDOMUIEVENT
  NS_DECL_NSIDOMCOMPOSITIONEVENT

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope)
  {
    return mozilla::dom::CompositionEventBinding::Wrap(aCx, aScope, this);
  }

  void
  InitCompositionEvent(const nsAString& aType,
                       bool aCanBubble,
                       bool aCancelable,
                       nsIDOMWindow* aView,
                       const nsAString& aData,
                       const nsAString& aLocale,
                       mozilla::ErrorResult& aRv)
  {
    aRv = InitCompositionEvent(aType, aCanBubble, aCancelable, aView,
                               aData, aLocale);
  }

protected:
  nsString mData;
  nsString mLocale;
};

#endif 

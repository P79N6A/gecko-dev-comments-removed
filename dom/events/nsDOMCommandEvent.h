




#ifndef nsDOMCommandEvent_h__
#define nsDOMCommandEvent_h__

#include "nsIDOMCommandEvent.h"
#include "nsDOMEvent.h"
#include "mozilla/dom/CommandEventBinding.h"
#include "mozilla/EventForwards.h"

class nsDOMCommandEvent : public nsDOMEvent,
                          public nsIDOMCommandEvent
{
public:
  nsDOMCommandEvent(mozilla::dom::EventTarget* aOwner,
                    nsPresContext* aPresContext,
                    mozilla::WidgetCommandEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMCOMMANDEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::CommandEventBinding::Wrap(aCx, aScope, this);
  }

  void InitCommandEvent(const nsAString& aType,
                        bool aCanBubble,
                        bool aCancelable,
                        const nsAString& aCommand,
                        mozilla::ErrorResult& aRv)
  {
    aRv = InitCommandEvent(aType, aCanBubble, aCancelable, aCommand);
  }
};

#endif 






#ifndef mozilla_dom_CommandEvent_h_
#define mozilla_dom_CommandEvent_h_

#include "nsIDOMCommandEvent.h"
#include "nsDOMEvent.h"
#include "mozilla/dom/CommandEventBinding.h"
#include "mozilla/EventForwards.h"

namespace mozilla {
namespace dom {

class CommandEvent : public nsDOMEvent,
                     public nsIDOMCommandEvent
{
public:
  CommandEvent(EventTarget* aOwner,
               nsPresContext* aPresContext,
               WidgetCommandEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMCOMMANDEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return CommandEventBinding::Wrap(aCx, aScope, this);
  }

  void InitCommandEvent(const nsAString& aType,
                        bool aCanBubble,
                        bool aCancelable,
                        const nsAString& aCommand,
                        ErrorResult& aRv)
  {
    aRv = InitCommandEvent(aType, aCanBubble, aCancelable, aCommand);
  }
};

} 
} 

#endif 

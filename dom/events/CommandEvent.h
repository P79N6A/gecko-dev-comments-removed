




#ifndef mozilla_dom_CommandEvent_h_
#define mozilla_dom_CommandEvent_h_

#include "mozilla/EventForwards.h"
#include "mozilla/dom/CommandEventBinding.h"
#include "mozilla/dom/Event.h"
#include "nsIDOMCommandEvent.h"

namespace mozilla {
namespace dom {

class CommandEvent : public Event,
                     public nsIDOMCommandEvent
{
public:
  CommandEvent(EventTarget* aOwner,
               nsPresContext* aPresContext,
               WidgetCommandEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMCOMMANDEVENT

  
  NS_FORWARD_TO_EVENT

  virtual JSObject* WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override
  {
    return CommandEventBinding::Wrap(aCx, this, aGivenProto);
  }

  void InitCommandEvent(const nsAString& aType,
                        bool aCanBubble,
                        bool aCancelable,
                        const nsAString& aCommand,
                        ErrorResult& aRv)
  {
    aRv = InitCommandEvent(aType, aCanBubble, aCancelable, aCommand);
  }

protected:
  ~CommandEvent() {}
};

} 
} 

#endif 

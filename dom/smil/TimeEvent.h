




#ifndef mozilla_dom_TimeEvent_h_
#define mozilla_dom_TimeEvent_h_

#include "mozilla/dom/Event.h"
#include "mozilla/dom/TimeEventBinding.h"
#include "nsIDOMTimeEvent.h"

namespace mozilla {
namespace dom {

class TimeEvent MOZ_FINAL : public Event,
                            public nsIDOMTimeEvent
{
public:
  TimeEvent(EventTarget* aOwner,
            nsPresContext* aPresContext,
            InternalSMILTimeEvent* aEvent);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TimeEvent, Event)

  
  NS_DECL_NSIDOMTIMEEVENT

  
  NS_FORWARD_TO_EVENT

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return TimeEventBinding::Wrap(aCx, this);
  }

  int32_t Detail() const
  {
    return mDetail;
  }

  nsIDOMWindow* GetView() const
  {
    return mView;
  }

  void InitTimeEvent(const nsAString& aType, nsIDOMWindow* aView,
                     int32_t aDetail, ErrorResult& aRv)
  {
    aRv = InitTimeEvent(aType, aView, aDetail);
  }

private:
  ~TimeEvent() {}

  nsCOMPtr<nsIDOMWindow> mView;
  int32_t mDetail;
};

} 
} 

#endif 

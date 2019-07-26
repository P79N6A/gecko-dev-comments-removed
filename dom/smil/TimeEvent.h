




#ifndef mozilla_dom_TimeEvent_h_
#define mozilla_dom_TimeEvent_h_

#include "nsIDOMTimeEvent.h"
#include "nsDOMEvent.h"
#include "mozilla/dom/TimeEventBinding.h"

namespace mozilla {
namespace dom {

class TimeEvent MOZ_FINAL : public nsDOMEvent,
                            public nsIDOMTimeEvent
{
public:
  TimeEvent(EventTarget* aOwner,
            nsPresContext* aPresContext,
            WidgetEvent* aEvent);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TimeEvent, nsDOMEvent)

  
  NS_DECL_NSIDOMTIMEEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return TimeEventBinding::Wrap(aCx, aScope, this);
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
  nsCOMPtr<nsIDOMWindow> mView;
  int32_t mDetail;
};

} 
} 

#endif 






#ifndef NS_DOMTIMEEVENT_H_
#define NS_DOMTIMEEVENT_H_

#include "nsIDOMTimeEvent.h"
#include "nsDOMEvent.h"
#include "mozilla/dom/TimeEventBinding.h"

class nsDOMTimeEvent MOZ_FINAL : public nsDOMEvent,
                                 public nsIDOMTimeEvent
{
public:
  nsDOMTimeEvent(mozilla::dom::EventTarget* aOwner,
                 nsPresContext* aPresContext,
                 mozilla::WidgetEvent* aEvent);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMTimeEvent, nsDOMEvent)

  
  NS_DECL_NSIDOMTIMEEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::TimeEventBinding::Wrap(aCx, aScope, this);
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
                     int32_t aDetail, mozilla::ErrorResult& aRv)
  {
    aRv = InitTimeEvent(aType, aView, aDetail);
  }

private:
  nsCOMPtr<nsIDOMWindow> mView;
  int32_t mDetail;
};

#endif 

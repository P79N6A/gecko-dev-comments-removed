



#ifndef mozilla_dom_icc_stkcommandevent_h
#define mozilla_dom_icc_stkcommandevent_h

#include "nsDOMEvent.h"
#include "SimToolKit.h"

namespace mozilla {
namespace dom {
namespace icc {

class StkCommandEvent : public nsDOMEvent,
                        public nsIDOMMozStkCommandEvent
{
  nsString mCommand;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_NSDOMEVENT
  NS_DECL_NSIDOMMOZSTKCOMMANDEVENT
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(StkCommandEvent, nsDOMEvent)

  static already_AddRefed<StkCommandEvent>
  Create(mozilla::dom::EventTarget* aOwner, const nsAString& aMessage);

  nsresult
  Dispatch(nsIDOMEventTarget* aTarget, const nsAString& aEventType)
  {
    NS_ASSERTION(aTarget, "Null pointer!");
    NS_ASSERTION(!aEventType.IsEmpty(), "Empty event type!");

    nsresult rv = InitEvent(aEventType, false, false);
    NS_ENSURE_SUCCESS(rv, rv);

    SetTrusted(true);

    nsDOMEvent* thisEvent = this;

    bool dummy;
    rv = aTarget->DispatchEvent(thisEvent, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

private:
  StkCommandEvent(mozilla::dom::EventTarget* aOwner)
  : nsDOMEvent(aOwner, nullptr, nullptr)
  { }

  ~StkCommandEvent()
  { }
};

}
}
}

#endif 






#ifndef nsDOMDataContainerEvent_h___
#define nsDOMDataContainerEvent_h___

#include "nsIDOMDataContainerEvent.h"
#include "nsDOMEvent.h"
#include "nsInterfaceHashtable.h"

class nsDOMDataContainerEvent : public nsDOMEvent,
                                public nsIDOMDataContainerEvent
{
public:
  nsDOMDataContainerEvent(mozilla::dom::EventTarget* aOwner,
                          nsPresContext* aPresContext, nsEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMDataContainerEvent, nsDOMEvent)

  NS_FORWARD_TO_NSDOMEVENT

  NS_DECL_NSIDOMDATACONTAINEREVENT

private:
  static PLDHashOperator
    TraverseEntry(const nsAString& aKey, nsIVariant *aDataItem, void* aUserArg);

  nsInterfaceHashtable<nsStringHashKey, nsIVariant> mData;
};

#endif


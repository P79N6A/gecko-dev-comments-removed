




#ifndef mozilla_dom_voicemail_VoicemailEvent_h__
#define mozilla_dom_voicemail_VoicemailEvent_h__

#include "nsIDOMMozVoicemailEvent.h"
#include "nsDOMEvent.h"

class nsIDOMMozVoicemailStatus;

namespace mozilla {
namespace dom {

class VoicemailEvent : public nsIDOMMozVoicemailEvent,
                       public nsDOMEvent
{
public:
  VoicemailEvent(nsPresContext* aPresContext, nsEvent* aEvent)
    : nsDOMEvent(aPresContext, aEvent) { }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMOZVOICEMAILEVENT

  NS_FORWARD_TO_NSDOMEVENT

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(VoicemailEvent, nsDOMEvent)

  nsresult InitVoicemailEvent(const nsAString& aEventTypeArg,
                              bool aCanBubbleArg, bool aCancelableArg,
                              nsIDOMMozVoicemailStatus* aStatus);

private:
  nsCOMPtr<nsIDOMMozVoicemailStatus> mStatus;
};

} 
} 

#endif 

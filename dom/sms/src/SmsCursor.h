




































#ifndef mozilla_dom_sms_SmsCursor_h
#define mozilla_dom_sms_SmsCursor_h

#include "nsIDOMSmsCursor.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"

class nsIDOMMozSmsFilter;
class nsIDOMMozSmsMessage;
class nsIDOMMozSmsRequest;

namespace mozilla {
namespace dom {
namespace sms {

class SmsCursor : public nsIDOMMozSmsCursor
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSCURSOR

  NS_DECL_CYCLE_COLLECTION_CLASS(SmsCursor)

  SmsCursor(nsIDOMMozSmsFilter* aFilter);
  SmsCursor(nsIDOMMozSmsFilter* aFilter, nsIDOMMozSmsRequest* aRequest);

  void SetMessage(nsIDOMMozSmsMessage* aMessage);

private:
  nsCOMPtr<nsIDOMMozSmsFilter>  mFilter;
  nsCOMPtr<nsIDOMMozSmsRequest> mRequest;
  nsCOMPtr<nsIDOMMozSmsMessage> mMessage;
};

inline void
SmsCursor::SetMessage(nsIDOMMozSmsMessage* aMessage)
{
  mMessage = aMessage;
}

} 
} 
} 

#endif 

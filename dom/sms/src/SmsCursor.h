




#ifndef mozilla_dom_sms_SmsCursor_h
#define mozilla_dom_sms_SmsCursor_h

#include "nsIDOMSmsCursor.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"

class nsIDOMMozSmsMessage;
class nsIDOMMozSmsRequest;

namespace mozilla {
namespace dom {
namespace sms {

class SmsCursor MOZ_FINAL : public nsIDOMMozSmsCursor
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSCURSOR

  NS_DECL_CYCLE_COLLECTION_CLASS(SmsCursor)

  SmsCursor();
  SmsCursor(int32_t aListId, nsIDOMMozSmsRequest* aRequest);

  ~SmsCursor();

  void SetMessage(nsIDOMMozSmsMessage* aMessage);

  void Disconnect();

private:
  int32_t                       mListId;
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

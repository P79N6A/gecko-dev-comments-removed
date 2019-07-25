




































#ifndef mozilla_dom_sms_SmsParent_h
#define mozilla_dom_sms_SmsParent_h

#include "mozilla/dom/sms/PSmsParent.h"
#include "nsIObserver.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsParent : public PSmsParent
                , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  SmsParent();

  NS_OVERRIDE virtual bool RecvHasSupport(bool* aHasSupport);
  NS_OVERRIDE virtual bool RecvGetNumberOfMessagesForText(const nsString& aText, PRUint16* aResult);
  NS_OVERRIDE virtual bool RecvSendMessage(const nsString& aNumber, const nsString& aMessage);

protected:
  virtual void ActorDestroy(ActorDestroyReason why);
};

} 
} 
} 

#endif 

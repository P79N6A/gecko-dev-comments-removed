




































#ifndef mozilla_dom_sms_SmsParent_h
#define mozilla_dom_sms_SmsParent_h

#include "mozilla/dom/sms/PSmsParent.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsParent : public PSmsParent
{
  NS_OVERRIDE virtual bool RecvHasSupport(bool* aHasSupport);
  NS_OVERRIDE virtual bool RecvGetNumberOfMessagesForText(const nsString& aText, PRUint16* aResult);
  NS_OVERRIDE virtual bool RecvSendMessage(const nsString& aNumber, const nsString& aMessage);
};

} 
} 
} 

#endif 






































#ifndef mozilla_dom_sms_SmsChild_h
#define mozilla_dom_sms_SmsChild_h

#include "mozilla/dom/sms/PSmsChild.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsChild : public PSmsChild
{
public:
  NS_OVERRIDE virtual bool RecvNotifyReceivedMessage(const SmsMessageData& aMessage);
  NS_OVERRIDE virtual bool RecvNotifySentMessage(const SmsMessageData& aMessage);
  NS_OVERRIDE virtual bool RecvNotifyDeliveredMessage(const SmsMessageData& aMessage);
  NS_OVERRIDE virtual bool RecvNotifyRequestSmsSent(const SmsMessageData& aMessage, const PRInt32& aRequestId, const PRUint64& aProcessId);
};

} 
} 
} 

#endif 

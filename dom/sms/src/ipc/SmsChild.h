




#ifndef mozilla_dom_sms_SmsChild_h
#define mozilla_dom_sms_SmsChild_h

#include "mozilla/dom/sms/PSmsChild.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsChild : public PSmsChild
{
public:
  virtual bool RecvNotifyReceivedMessage(const SmsMessageData& aMessage) MOZ_OVERRIDE;
  virtual bool RecvNotifySentMessage(const SmsMessageData& aMessage) MOZ_OVERRIDE;
  virtual bool RecvNotifyDeliveredMessage(const SmsMessageData& aMessage) MOZ_OVERRIDE;
  virtual bool RecvNotifyRequestSmsSent(const SmsMessageData& aMessage, const PRInt32& aRequestId, const PRUint64& aProcessId) MOZ_OVERRIDE;
  virtual bool RecvNotifyRequestSmsSendFailed(const PRInt32& aError, const PRInt32& aRequestId, const PRUint64& aProcessId) MOZ_OVERRIDE;
  virtual bool RecvNotifyRequestGotSms(const SmsMessageData& aMessage, const PRInt32& aRequestId, const PRUint64& aProcessId) MOZ_OVERRIDE;
  virtual bool RecvNotifyRequestGetSmsFailed(const PRInt32& aError, const PRInt32& aRequestId, const PRUint64& aProcessId) MOZ_OVERRIDE;
  virtual bool RecvNotifyRequestSmsDeleted(const bool& aDeleted, const PRInt32& aRequestId, const PRUint64& aProcessId) MOZ_OVERRIDE;
  virtual bool RecvNotifyRequestSmsDeleteFailed(const PRInt32& aError, const PRInt32& aRequestId, const PRUint64& aProcessId) MOZ_OVERRIDE;
  virtual bool RecvNotifyRequestNoMessageInList(const PRInt32& aRequestId, const PRUint64& aProcessId) MOZ_OVERRIDE;
  virtual bool RecvNotifyRequestCreateMessageList(const PRInt32& aListId, const SmsMessageData& aMessage, const PRInt32& aRequestId, const PRUint64& aProcessId) MOZ_OVERRIDE;
  virtual bool RecvNotifyRequestGotNextMessage(const SmsMessageData& aMessage, const PRInt32& aRequestId, const PRUint64& aProcessId) MOZ_OVERRIDE;
  virtual bool RecvNotifyRequestReadListFailed(const PRInt32& aError, const PRInt32& aRequestId, const PRUint64& aProcessId) MOZ_OVERRIDE;
  virtual bool RecvNotifyRequestMarkedMessageRead(const bool& aRead, const PRInt32& aRequestId, const PRUint64& aProcessId) MOZ_OVERRIDE;
  virtual bool RecvNotifyRequestMarkMessageReadFailed(const PRInt32& aError, const PRInt32& aRequestId, const PRUint64& aProcessId) MOZ_OVERRIDE;
};

} 
} 
} 

#endif 

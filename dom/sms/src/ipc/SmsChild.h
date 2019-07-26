




#ifndef mozilla_dom_sms_SmsChild_h
#define mozilla_dom_sms_SmsChild_h

#include "mozilla/dom/sms/PSmsChild.h"
#include "mozilla/dom/sms/PSmsRequestChild.h"

class nsISmsRequest;

namespace mozilla {
namespace dom {
namespace sms {

class SmsChild : public PSmsChild
{
public:
  SmsChild();

protected:
  virtual ~SmsChild();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyReceivedMessage(const SmsMessageData& aMessage) MOZ_OVERRIDE;

  virtual bool
  RecvNotifySendingMessage(const SmsMessageData& aMessage) MOZ_OVERRIDE;

  virtual bool
  RecvNotifySentMessage(const SmsMessageData& aMessage) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyFailedMessage(const SmsMessageData& aMessage) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyDeliverySuccessMessage(const SmsMessageData& aMessage) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyDeliveryErrorMessage(const SmsMessageData& aMessage) MOZ_OVERRIDE;

  virtual PSmsRequestChild*
  AllocPSmsRequest(const IPCSmsRequest& aRequest) MOZ_OVERRIDE;

  virtual bool
  DeallocPSmsRequest(PSmsRequestChild* aActor) MOZ_OVERRIDE;
};

class SmsRequestChild : public PSmsRequestChild
{
  friend class mozilla::dom::sms::SmsChild;

  nsCOMPtr<nsISmsRequest> mReplyRequest;

public:
  SmsRequestChild(nsISmsRequest* aReplyRequest);

protected:
  virtual ~SmsRequestChild();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  Recv__delete__(const MessageReply& aReply) MOZ_OVERRIDE;
};

} 
} 
} 

#endif 






#ifndef mozilla_dom_mobilemessage_SmsChild_h
#define mozilla_dom_mobilemessage_SmsChild_h

#include "mozilla/dom/mobilemessage/PSmsChild.h"
#include "mozilla/dom/mobilemessage/PSmsRequestChild.h"
#include "mozilla/dom/mobilemessage/PMobileMessageCursorChild.h"
#include "nsIDOMDOMCursor.h"
#include "nsIMobileMessageCallback.h"
#include "nsIMobileMessageCursorCallback.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

class SmsChild : public PSmsChild
{
public:
  SmsChild()
  {
    MOZ_COUNT_CTOR(SmsChild);
  }

protected:
  virtual ~SmsChild()
  {
    MOZ_COUNT_DTOR(SmsChild);
  }

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

  virtual PMobileMessageCursorChild*
  AllocPMobileMessageCursor(const CreateMessageCursorRequest& aRequest) MOZ_OVERRIDE;

  virtual bool
  DeallocPMobileMessageCursor(PMobileMessageCursorChild* aActor) MOZ_OVERRIDE;
};

class SmsRequestChild : public PSmsRequestChild
{
  friend class SmsChild;

  nsCOMPtr<nsIMobileMessageCallback> mReplyRequest;

public:
  SmsRequestChild(nsIMobileMessageCallback* aReplyRequest);

protected:
  virtual ~SmsRequestChild()
  {
    MOZ_COUNT_DTOR(SmsRequestChild);
  }

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  Recv__delete__(const MessageReply& aReply) MOZ_OVERRIDE;
};

class MobileMessageCursorChild : public PMobileMessageCursorChild
                               , public nsICursorContinueCallback
{
  friend class SmsChild;

  nsCOMPtr<nsIMobileMessageCursorCallback> mCursorCallback;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICURSORCONTINUECALLBACK

  MobileMessageCursorChild(nsIMobileMessageCursorCallback* aCallback);

protected:
  virtual ~MobileMessageCursorChild()
  {
    MOZ_COUNT_DTOR(MobileMessageCursorChild);
  }

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyResult(const SmsMessageData& aMessageData) MOZ_OVERRIDE;

  virtual bool
  Recv__delete__(const int32_t& aError) MOZ_OVERRIDE;
};

} 
} 
} 

#endif 

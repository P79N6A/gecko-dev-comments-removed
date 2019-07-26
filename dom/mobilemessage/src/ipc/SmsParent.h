




#ifndef mozilla_dom_mobilemessage_SmsParent_h
#define mozilla_dom_mobilemessage_SmsParent_h

#include "mozilla/dom/mobilemessage/PSmsParent.h"
#include "mozilla/dom/mobilemessage/PSmsRequestParent.h"
#include "mozilla/dom/mobilemessage/PMobileMessageCursorParent.h"
#include "nsIDOMDOMCursor.h"
#include "nsIMobileMessageCursorCallback.h"
#include "nsIObserver.h"

namespace mozilla {
namespace dom {

class ContentParent;
class SmsRequest;

namespace mobilemessage {

class SmsParent : public PSmsParent
                , public nsIObserver
{
  friend class mozilla::dom::ContentParent;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

protected:
  virtual bool
  RecvHasSupport(bool* aHasSupport) MOZ_OVERRIDE;

  virtual bool
  RecvGetSegmentInfoForText(const nsString& aText, SmsSegmentInfoData* aResult) MOZ_OVERRIDE;

  SmsParent();
  virtual ~SmsParent()
  {
    MOZ_COUNT_DTOR(SmsParent);
  }

  virtual void
  ActorDestroy(ActorDestroyReason why);

  virtual bool
  RecvPSmsRequestConstructor(PSmsRequestParent* aActor,
                             const IPCSmsRequest& aRequest) MOZ_OVERRIDE;

  virtual PSmsRequestParent*
  AllocPSmsRequest(const IPCSmsRequest& aRequest) MOZ_OVERRIDE;

  virtual bool
  DeallocPSmsRequest(PSmsRequestParent* aActor) MOZ_OVERRIDE;

  virtual bool
  RecvPMobileMessageCursorConstructor(PMobileMessageCursorParent* aActor,
                                      const IPCMobileMessageCursor& aCursor) MOZ_OVERRIDE;

  virtual PMobileMessageCursorParent*
  AllocPMobileMessageCursor(const IPCMobileMessageCursor& aCursor) MOZ_OVERRIDE;

  virtual bool
  DeallocPMobileMessageCursor(PMobileMessageCursorParent* aActor) MOZ_OVERRIDE;
};

class SmsRequestParent : public PSmsRequestParent
{
  friend class SmsParent;

  nsRefPtr<SmsRequest> mSmsRequest;

public:
  void
  SendReply(const MessageReply& aReply);

protected:
  SmsRequestParent();
  virtual ~SmsRequestParent();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  bool
  DoRequest(const SendMessageRequest& aRequest);

  bool
  DoRequest(const GetMessageRequest& aRequest);

  bool
  DoRequest(const DeleteMessageRequest& aRequest);

  bool
  DoRequest(const MarkMessageReadRequest& aRequest);
};

class MobileMessageCursorParent : public PMobileMessageCursorParent
                                , public nsIMobileMessageCursorCallback
{
  friend class SmsParent;

  nsCOMPtr<nsICursorContinueCallback> mContinueCallback;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILEMESSAGECURSORCALLBACK

protected:
  MobileMessageCursorParent()
  {
    MOZ_COUNT_CTOR(MobileMessageCursorParent);
  }

  virtual ~MobileMessageCursorParent()
  {
    MOZ_COUNT_DTOR(MobileMessageCursorParent);
  }

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  RecvContinue() MOZ_OVERRIDE;

  bool
  DoRequest(const CreateMessageCursorRequest& aRequest);

  bool
  DoRequest(const CreateThreadCursorRequest& aRequest);
};

} 
} 
} 

#endif 

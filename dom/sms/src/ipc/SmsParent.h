




#ifndef mozilla_dom_sms_SmsParent_h
#define mozilla_dom_sms_SmsParent_h

#include "mozilla/dom/sms/PSmsParent.h"
#include "mozilla/dom/sms/PSmsRequestParent.h"
#include "nsIObserver.h"

namespace mozilla {
namespace dom {

class ContentParent;

} 
} 

namespace mozilla {
namespace dom {
namespace sms {

class SmsRequest;

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
  RecvGetNumberOfMessagesForText(const nsString& aText, uint16_t* aResult) MOZ_OVERRIDE;

  virtual bool
  RecvSaveReceivedMessage(const nsString& aSender, const nsString& aBody, const nsString& aMessageClass, const uint64_t& aDate, int32_t* aId) MOZ_OVERRIDE;

  virtual bool
  RecvSaveSentMessage(const nsString& aRecipient, const nsString& aBody, const uint64_t& aDate, int32_t* aId) MOZ_OVERRIDE;

  virtual bool
  RecvSetMessageDeliveryStatus(const int32_t& aMessageId, const nsString& aDeliveryStatus) MOZ_OVERRIDE;

  virtual bool
  RecvClearMessageList(const int32_t& aListId) MOZ_OVERRIDE;

  SmsParent();
  virtual ~SmsParent();

  virtual void
  ActorDestroy(ActorDestroyReason why);

  virtual bool
  RecvPSmsRequestConstructor(PSmsRequestParent* aActor,
                             const IPCSmsRequest& aRequest) MOZ_OVERRIDE;

  virtual PSmsRequestParent*
  AllocPSmsRequest(const IPCSmsRequest& aRequest) MOZ_OVERRIDE;

  virtual bool
  DeallocPSmsRequest(PSmsRequestParent* aActor) MOZ_OVERRIDE;
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
  DoRequest(const CreateMessageListRequest& aRequest);

  bool
  DoRequest(const GetNextMessageInListRequest& aRequest);

  bool
  DoRequest(const MarkMessageReadRequest& aRequest);
};

} 
} 
} 

#endif 

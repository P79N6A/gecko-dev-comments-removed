




#ifndef mozilla_dom_mobilemessage_SmsParent_h
#define mozilla_dom_mobilemessage_SmsParent_h

#include "mozilla/dom/mobilemessage/PSmsParent.h"
#include "mozilla/dom/mobilemessage/PSmsRequestParent.h"
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

  bool
  DoRequest(const GetThreadListRequest& aRequest);
};

} 
} 
} 

#endif 

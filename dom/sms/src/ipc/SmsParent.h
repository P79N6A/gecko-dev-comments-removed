




































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

  static void GetAll(nsTArray<SmsParent*>& aArray);

  SmsParent();

  NS_OVERRIDE virtual bool RecvHasSupport(bool* aHasSupport);
  NS_OVERRIDE virtual bool RecvGetNumberOfMessagesForText(const nsString& aText, PRUint16* aResult);
  NS_OVERRIDE virtual bool RecvSendMessage(const nsString& aNumber, const nsString& aMessage, const PRInt32& aRequestId, const PRUint64& aProcessId);
  NS_OVERRIDE virtual bool RecvSaveSentMessage(const nsString& aRecipient, const nsString& aBody, const PRUint64& aDate, PRInt32* aId);
  NS_OVERRIDE virtual bool RecvGetMessage(const PRInt32& aMessageId, const PRInt32& aRequestId, const PRUint64& aProcessId);

protected:
  virtual void ActorDestroy(ActorDestroyReason why);

private:
  static nsTArray<SmsParent*>* gSmsParents;
};

} 
} 
} 

#endif 





#ifndef mozilla_dom_Icc_h
#define mozilla_dom_Icc_h

#include "nsDOMEventTargetHelper.h"
#include "nsIIccProvider.h"

namespace mozilla {
namespace dom {

class Icc MOZ_FINAL : public nsDOMEventTargetHelper
{
public:
  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  Icc(nsPIDOMWindow* aWindow, long aClientId, const nsAString& aIccId);

  void
  Shutdown();

  nsresult
  NotifyEvent(const nsAString& aName);

  nsresult
  NotifyStkEvent(const nsAString& aName, const nsAString& aMessage);

  nsString
  GetIccId()
  {
    return mIccId;
  }

  nsPIDOMWindow*
  GetParentObject() const
  {
    return GetOwner();
  }

  
  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  already_AddRefed<nsIDOMMozIccInfo>
  GetIccInfo() const;

  void
  GetCardState(nsString& aCardState) const;

  void
  SendStkResponse(const JSContext* aCx, const JS::Value& aCommand,
                  const JS::Value& aResponse, ErrorResult& aRv);

  void
  SendStkMenuSelection(uint16_t aItemIdentifier, bool aHelpRequested,
                       ErrorResult& aRv);

  void
  SendStkTimerExpiration(const JSContext* aCx, const JS::Value& aTimer,
                         ErrorResult& aRv);

  void
  SendStkEventDownload(const JSContext* aCx, const JS::Value& aEvent,
                       ErrorResult& aRv);

  already_AddRefed<nsISupports>
  GetCardLock(const nsAString& aLockType, ErrorResult& aRv);

  already_AddRefed<nsISupports>
  UnlockCardLock(const JSContext* aCx, const JS::Value& aInfo,
                 ErrorResult& aRv);

  already_AddRefed<nsISupports>
  SetCardLock(const JSContext* aCx, const JS::Value& aInfo, ErrorResult& aRv);

  already_AddRefed<nsISupports>
  GetCardLockRetryCount(const nsAString& aLockType, ErrorResult& aRv);

  already_AddRefed<nsISupports>
  ReadContacts(const nsAString& aContactType, ErrorResult& aRv);

  already_AddRefed<nsISupports>
  UpdateContact(const JSContext* aCx, const nsAString& aContactType,
                const JS::Value& aContact, const nsAString& aPin2,
                ErrorResult& aRv);

  already_AddRefed<nsISupports>
  IccOpenChannel(const nsAString& aAid, ErrorResult& aRv);

  already_AddRefed<nsISupports>
  IccExchangeAPDU(const JSContext* aCx, int32_t aChannel, const jsval& aApdu,
                  ErrorResult& aRv);

  already_AddRefed<nsISupports>
  IccCloseChannel(int32_t aChannel, ErrorResult& aRv);

  IMPL_EVENT_HANDLER(iccinfochange)
  IMPL_EVENT_HANDLER(cardstatechange)
  IMPL_EVENT_HANDLER(stkcommand)
  IMPL_EVENT_HANDLER(stksessionend)

private:
  bool mLive;
  uint32_t mClientId;
  nsString mIccId;
  
  
  nsCOMPtr<nsIIccProvider> mProvider;
};

} 
} 

#endif 

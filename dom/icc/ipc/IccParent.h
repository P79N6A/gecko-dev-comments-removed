



#ifndef mozilla_dom_icc_IccParent_h
#define mozilla_dom_icc_IccParent_h

#include "mozilla/dom/icc/PIccParent.h"
#include "mozilla/dom/icc/PIccRequestParent.h"
#include "nsIIccService.h"

namespace mozilla {
namespace dom {
namespace icc {

class IccParent MOZ_FINAL : public PIccParent
                          , public nsIIccListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIICCLISTENER

  explicit IccParent(uint32_t aServiceId);

protected:
  virtual
  
  ~IccParent()
  {
    MOZ_COUNT_DTOR(IccParent);
  }

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  RecvInit(
          OptionalIccInfoData* aInfoData,
          uint32_t* aCardState) MOZ_OVERRIDE;

  virtual PIccRequestParent*
  AllocPIccRequestParent(const IccRequest& aRequest) MOZ_OVERRIDE;

  virtual bool
  DeallocPIccRequestParent(PIccRequestParent* aActor) MOZ_OVERRIDE;

  virtual bool
  RecvPIccRequestConstructor(PIccRequestParent* aActor,
                             const IccRequest& aRequest) MOZ_OVERRIDE;

private:
  IccParent();
  nsCOMPtr<nsIIcc> mIcc;
};

class IccRequestParent MOZ_FINAL : public PIccRequestParent
                                 , public nsIIccCallback
{
  friend class IccParent;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIICCCALLBACK

  explicit IccRequestParent(nsIIcc* icc);

protected:
  virtual void
  ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

private:
  
  ~IccRequestParent()
  {
    MOZ_COUNT_DTOR(IccRequestParent);
  }

  bool
  DoRequest(const GetCardLockEnabledRequest& aRequest);

  bool
  DoRequest(const UnlockCardLockRequest& aRequest);

  bool
  DoRequest(const SetCardLockEnabledRequest& aRequest);

  bool
  DoRequest(const ChangeCardLockPasswordRequest& aRequest);

  bool
  DoRequest(const GetCardLockRetryCountRequest& aRequest);

  bool
  DoRequest(const MatchMvnoRequest& aRequest);

  bool
  DoRequest(const GetServiceStateEnabledRequest& aRequest);

  nsresult
  SendReply(const IccReply& aReply);

  nsCOMPtr<nsIIcc> mIcc;
};

} 
} 
} 

#endif 

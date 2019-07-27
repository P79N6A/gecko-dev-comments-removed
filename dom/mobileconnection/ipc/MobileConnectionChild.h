



#ifndef mozilla_dom_mobileconnection_MobileConnectionChild_h
#define mozilla_dom_mobileconnection_MobileConnectionChild_h

#include "mozilla/dom/MobileConnectionCallback.h"
#include "mozilla/dom/MobileConnectionInfo.h"
#include "mozilla/dom/PMobileConnectionChild.h"
#include "mozilla/dom/PMobileConnectionRequestChild.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsIMobileConnectionService.h"
#include "nsIVariant.h"

namespace mozilla {
namespace dom {
namespace mobileconnection {







class MobileConnectionChild : public PMobileConnectionChild
{
  NS_INLINE_DECL_REFCOUNTING(MobileConnectionChild)

public:
  MobileConnectionChild()
    : mLive(true)
  {
    MOZ_COUNT_CTOR(MobileConnectionChild);
  }

  void
  Init();

  void
  Shutdown();

  void
  RegisterListener(nsIMobileConnectionListener* aListener);

  void
  UnregisterListener(nsIMobileConnectionListener* aListener);

  MobileConnectionInfo*
  GetVoiceInfo();

  MobileConnectionInfo*
  GetDataInfo();

  void
  GetIccId(nsAString& aIccId);

  void
  GetRadioState(nsAString& aRadioState);

  nsIVariant*
  GetSupportedNetworkTypes();

  void
  GetLastNetwork(nsAString& aNetwork);

  void
  GetLastHomeNetwork(nsAString& aNetwork);

  void
  GetNetworkSelectionMode(nsAString& aMode);

protected:
  virtual
  ~MobileConnectionChild()
  {
    MOZ_COUNT_DTOR(MobileConnectionChild);
    Shutdown();
  }

  virtual void
  ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  virtual PMobileConnectionRequestChild*
  AllocPMobileConnectionRequestChild(const MobileConnectionRequest& request) MOZ_OVERRIDE;

  virtual bool
  DeallocPMobileConnectionRequestChild(PMobileConnectionRequestChild* aActor) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyVoiceInfoChanged(nsIMobileConnectionInfo* const& aInfo) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyDataInfoChanged(nsIMobileConnectionInfo* const& aInfo) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyUssdReceived(const nsString& aMessage,
                         const bool& aSessionEnd) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyDataError(const nsString& aMessage) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyCFStateChanged(const bool& aSuccess, const uint16_t& aAction,
                           const uint16_t& aReason, const nsString& aNumber,
                           const uint16_t& aTimeSeconds, const uint16_t& aServiceClass) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyEmergencyCbModeChanged(const bool& aActive,
                                   const uint32_t& aTimeoutMs) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyOtaStatusChanged(const nsString& aStatus) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyIccChanged(const nsString& aIccId) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyRadioStateChanged(const nsString& aRadioState) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyClirModeChanged(const uint32_t& aMode) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyLastNetworkChanged(const nsString& aNetwork) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyLastHomeNetworkChanged(const nsString& aNetwork) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyNetworkSelectionModeChanged(const nsString& aMode) MOZ_OVERRIDE;

private:
  bool mLive;
  nsCOMArray<nsIMobileConnectionListener> mListeners;
  nsCOMPtr<nsIWritableVariant> mSupportedNetworkTypes;
  nsRefPtr<MobileConnectionInfo> mVoice;
  nsRefPtr<MobileConnectionInfo> mData;
  nsString mIccId;
  nsString mRadioState;
  nsString mLastNetwork;
  nsString mLastHomeNetwork;
  nsString mNetworkSelectionMode;
};










class MobileConnectionRequestChild : public PMobileConnectionRequestChild
{
public:
  MobileConnectionRequestChild(nsIMobileConnectionCallback* aRequestCallback)
    : mRequestCallback(aRequestCallback)
  {
    MOZ_COUNT_CTOR(MobileConnectionRequestChild);
    MOZ_ASSERT(mRequestCallback);
  }

  bool
  DoReply(const MobileConnectionReplySuccess& aReply);

  bool
  DoReply(const MobileConnectionReplySuccessString& aReply);

  bool
  DoReply(const MobileConnectionReplySuccessBoolean& aReply);

  bool
  DoReply(const MobileConnectionReplySuccessNetworks& aReply);

  bool
  DoReply(const MobileConnectionReplySuccessMmi& aReply);

  bool
  DoReply(const MobileConnectionReplySuccessCallForwarding& aReply);

  bool
  DoReply(const MobileConnectionReplySuccessCallBarring& aReply);

  bool
  DoReply(const MobileConnectionReplySuccessClirStatus& aReply);

  bool
  DoReply(const MobileConnectionReplyError& aReply);

  bool
  DoReply(const MobileConnectionReplyErrorMmi& aReply);

protected:
  virtual
  ~MobileConnectionRequestChild()
  {
    MOZ_COUNT_DTOR(MobileConnectionRequestChild);
  }

  virtual void
  ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  virtual bool
  Recv__delete__(const MobileConnectionReply& aReply) MOZ_OVERRIDE;

private:
  nsCOMPtr<nsIMobileConnectionCallback> mRequestCallback;
};

} 
} 
} 

#endif 

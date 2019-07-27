



#ifndef mozilla_dom_mobileconnection_MobileConnectionParent_h
#define mozilla_dom_mobileconnection_MobileConnectionParent_h

#include "mozilla/dom/PMobileConnectionParent.h"
#include "mozilla/dom/PMobileConnectionRequestParent.h"
#include "nsIMobileConnectionInfo.h"
#include "nsIMobileConnectionService.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {
namespace mobileconnection {





class MobileConnectionParent : public PMobileConnectionParent
                             , public nsIMobileConnectionListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILECONNECTIONLISTENER

  explicit MobileConnectionParent(uint32_t aClientId);

protected:
  virtual
  ~MobileConnectionParent()
  {
    MOZ_COUNT_DTOR(MobileConnectionParent);
  }

  virtual void
  ActorDestroy(ActorDestroyReason why);

  virtual bool
  RecvPMobileConnectionRequestConstructor(PMobileConnectionRequestParent* aActor,
                                          const MobileConnectionRequest& aRequest) MOZ_OVERRIDE;

  virtual PMobileConnectionRequestParent*
  AllocPMobileConnectionRequestParent(const MobileConnectionRequest& request) MOZ_OVERRIDE;

  virtual bool
  DeallocPMobileConnectionRequestParent(PMobileConnectionRequestParent* aActor) MOZ_OVERRIDE;

  virtual bool
  RecvInit(nsMobileConnectionInfo* aVoice, nsMobileConnectionInfo* aData,
           nsString* aLastKnownNetwork, nsString* aLastKnownHomeNetwork,
           nsString* aIccId, nsString* aNetworkSelectionMode,
           nsString* aRadioState, nsTArray<nsString>* aSupportedNetworkTypes) MOZ_OVERRIDE;

private:
  uint32_t mClientId;
  bool mLive;
  nsCOMPtr<nsIMobileConnectionService> mService;
};












class MobileConnectionRequestParent : public PMobileConnectionRequestParent
                                    , public nsIMobileConnectionCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILECONNECTIONCALLBACK

  explicit MobileConnectionRequestParent(uint32_t aClientId)
    : mClientId(aClientId)
    , mLive(true)
  {
    MOZ_COUNT_CTOR(MobileConnectionRequestParent);

    mService = do_GetService(NS_MOBILE_CONNECTION_SERVICE_CONTRACTID);
    NS_ASSERTION(mService, "This shouldn't fail!");
  }

  bool
  DoRequest(const GetNetworksRequest& aRequest);

  bool
  DoRequest(const SelectNetworkRequest& aRequest);

  bool
  DoRequest(const SelectNetworkAutoRequest& aRequest);

  bool
  DoRequest(const SetPreferredNetworkTypeRequest& aRequest);

  bool
  DoRequest(const GetPreferredNetworkTypeRequest& aRequest);

  bool
  DoRequest(const SetRoamingPreferenceRequest& aRequest);

  bool
  DoRequest(const GetRoamingPreferenceRequest& aRequest);

  bool
  DoRequest(const SetVoicePrivacyModeRequest& aRequest);

  bool
  DoRequest(const GetVoicePrivacyModeRequest& aRequest);

  bool
  DoRequest(const SendMmiRequest& aRequest);

  bool
  DoRequest(const CancelMmiRequest& aRequest);

  bool
  DoRequest(const SetCallForwardingRequest& aRequest);

  bool
  DoRequest(const GetCallForwardingRequest& aRequest);

  bool
  DoRequest(const SetCallBarringRequest& aRequest);

  bool
  DoRequest(const GetCallBarringRequest& aRequest);

  bool
  DoRequest(const ChangeCallBarringPasswordRequest& aRequest);

  bool
  DoRequest(const SetCallWaitingRequest& aRequest);

  bool
  DoRequest(const GetCallWaitingRequest& aRequest);

  bool
  DoRequest(const SetCallingLineIdRestrictionRequest& aRequest);

  bool
  DoRequest(const GetCallingLineIdRestrictionRequest& aRequest);

  bool
  DoRequest(const ExitEmergencyCbModeRequest& aRequest);

  bool
  DoRequest(const SetRadioEnabledRequest& aRequest);

protected:
  virtual
  ~MobileConnectionRequestParent()
  {
    MOZ_COUNT_DTOR(MobileConnectionRequestParent);
  }

  virtual void
  ActorDestroy(ActorDestroyReason why);

  nsresult
  SendReply(const MobileConnectionReply& aReply);

private:
  uint32_t mClientId;
  bool mLive;
  nsCOMPtr<nsIMobileConnectionService> mService;
};

} 
} 
} 

#endif 

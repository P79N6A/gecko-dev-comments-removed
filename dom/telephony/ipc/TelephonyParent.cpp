




#include "mozilla/dom/telephony/TelephonyParent.h"
#include "nsServiceManagerUtils.h"

USING_TELEPHONY_NAMESPACE





NS_IMPL_ISUPPORTS1(TelephonyParent, nsITelephonyListener)

TelephonyParent::TelephonyParent()
  : mActorDestroyed(false)
  , mRegistered(false)
{
}

void
TelephonyParent::ActorDestroy(ActorDestroyReason why)
{
  
  
  
  mActorDestroyed = true;

  
  RecvUnregisterListener();
}

bool
TelephonyParent::RecvPTelephonyRequestConstructor(PTelephonyRequestParent* aActor,
                                                  const IPCTelephonyRequest& aRequest)
{
  TelephonyRequestParent* actor = static_cast<TelephonyRequestParent*>(aActor);

  switch (aRequest.type()) {
    case IPCTelephonyRequest::TEnumerateCallsRequest:
      return actor->DoRequest(aRequest.get_EnumerateCallsRequest());
    case IPCTelephonyRequest::TDialRequest:
      return actor->DoRequest(aRequest.get_DialRequest());
    default:
      MOZ_CRASH("Unknown type!");
  }

  return false;
}

PTelephonyRequestParent*
TelephonyParent::AllocPTelephonyRequestParent(const IPCTelephonyRequest& aRequest)
{
  TelephonyRequestParent* actor = new TelephonyRequestParent();
  
  
  NS_ADDREF(actor);

  return actor;
}

bool
TelephonyParent::DeallocPTelephonyRequestParent(PTelephonyRequestParent* aActor)
{
  
  static_cast<TelephonyRequestParent*>(aActor)->Release();
  return true;
}

bool
TelephonyParent::Recv__delete__()
{
  return true; 
}

bool
TelephonyParent::RecvRegisterListener()
{
  NS_ENSURE_TRUE(!mRegistered, true);

  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  mRegistered = NS_SUCCEEDED(provider->RegisterListener(this));
  return true;
}

bool
TelephonyParent::RecvUnregisterListener()
{
  NS_ENSURE_TRUE(mRegistered, true);

  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  mRegistered = !NS_SUCCEEDED(provider->UnregisterListener(this));
  return true;
}

bool
TelephonyParent::RecvHangUpCall(const uint32_t& aClientId,
                                const uint32_t& aCallIndex)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->HangUp(aClientId, aCallIndex);
  return true;
}

bool
TelephonyParent::RecvAnswerCall(const uint32_t& aClientId,
                                const uint32_t& aCallIndex)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->AnswerCall(aClientId, aCallIndex);
  return true;
}

bool
TelephonyParent::RecvRejectCall(const uint32_t& aClientId,
                                const uint32_t& aCallIndex)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->RejectCall(aClientId, aCallIndex);
  return true;
}

bool
TelephonyParent::RecvHoldCall(const uint32_t& aClientId,
                              const uint32_t& aCallIndex)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->HoldCall(aClientId, aCallIndex);
  return true;
}

bool
TelephonyParent::RecvResumeCall(const uint32_t& aClientId,
                                const uint32_t& aCallIndex)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->ResumeCall(aClientId, aCallIndex);
  return true;
}

bool
TelephonyParent::RecvConferenceCall(const uint32_t& aClientId)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->ConferenceCall(aClientId);
  return true;
}

bool
TelephonyParent::RecvSeparateCall(const uint32_t& aClientId,
                                  const uint32_t& aCallIndex)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->SeparateCall(aClientId, aCallIndex);
  return true;
}

bool
TelephonyParent::RecvHoldConference(const uint32_t& aClientId)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->HoldConference(aClientId);
  return true;
}

bool
TelephonyParent::RecvResumeConference(const uint32_t& aClientId)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->ResumeConference(aClientId);
  return true;
}

bool
TelephonyParent::RecvStartTone(const uint32_t& aClientId, const nsString& aTone)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->StartTone(aClientId, aTone);
  return true;
}

bool
TelephonyParent::RecvStopTone(const uint32_t& aClientId)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->StopTone(aClientId);
  return true;
}

bool
TelephonyParent::RecvGetMicrophoneMuted(bool* aMuted)
{
  *aMuted = false;

  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->GetMicrophoneMuted(aMuted);
  return true;
}

bool
TelephonyParent::RecvSetMicrophoneMuted(const bool& aMuted)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->SetMicrophoneMuted(aMuted);
  return true;
}

bool
TelephonyParent::RecvGetSpeakerEnabled(bool* aEnabled)
{
  *aEnabled = false;

  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->GetSpeakerEnabled(aEnabled);
  return true;
}

bool
TelephonyParent::RecvSetSpeakerEnabled(const bool& aEnabled)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  provider->SetSpeakerEnabled(aEnabled);
  return true;
}



NS_IMETHODIMP
TelephonyParent::CallStateChanged(uint32_t aClientId,
                                  uint32_t aCallIndex,
                                  uint16_t aCallState,
                                  const nsAString& aNumber,
                                  bool aIsActive,
                                  bool aIsOutgoing,
                                  bool aIsEmergency,
                                  bool aIsConference,
                                  bool aIsSwitchable,
                                  bool aIsMergeable)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  IPCCallStateData data(aCallIndex, aCallState, nsString(aNumber),
                        aIsActive, aIsOutgoing, aIsEmergency, aIsConference,
                        aIsSwitchable, aIsMergeable);
  return SendNotifyCallStateChanged(aClientId, data) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::ConferenceCallStateChanged(uint16_t aCallState)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  return SendNotifyConferenceCallStateChanged(aCallState) ? NS_OK
                                                          : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::EnumerateCallStateComplete()
{
  MOZ_CRASH("Not a EnumerateCalls request!");
}

NS_IMETHODIMP
TelephonyParent::EnumerateCallState(uint32_t aClientId,
                                    uint32_t aCallIndex,
                                    uint16_t aCallState,
                                    const nsAString& aNumber,
                                    bool aIsActive,
                                    bool aIsOutgoing,
                                    bool aIsEmergency,
                                    bool aIsConference,
                                    bool aIsSwitchable,
                                    bool aIsMergeable)
{
  MOZ_CRASH("Not a EnumerateCalls request!");
}

NS_IMETHODIMP
TelephonyParent::NotifyCdmaCallWaiting(uint32_t aClientId,
                                       const nsAString& aNumber)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  return SendNotifyCdmaCallWaiting(aClientId, nsString(aNumber))
      ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::NotifyConferenceError(const nsAString& aName,
                                       const nsAString& aMessage)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  return SendNotifyConferenceError(nsString(aName), nsString(aMessage)) ? NS_OK
                                                                        : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::NotifyError(uint32_t aClientId,
                             int32_t aCallIndex,
                             const nsAString& aError)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  return SendNotifyCallError(aClientId, aCallIndex, nsString(aError))
      ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::SupplementaryServiceNotification(uint32_t aClientId,
                                                  int32_t aCallIndex,
                                                  uint16_t aNotification)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  return SendNotifySupplementaryService(aClientId, aCallIndex, aNotification)
      ? NS_OK : NS_ERROR_FAILURE;
}





NS_IMPL_ISUPPORTS2(TelephonyRequestParent,
                   nsITelephonyListener,
                   nsITelephonyCallback)

TelephonyRequestParent::TelephonyRequestParent()
  : mActorDestroyed(false)
{
}

void
TelephonyRequestParent::ActorDestroy(ActorDestroyReason why)
{
  
  
  
  mActorDestroyed = true;
}

bool
TelephonyRequestParent::DoRequest(const EnumerateCallsRequest& aRequest)
{
  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  if (provider) {
    rv = provider->EnumerateCalls(this);
  }

  if (NS_FAILED(rv)) {
    return NS_SUCCEEDED(EnumerateCallStateComplete());
  }

  return true;
}

bool
TelephonyRequestParent::DoRequest(const DialRequest& aRequest)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  if (provider) {
    provider->Dial(aRequest.clientId(), aRequest.number(),
                   aRequest.isEmergency(), this);
  } else {
    return NS_SUCCEEDED(NotifyDialError(NS_LITERAL_STRING("InvalidStateError")));
  }

  return true;
}



NS_IMETHODIMP
TelephonyRequestParent::CallStateChanged(uint32_t aClientId,
                                         uint32_t aCallIndex,
                                         uint16_t aCallState,
                                         const nsAString& aNumber,
                                         bool aIsActive,
                                         bool aIsOutgoing,
                                         bool aIsEmergency,
                                         bool aIsConference,
                                         bool aIsSwitchable,
                                         bool aIsMergeable)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::ConferenceCallStateChanged(uint16_t aCallState)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::EnumerateCallStateComplete()
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  return Send__delete__(this, EnumerateCallsResponse()) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyRequestParent::EnumerateCallState(uint32_t aClientId,
                                           uint32_t aCallIndex,
                                           uint16_t aCallState,
                                           const nsAString& aNumber,
                                           bool aIsActive,
                                           bool aIsOutgoing,
                                           bool aIsEmergency,
                                           bool aIsConference,
                                           bool aIsSwitchable,
                                           bool aIsMergeable)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  IPCCallStateData data(aCallIndex, aCallState, nsString(aNumber),
                        aIsActive, aIsOutgoing, aIsEmergency, aIsConference,
                        aIsSwitchable, aIsMergeable);
  return SendNotifyEnumerateCallState(aClientId, data) ? NS_OK
                                                       : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyCdmaCallWaiting(uint32_t aClientId,
                                              const nsAString& aNumber)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyConferenceError(const nsAString& aName,
                                              const nsAString& aMessage)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyError(uint32_t aClientId,
                                    int32_t aCallIndex,
                                    const nsAString& aError)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::SupplementaryServiceNotification(uint32_t aClientId,
                                                         int32_t aCallIndex,
                                                         uint16_t aNotification)
{
  MOZ_CRASH("Not a TelephonyParent!");
}



NS_IMETHODIMP
TelephonyRequestParent::NotifyDialError(const nsAString& aError)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  return (SendNotifyDialError(nsString(aError)) &&
          Send__delete__(this, DialResponse())) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyDialSuccess()
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  return (SendNotifyDialSuccess() &&
          Send__delete__(this, DialResponse())) ? NS_OK : NS_ERROR_FAILURE;
}

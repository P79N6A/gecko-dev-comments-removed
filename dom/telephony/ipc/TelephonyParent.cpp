




#include "mozilla/dom/telephony/TelephonyParent.h"
#include "nsServiceManagerUtils.h"

USING_TELEPHONY_NAMESPACE





NS_IMPL_ISUPPORTS(TelephonyParent, nsITelephonyListener)

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

  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  mRegistered = NS_SUCCEEDED(service->RegisterListener(this));
  return true;
}

bool
TelephonyParent::RecvUnregisterListener()
{
  NS_ENSURE_TRUE(mRegistered, true);

  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  mRegistered = !NS_SUCCEEDED(service->UnregisterListener(this));
  return true;
}

bool
TelephonyParent::RecvHangUpCall(const uint32_t& aClientId,
                                const uint32_t& aCallIndex)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->HangUp(aClientId, aCallIndex);
  return true;
}

bool
TelephonyParent::RecvAnswerCall(const uint32_t& aClientId,
                                const uint32_t& aCallIndex)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->AnswerCall(aClientId, aCallIndex);
  return true;
}

bool
TelephonyParent::RecvRejectCall(const uint32_t& aClientId,
                                const uint32_t& aCallIndex)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->RejectCall(aClientId, aCallIndex);
  return true;
}

bool
TelephonyParent::RecvHoldCall(const uint32_t& aClientId,
                              const uint32_t& aCallIndex)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->HoldCall(aClientId, aCallIndex);
  return true;
}

bool
TelephonyParent::RecvResumeCall(const uint32_t& aClientId,
                                const uint32_t& aCallIndex)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->ResumeCall(aClientId, aCallIndex);
  return true;
}

bool
TelephonyParent::RecvConferenceCall(const uint32_t& aClientId)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->ConferenceCall(aClientId);
  return true;
}

bool
TelephonyParent::RecvSeparateCall(const uint32_t& aClientId,
                                  const uint32_t& aCallIndex)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->SeparateCall(aClientId, aCallIndex);
  return true;
}

bool
TelephonyParent::RecvHoldConference(const uint32_t& aClientId)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->HoldConference(aClientId);
  return true;
}

bool
TelephonyParent::RecvResumeConference(const uint32_t& aClientId)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->ResumeConference(aClientId);
  return true;
}

bool
TelephonyParent::RecvStartTone(const uint32_t& aClientId, const nsString& aTone)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->StartTone(aClientId, aTone);
  return true;
}

bool
TelephonyParent::RecvStopTone(const uint32_t& aClientId)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->StopTone(aClientId);
  return true;
}

bool
TelephonyParent::RecvGetMicrophoneMuted(bool* aMuted)
{
  *aMuted = false;

  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->GetMicrophoneMuted(aMuted);
  return true;
}

bool
TelephonyParent::RecvSetMicrophoneMuted(const bool& aMuted)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->SetMicrophoneMuted(aMuted);
  return true;
}

bool
TelephonyParent::RecvGetSpeakerEnabled(bool* aEnabled)
{
  *aEnabled = false;

  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->GetSpeakerEnabled(aEnabled);
  return true;
}

bool
TelephonyParent::RecvSetSpeakerEnabled(const bool& aEnabled)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->SetSpeakerEnabled(aEnabled);
  return true;
}



NS_IMETHODIMP
TelephonyParent::CallStateChanged(uint32_t aClientId,
                                  uint32_t aCallIndex,
                                  uint16_t aCallState,
                                  const nsAString& aNumber,
                                  uint16_t aNumberPresentation,
                                  const nsAString& aName,
                                  uint16_t aNamePresentation,
                                  bool aIsOutgoing,
                                  bool aIsEmergency,
                                  bool aIsConference,
                                  bool aIsSwitchable,
                                  bool aIsMergeable)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  IPCCallStateData data(aCallIndex, aCallState, nsString(aNumber),
                        aNumberPresentation, nsString(aName), aNamePresentation,
                        aIsOutgoing, aIsEmergency, aIsConference,
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
                                    uint16_t aNumberPresentation,
                                    const nsAString& aName,
                                    uint16_t aNamePresentation,
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
                                       const nsAString& aNumber,
                                       uint16_t aNumberPresentation,
                                       const nsAString& aName,
                                       uint16_t aNamePresentation)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  IPCCdmaWaitingCallData data(nsString(aNumber), aNumberPresentation,
                              nsString(aName), aNamePresentation);
  return SendNotifyCdmaCallWaiting(aClientId, data) ? NS_OK : NS_ERROR_FAILURE;
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





NS_IMPL_ISUPPORTS(TelephonyRequestParent,
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

  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  if (service) {
    rv = service->EnumerateCalls(this);
  }

  if (NS_FAILED(rv)) {
    return NS_SUCCEEDED(EnumerateCallStateComplete());
  }

  return true;
}

bool
TelephonyRequestParent::DoRequest(const DialRequest& aRequest)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  if (service) {
    service->Dial(aRequest.clientId(), aRequest.number(),
                   aRequest.isEmergency(), this);
  } else {
    return NS_SUCCEEDED(NotifyDialError(NS_LITERAL_STRING("InvalidStateError")));
  }

  return true;
}

nsresult
TelephonyRequestParent::SendResponse(const IPCTelephonyResponse& aResponse)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  return Send__delete__(this, aResponse) ? NS_OK : NS_ERROR_FAILURE;
}



NS_IMETHODIMP
TelephonyRequestParent::CallStateChanged(uint32_t aClientId,
                                         uint32_t aCallIndex,
                                         uint16_t aCallState,
                                         const nsAString& aNumber,
                                         uint16_t aNumberPresentation,
                                         const nsAString& aName,
                                         uint16_t aNamePresentation,
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
                                           uint16_t aNumberPresentation,
                                           const nsAString& aName,
                                           uint16_t aNamePresentation,
                                           bool aIsOutgoing,
                                           bool aIsEmergency,
                                           bool aIsConference,
                                           bool aIsSwitchable,
                                           bool aIsMergeable)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  IPCCallStateData data(aCallIndex, aCallState, nsString(aNumber),
                        aNumberPresentation, nsString(aName), aNamePresentation,
                        aIsOutgoing, aIsEmergency, aIsConference,
                        aIsSwitchable, aIsMergeable);
  return SendNotifyEnumerateCallState(aClientId, data) ? NS_OK
                                                       : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyCdmaCallWaiting(uint32_t aClientId,
                                              const nsAString& aNumber,
                                              uint16_t aNumberPresentation,
                                              const nsAString& aName,
                                              uint16_t aNamePresentation)
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
  return SendResponse(DialResponseError(nsAutoString(aError)));
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyDialCallSuccess(uint32_t aCallIndex,
                                              const nsAString& aNumber)
{
  return SendResponse(DialResponseCallSuccess(aCallIndex, nsAutoString(aNumber)));
}

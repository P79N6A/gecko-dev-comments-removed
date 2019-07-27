




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
  nsCOMPtr<nsITelephonyService> service = do_GetService(TELEPHONY_SERVICE_CONTRACTID);

  if (!service) {
    return NS_SUCCEEDED(actor->NotifyError(NS_LITERAL_STRING("InvalidStateError")));
  }

  switch (aRequest.type()) {
    case IPCTelephonyRequest::TEnumerateCallsRequest: {
      nsresult rv = service->EnumerateCalls(actor);
      if (NS_FAILED(rv)) {
        return NS_SUCCEEDED(EnumerateCallStateComplete());
      } else {
        return true;
      }
    }

    case IPCTelephonyRequest::TDialRequest: {
      const DialRequest& request = aRequest.get_DialRequest();
      service->Dial(request.clientId(), request.number(),
                    request.isEmergency(), actor);
      return true;
    }

    case IPCTelephonyRequest::TUSSDRequest: {
      const USSDRequest& request = aRequest.get_USSDRequest();
      service->SendUSSD(request.clientId(), request.ussd(), actor);
      return true;
    }

    case IPCTelephonyRequest::THangUpConferenceRequest: {
      const HangUpConferenceRequest& request = aRequest.get_HangUpConferenceRequest();
      service->HangUpConference(request.clientId(), actor);
      return true;
    }

    case IPCTelephonyRequest::TAnswerCallRequest: {
      const AnswerCallRequest& request = aRequest.get_AnswerCallRequest();
      service->AnswerCall(request.clientId(), request.callIndex(), actor);
      return true;
    }

    case IPCTelephonyRequest::THangUpCallRequest: {
      const HangUpCallRequest& request = aRequest.get_HangUpCallRequest();
      service->HangUpCall(request.clientId(), request.callIndex(), actor);
      return true;
    }

    case IPCTelephonyRequest::TRejectCallRequest: {
      const RejectCallRequest& request = aRequest.get_RejectCallRequest();
      service->RejectCall(request.clientId(), request.callIndex(), actor);
      return true;
    }

    case IPCTelephonyRequest::THoldCallRequest: {
      const HoldCallRequest& request = aRequest.get_HoldCallRequest();
      service->HoldCall(request.clientId(), request.callIndex(), actor);
      return true;
    }

    case IPCTelephonyRequest::TResumeCallRequest: {
      const ResumeCallRequest& request = aRequest.get_ResumeCallRequest();
      service->ResumeCall(request.clientId(), request.callIndex(), actor);
      return true;
    }

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
                  nsITelephonyCallback,
                  nsITelephonyDialCallback)

TelephonyRequestParent::TelephonyRequestParent()
  : mActorDestroyed(false)
{
}

void
TelephonyRequestParent::ActorDestroy(ActorDestroyReason why)
{
  
  
  
  mActorDestroyed = true;
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
TelephonyRequestParent::NotifyDialMMI(const nsAString& aServiceCode)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  return SendNotifyDialMMI(nsAutoString(aServiceCode)) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyRequestParent::NotifySuccess()
{
  return SendResponse(SuccessResponse());
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyError(const nsAString& aError)
{
  return SendResponse(ErrorResponse(nsAutoString(aError)));
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyDialCallSuccess(uint32_t aClientId,
                                              uint32_t aCallIndex,
                                              const nsAString& aNumber)
{
  return SendResponse(DialResponseCallSuccess(aClientId, aCallIndex,
                                              nsAutoString(aNumber)));
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyDialMMISuccess(const nsAString& aStatusMessage)
{
  return SendResponse(DialResponseMMISuccess(nsAutoString(aStatusMessage),
                                             AdditionalInformation(mozilla::void_t())));
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyDialMMISuccessWithInteger(const nsAString& aStatusMessage,
                                                        uint16_t aAdditionalInformation)
{
  return SendResponse(DialResponseMMISuccess(nsAutoString(aStatusMessage),
                                             AdditionalInformation(aAdditionalInformation)));
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyDialMMISuccessWithStrings(const nsAString& aStatusMessage,
                                                        uint32_t aCount,
                                                        const char16_t** aAdditionalInformation)
{
  nsTArray<nsString> additionalInformation;
  for (uint32_t i = 0; i < aCount; i++) {
    additionalInformation.AppendElement(nsDependentString(aAdditionalInformation[i]));
  }

  return SendResponse(DialResponseMMISuccess(nsAutoString(aStatusMessage),
                                             AdditionalInformation(additionalInformation)));
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyDialMMISuccessWithCallForwardingOptions(const nsAString& aStatusMessage,
                                                                      uint32_t aCount,
                                                                      nsIMobileCallForwardingOptions** aAdditionalInformation)
{
  nsTArray<nsIMobileCallForwardingOptions*> additionalInformation;
  for (uint32_t i = 0; i < aCount; i++) {
    additionalInformation.AppendElement(aAdditionalInformation[i]);
  }

  return SendResponse(DialResponseMMISuccess(nsAutoString(aStatusMessage),
                                             AdditionalInformation(additionalInformation)));
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyDialMMIError(const nsAString& aError)
{
  return SendResponse(DialResponseMMIError(nsAutoString(aError),
                                           AdditionalInformation(mozilla::void_t())));
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyDialMMIErrorWithInfo(const nsAString& aError,
                                                   uint16_t aInfo)
{
  return SendResponse(DialResponseMMIError(nsAutoString(aError),
                                           AdditionalInformation(aInfo)));
}

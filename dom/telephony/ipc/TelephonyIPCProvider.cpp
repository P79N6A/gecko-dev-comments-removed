




#include "TelephonyIPCProvider.h"

#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/telephony/TelephonyChild.h"
#include "mozilla/Preferences.h"

USING_TELEPHONY_NAMESPACE
using namespace mozilla::dom;

namespace {

const char* kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";
#define kPrefDefaultServiceId "dom.telephony.defaultServiceId"
const char* kObservedPrefs[] = {
  kPrefDefaultServiceId,
  nullptr
};

uint32_t
getDefaultServiceId()
{
  int32_t id = mozilla::Preferences::GetInt(kPrefDefaultServiceId, 0);
  int32_t numRil = mozilla::Preferences::GetInt(kPrefRilNumRadioInterfaces, 1);

  if (id >= numRil || id < 0) {
    id = 0;
  }

  return id;
}

} 

NS_IMPL_ISUPPORTS(TelephonyIPCProvider,
                  nsITelephonyProvider,
                  nsITelephonyListener,
                  nsIObserver)

TelephonyIPCProvider::TelephonyIPCProvider()
{
  
  mPTelephonyChild = new TelephonyChild(this);
  ContentChild::GetSingleton()->SendPTelephonyConstructor(mPTelephonyChild);

  Preferences::AddStrongObservers(this, kObservedPrefs);
  mDefaultServiceId = getDefaultServiceId();
}

TelephonyIPCProvider::~TelephonyIPCProvider()
{
  if (mPTelephonyChild) {
    mPTelephonyChild->Send__delete__(mPTelephonyChild);
    mPTelephonyChild = nullptr;
  }
}

void
TelephonyIPCProvider::NoteActorDestroyed()
{
  MOZ_ASSERT(mPTelephonyChild);

  mPTelephonyChild = nullptr;
}





NS_IMETHODIMP
TelephonyIPCProvider::Observe(nsISupports* aSubject,
                              const char* aTopic,
                              const char16_t* aData)
{
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsDependentString data(aData);
    if (data.EqualsLiteral(kPrefDefaultServiceId)) {
      mDefaultServiceId = getDefaultServiceId();
    }
    return NS_OK;
  }

  MOZ_ASSERT(false, "TelephonyIPCProvider got unexpected topic!");
  return NS_ERROR_UNEXPECTED;
}





NS_IMETHODIMP
TelephonyIPCProvider::GetDefaultServiceId(uint32_t* aServiceId)
{
  *aServiceId = mDefaultServiceId;
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::RegisterListener(nsITelephonyListener *aListener)
{
  MOZ_ASSERT(!mListeners.Contains(aListener));

  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  
  mListeners.AppendElement(aListener);

  if (mListeners.Length() == 1) {
    mPTelephonyChild->SendRegisterListener();
  }
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::UnregisterListener(nsITelephonyListener *aListener)
{
  MOZ_ASSERT(mListeners.Contains(aListener));

  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  
  mListeners.RemoveElement(aListener);

  if (!mListeners.Length()) {
    mPTelephonyChild->SendUnregisterListener();
  }
  return NS_OK;
}

nsresult
TelephonyIPCProvider::SendRequest(nsITelephonyListener *aListener,
                                  nsITelephonyCallback *aCallback,
                                  const IPCTelephonyRequest& aRequest)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  
  
  TelephonyRequestChild* actor = new TelephonyRequestChild(aListener, aCallback);
  mPTelephonyChild->SendPTelephonyRequestConstructor(actor, aRequest);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::EnumerateCalls(nsITelephonyListener *aListener)
{
  return SendRequest(aListener, nullptr, EnumerateCallsRequest());
}

NS_IMETHODIMP
TelephonyIPCProvider::Dial(uint32_t aClientId, const nsAString& aNumber,
                           bool aIsEmergency, nsITelephonyCallback *aCallback)
{
  return SendRequest(nullptr, aCallback,
                     DialRequest(aClientId, nsString(aNumber), aIsEmergency));
}

NS_IMETHODIMP
TelephonyIPCProvider::HangUp(uint32_t aClientId, uint32_t aCallIndex)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendHangUpCall(aClientId, aCallIndex);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::AnswerCall(uint32_t aClientId, uint32_t aCallIndex)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendAnswerCall(aClientId, aCallIndex);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::RejectCall(uint32_t aClientId, uint32_t aCallIndex)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendRejectCall(aClientId, aCallIndex);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::HoldCall(uint32_t aClientId, uint32_t aCallIndex)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendHoldCall(aClientId, aCallIndex);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::ResumeCall(uint32_t aClientId, uint32_t aCallIndex)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendResumeCall(aClientId, aCallIndex);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::ConferenceCall(uint32_t aClientId)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendConferenceCall(aClientId);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::SeparateCall(uint32_t aClientId, uint32_t aCallIndex)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendSeparateCall(aClientId, aCallIndex);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::HoldConference(uint32_t aClientId)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendHoldConference(aClientId);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::ResumeConference(uint32_t aClientId)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendResumeConference(aClientId);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::StartTone(uint32_t aClientId, const nsAString& aDtmfChar)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendStartTone(aClientId, nsString(aDtmfChar));
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::StopTone(uint32_t aClientId)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendStopTone(aClientId);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::GetMicrophoneMuted(bool* aMuted)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendGetMicrophoneMuted(aMuted);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::SetMicrophoneMuted(bool aMuted)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendSetMicrophoneMuted(aMuted);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::GetSpeakerEnabled(bool* aEnabled)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendGetSpeakerEnabled(aEnabled);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::SetSpeakerEnabled(bool aEnabled)
{
  if (!mPTelephonyChild) {
    NS_WARNING("TelephonyProvider used after shutdown has begun!");
    return NS_ERROR_FAILURE;
  }

  mPTelephonyChild->SendSetSpeakerEnabled(aEnabled);
  return NS_OK;
}



NS_IMETHODIMP
TelephonyIPCProvider::CallStateChanged(uint32_t aClientId,
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
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->CallStateChanged(aClientId, aCallIndex, aCallState, aNumber,
                                    aIsActive, aIsOutgoing, aIsEmergency,
                                    aIsConference, aIsSwitchable, aIsMergeable);
  }
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::ConferenceCallStateChanged(uint16_t aCallState)
{
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->ConferenceCallStateChanged(aCallState);
  }
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::EnumerateCallStateComplete()
{
  MOZ_CRASH("Not a EnumerateCalls request!");
}

NS_IMETHODIMP
TelephonyIPCProvider::EnumerateCallState(uint32_t aClientId,
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
TelephonyIPCProvider::NotifyCdmaCallWaiting(uint32_t aClientId,
                                            const nsAString& aNumber)
{
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->NotifyCdmaCallWaiting(aClientId, aNumber);
  }
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::NotifyConferenceError(const nsAString& aName,
                                            const nsAString& aMessage)
{
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->NotifyConferenceError(aName, aMessage);
  }
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::NotifyError(uint32_t aClientId, int32_t aCallIndex,
                                  const nsAString& aError)
{
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->NotifyError(aClientId, aCallIndex, aError);
  }
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::SupplementaryServiceNotification(uint32_t aClientId,
                                                       int32_t aCallIndex,
                                                       uint16_t aNotification)
{
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->SupplementaryServiceNotification(aClientId, aCallIndex,
                                                    aNotification);
  }
  return NS_OK;
}

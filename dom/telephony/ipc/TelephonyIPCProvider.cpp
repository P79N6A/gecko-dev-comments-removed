




#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/telephony/TelephonyChild.h"
#include "ipc/TelephonyIPCProvider.h"

USING_TELEPHONY_NAMESPACE
using namespace mozilla::dom;

NS_IMPL_ISUPPORTS2(TelephonyIPCProvider,
                   nsITelephonyProvider,
                   nsITelephonyListener)

TelephonyIPCProvider::TelephonyIPCProvider()
{
  
  mPTelephonyChild = new TelephonyChild(this);
  ContentChild::GetSingleton()->SendPTelephonyConstructor(mPTelephonyChild);
}

TelephonyIPCProvider::~TelephonyIPCProvider()
{
  mPTelephonyChild->Send__delete__(mPTelephonyChild);
  mPTelephonyChild = nullptr;
}





NS_IMETHODIMP
TelephonyIPCProvider::RegisterListener(nsITelephonyListener *aListener)
{
  MOZ_ASSERT(!mListeners.Contains(aListener));

  
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

  
  mListeners.RemoveElement(aListener);

  if (!mListeners.Length()) {
    mPTelephonyChild->SendUnregisterListener();
  }
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::EnumerateCalls(nsITelephonyListener *aListener)
{
  
  
  TelephonyRequestChild* actor = new TelephonyRequestChild(aListener);
  mPTelephonyChild->SendPTelephonyRequestConstructor(actor);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::Dial(const nsAString& aNumber,
                          bool aIsEmergency)
{
  mPTelephonyChild->SendDialCall(nsString(aNumber), aIsEmergency);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::HangUp(uint32_t aCallIndex)
{
  mPTelephonyChild->SendHangUpCall(aCallIndex);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::AnswerCall(uint32_t aCallIndex)
{
  mPTelephonyChild->SendAnswerCall(aCallIndex);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::RejectCall(uint32_t aCallIndex)
{
  mPTelephonyChild->SendRejectCall(aCallIndex);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::HoldCall(uint32_t aCallIndex)
{
  mPTelephonyChild->SendHoldCall(aCallIndex);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::ResumeCall(uint32_t aCallIndex)
{
  mPTelephonyChild->SendResumeCall(aCallIndex);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::ConferenceCall()
{
  mPTelephonyChild->SendConferenceCall();
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::SeparateCall(uint32_t aCallIndex)
{
  mPTelephonyChild->SendSeparateCall(aCallIndex);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::HoldConference()
{
  mPTelephonyChild->SendHoldConference();
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::ResumeConference()
{
  mPTelephonyChild->SendResumeConference();
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::StartTone(const nsAString& aDtmfChar)
{
  mPTelephonyChild->SendStartTone(nsString(aDtmfChar));
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::StopTone()
{
  mPTelephonyChild->SendStopTone();
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::GetMicrophoneMuted(bool* aMuted)
{
  mPTelephonyChild->SendGetMicrophoneMuted(aMuted);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::SetMicrophoneMuted(bool aMuted)
{
  mPTelephonyChild->SendSetMicrophoneMuted(aMuted);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::GetSpeakerEnabled(bool* aEnabled)
{
  mPTelephonyChild->SendGetSpeakerEnabled(aEnabled);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::SetSpeakerEnabled(bool aEnabled)
{
  mPTelephonyChild->SendSetSpeakerEnabled(aEnabled);
  return NS_OK;
}



NS_IMETHODIMP
TelephonyIPCProvider::CallStateChanged(uint32_t aCallIndex,
                                      uint16_t aCallState,
                                      const nsAString& aNumber,
                                      bool aIsActive,
                                      bool aIsOutgoing,
                                      bool aIsEmergency,
                                      bool aIsConference)
{
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->CallStateChanged(aCallIndex, aCallState, aNumber,
                                    aIsActive, aIsOutgoing, aIsEmergency,
                                    aIsConference);
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
TelephonyIPCProvider::EnumerateCallState(uint32_t aCallIndex,
                                        uint16_t aCallState,
                                        const nsAString& aNumber,
                                        bool aIsActive,
                                        bool aIsOutgoing,
                                        bool aIsEmergency,
                                        bool aIsConference)
{
  MOZ_CRASH("Not a EnumerateCalls request!");
}

NS_IMETHODIMP
TelephonyIPCProvider::NotifyCdmaCallWaiting(const nsAString& aNumber)
{
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->NotifyCdmaCallWaiting(aNumber);
  }
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::NotifyError(int32_t aCallIndex,
                                 const nsAString& aError)
{
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->NotifyError(aCallIndex, aError);
  }
  return NS_OK;
}

NS_IMETHODIMP
TelephonyIPCProvider::SupplementaryServiceNotification(int32_t aCallIndex,
                                                      uint16_t aNotification)
{
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->SupplementaryServiceNotification(aCallIndex, aNotification);
  }
  return NS_OK;
}

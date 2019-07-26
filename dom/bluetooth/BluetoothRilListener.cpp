





#include "BluetoothRilListener.h"

#include "BluetoothHfpManager.h"
#include "nsIDOMMobileConnection.h"
#include "nsIRadioInterfaceLayer.h"
#include "nsRadioInterfaceLayer.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"

USING_BLUETOOTH_NAMESPACE




NS_IMPL_ISUPPORTS(IccListener, nsIIccListener)

NS_IMETHODIMP
IccListener::NotifyIccInfoChanged()
{
  
  NS_ENSURE_TRUE(mOwner, NS_ERROR_FAILURE);

  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  NS_ENSURE_TRUE(hfp, NS_ERROR_FAILURE);

  hfp->HandleIccInfoChanged(mOwner->mClientId);

  return NS_OK;
}

NS_IMETHODIMP
IccListener::NotifyStkCommand(const nsAString & aMessage)
{
  return NS_OK;
}

NS_IMETHODIMP
IccListener::NotifyStkSessionEnd()
{
  return NS_OK;
}

NS_IMETHODIMP
IccListener::NotifyCardStateChanged()
{
  return NS_OK;
}

bool
IccListener::Listen(bool aStart)
{
  NS_ENSURE_TRUE(mOwner, false);

  nsCOMPtr<nsIIccProvider> provider =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(provider, false);

  nsresult rv;
  if (aStart) {
    rv = provider->RegisterIccMsg(mOwner->mClientId, this);
  } else {
    rv = provider->UnregisterIccMsg(mOwner->mClientId, this);
  }

  return NS_SUCCEEDED(rv);
}

void
IccListener::SetOwner(BluetoothRilListener *aOwner)
{
  mOwner = aOwner;
}




NS_IMPL_ISUPPORTS(MobileConnectionListener, nsIMobileConnectionListener)

NS_IMETHODIMP
MobileConnectionListener::NotifyVoiceChanged()
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  NS_ENSURE_TRUE(hfp, NS_OK);

  hfp->HandleVoiceConnectionChanged(mClientId);

  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionListener::NotifyDataChanged()
{
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionListener::NotifyUssdReceived(const nsAString & message,
                                             bool sessionEnded)
{
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionListener::NotifyDataError(const nsAString & message)
{
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionListener::NotifyCFStateChange(bool success,
                                              uint16_t action,
                                              uint16_t reason,
                                              const nsAString& number,
                                              uint16_t timeSeconds,
                                              uint16_t serviceClass)
{
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionListener::NotifyEmergencyCbModeChanged(bool active,
                                                       uint32_t timeoutMs)
{
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionListener::NotifyOtaStatusChanged(const nsAString & status)
{
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionListener::NotifyIccChanged()
{
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionListener::NotifyRadioStateChanged()
{
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionListener::NotifyClirModeChanged(uint32_t aMode)
{
  return NS_OK;
}

bool
MobileConnectionListener::Listen(bool aStart)
{
  nsCOMPtr<nsIMobileConnectionProvider> provider =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(provider, false);

  nsresult rv;
  if (aStart) {
    rv = provider->RegisterMobileConnectionMsg(mClientId, this);
  } else {
    rv = provider->UnregisterMobileConnectionMsg(mClientId, this);
  }

  return NS_SUCCEEDED(rv);
}




NS_IMPL_ISUPPORTS(TelephonyListener, nsITelephonyListener)

NS_IMETHODIMP
TelephonyListener::CallStateChanged(uint32_t aServiceId,
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
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  NS_ENSURE_TRUE(hfp, NS_ERROR_FAILURE);

  hfp->HandleCallStateChanged(aCallIndex, aCallState, EmptyString(), aNumber,
                              aIsOutgoing, aIsConference, true);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::EnumerateCallState(uint32_t aServiceId,
                                      uint32_t aCallIndex,
                                      uint16_t aCallState,
                                      const nsAString_internal& aNumber,
                                      bool aIsActive,
                                      bool aIsOutgoing,
                                      bool aIsEmergency,
                                      bool aIsConference,
                                      bool aIsSwitchable,
                                      bool aIsMergeable)
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  NS_ENSURE_TRUE(hfp, NS_ERROR_FAILURE);

  hfp->HandleCallStateChanged(aCallIndex, aCallState, EmptyString(), aNumber,
                              aIsOutgoing, aIsConference, false);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::NotifyError(uint32_t aServiceId,
                               int32_t aCallIndex,
                               const nsAString& aError)
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  NS_ENSURE_TRUE(hfp, NS_ERROR_FAILURE);

  if (aCallIndex > 0) {
    
    
    
    
    
    hfp->HandleCallStateChanged(aCallIndex,
                                nsITelephonyProvider::CALL_STATE_DISCONNECTED,
                                aError, EmptyString(), false, false, true);
    BT_WARNING("Reset the call state due to call transition ends abnormally");
  }

  BT_WARNING(NS_ConvertUTF16toUTF8(aError).get());
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::ConferenceCallStateChanged(uint16_t aCallState)
{
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::EnumerateCallStateComplete()
{
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::SupplementaryServiceNotification(uint32_t aServiceId,
                                                    int32_t aCallIndex,
                                                    uint16_t aNotification)
{
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::NotifyConferenceError(const nsAString& aName,
                                         const nsAString& aMessage)
{
  BT_WARNING(NS_ConvertUTF16toUTF8(aName).get());
  BT_WARNING(NS_ConvertUTF16toUTF8(aMessage).get());

  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::NotifyCdmaCallWaiting(uint32_t aServiceId,
                                         const nsAString& aNumber)
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  NS_ENSURE_TRUE(hfp, NS_ERROR_FAILURE);

  hfp->UpdateSecondNumber(aNumber);

  return NS_OK;
}

bool
TelephonyListener::Listen(bool aStart)
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, false);

  nsresult rv;
  if (aStart) {
    rv = provider->RegisterListener(this);
  } else {
    rv = provider->UnregisterListener(this);
  }

  return NS_SUCCEEDED(rv);
}




BluetoothRilListener::BluetoothRilListener()
{
  
  uint32_t numOfClients;
  nsCOMPtr<nsIRadioInterfaceLayer> radioInterfaceLayer =
    do_GetService(NS_RADIOINTERFACELAYER_CONTRACTID);
  NS_ENSURE_TRUE_VOID(radioInterfaceLayer);

  radioInterfaceLayer->GetNumRadioInterfaces(&numOfClients);

  
  for (uint32_t i = 0; i < numOfClients; i++) {
    mMobileConnListeners.AppendElement(new MobileConnectionListener(i));
  }

  mTelephonyListener = new TelephonyListener();
  mIccListener = new IccListener();
  mIccListener->SetOwner(this);

  
  SelectClient();
}

BluetoothRilListener::~BluetoothRilListener()
{
  mIccListener->SetOwner(nullptr);
}

bool
BluetoothRilListener::Listen(bool aStart)
{
  NS_ENSURE_TRUE(ListenMobileConnAndIccInfo(aStart), false);
  NS_ENSURE_TRUE(mTelephonyListener->Listen(aStart), false);

  return true;
}

void
BluetoothRilListener::SelectClient()
{
  
  mClientId = mMobileConnListeners.Length();

  nsCOMPtr<nsIMobileConnectionProvider> connection =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE_VOID(connection);

  for (uint32_t i = 0; i < mMobileConnListeners.Length(); i++) {
    nsCOMPtr<nsIDOMMozMobileConnectionInfo> voiceInfo;
    connection->GetVoiceConnectionInfo(i, getter_AddRefs(voiceInfo));
    if (!voiceInfo) {
      BT_WARNING("%s: Failed to get voice connection info", __FUNCTION__);
      continue;
    }

    nsString regState;
    voiceInfo->GetState(regState);
    if (regState.EqualsLiteral("registered")) {
      
      mClientId = i;
      return;
    }
  }
}

void
BluetoothRilListener::ServiceChanged(uint32_t aClientId, bool aRegistered)
{
  
  ListenMobileConnAndIccInfo(false);

  






  if (aRegistered) {
    mClientId = aClientId;
  } else {
    SelectClient();
  }

  
  ListenMobileConnAndIccInfo(true);

  BT_LOGR("%d client %d. new mClientId %d", aRegistered, aClientId,
          (mClientId < mMobileConnListeners.Length()) ? mClientId : -1);
}

void
BluetoothRilListener::EnumerateCalls()
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE_VOID(provider);

  nsCOMPtr<nsITelephonyListener> listener(
    do_QueryObject(mTelephonyListener));

  provider->EnumerateCalls(listener);
}

bool
BluetoothRilListener::ListenMobileConnAndIccInfo(bool aStart)
{
  








  if (mClientId < mMobileConnListeners.Length()) {
    NS_ENSURE_TRUE(mMobileConnListeners[mClientId]->Listen(aStart), false);
    NS_ENSURE_TRUE(mIccListener->Listen(aStart), false);
  } else {
    for (uint32_t i = 0; i < mMobileConnListeners.Length(); i++) {
      NS_ENSURE_TRUE(mMobileConnListeners[i]->Listen(aStart), false);
    }
  }

  return true;
}

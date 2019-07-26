





#include "BluetoothRilListener.h"

#include "BluetoothHfpManager.h"
#include "nsIIccProvider.h"
#include "nsIMobileConnectionProvider.h"
#include "nsITelephonyProvider.h"
#include "nsRadioInterfaceLayer.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"

USING_BLUETOOTH_NAMESPACE

namespace {




class IccListener : public nsIIccListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIICCLISTENER

  IccListener() { }
};

NS_IMPL_ISUPPORTS1(IccListener, nsIIccListener)

NS_IMETHODIMP
IccListener::NotifyIccInfoChanged()
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  hfp->HandleIccInfoChanged();

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




class MobileConnectionListener : public nsIMobileConnectionListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILECONNECTIONLISTENER

  MobileConnectionListener() { }
};

NS_IMPL_ISUPPORTS1(MobileConnectionListener, nsIMobileConnectionListener)

NS_IMETHODIMP
MobileConnectionListener::NotifyVoiceChanged()
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  hfp->HandleVoiceConnectionChanged();

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




class TelephonyListener : public nsITelephonyListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITELEPHONYLISTENER

  TelephonyListener() { }
};

NS_IMPL_ISUPPORTS1(TelephonyListener, nsITelephonyListener)

NS_IMETHODIMP
TelephonyListener::CallStateChanged(uint32_t aCallIndex,
                                    uint16_t aCallState,
                                    const nsAString& aNumber,
                                    bool aIsActive,
                                    bool aIsOutgoing,
                                    bool aIsEmergency,
                                    bool aIsConference)
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  hfp->HandleCallStateChanged(aCallIndex, aCallState, EmptyString(), aNumber,
                              aIsOutgoing, true);

  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::EnumerateCallState(uint32_t aCallIndex,
                                      uint16_t aCallState,
                                      const nsAString_internal& aNumber,
                                      bool aIsActive,
                                      bool aIsOutgoing,
                                      bool aIsEmergency,
                                      bool aIsConference)
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  hfp->HandleCallStateChanged(aCallIndex, aCallState, EmptyString(), aNumber,
                              aIsOutgoing, false);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::NotifyError(int32_t aCallIndex,
                               const nsAString& aError)
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();

  if (aCallIndex > 0) {
    
    
    
    
    
    hfp->HandleCallStateChanged(aCallIndex,
                                nsITelephonyProvider::CALL_STATE_DISCONNECTED,
                                aError, EmptyString(), false, true);
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
TelephonyListener::SupplementaryServiceNotification(int32_t aCallIndex,
                                                    uint16_t aNotification)
{
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::NotifyCdmaCallWaiting(const nsAString& aNumber)
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  hfp->UpdateSecondNumber(aNumber);

  return NS_OK;
}

} 




BluetoothRilListener::BluetoothRilListener()
{
  mIccListener = new IccListener();
  mMobileConnectionListener = new MobileConnectionListener();
  mTelephonyListener = new TelephonyListener();
}

bool
BluetoothRilListener::StartListening()
{
  NS_ENSURE_TRUE(StartIccListening(), false);
  NS_ENSURE_TRUE(StartMobileConnectionListening(), false);
  NS_ENSURE_TRUE(StartTelephonyListening(), false);

  return true;
}

bool
BluetoothRilListener::StopListening()
{
  NS_ENSURE_TRUE(StopIccListening(), false);
  NS_ENSURE_TRUE(StopMobileConnectionListening(), false);
  NS_ENSURE_TRUE(StopTelephonyListening(), false);

  return true;
}

void
BluetoothRilListener::EnumerateCalls()
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE_VOID(provider);

  provider->EnumerateCalls(mTelephonyListener);
}


bool
BluetoothRilListener::StartIccListening()
{
  nsCOMPtr<nsIIccProvider> provider =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(provider, false);

  nsresult rv = provider->RegisterIccMsg(mIccListener);
  return NS_SUCCEEDED(rv);
}

bool
BluetoothRilListener::StopIccListening()
{
  nsCOMPtr<nsIIccProvider> provider =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(provider, false);

  nsresult rv = provider->UnregisterIccMsg(mIccListener);
  return NS_SUCCEEDED(rv);
}

bool
BluetoothRilListener::StartMobileConnectionListening()
{
  nsCOMPtr<nsIMobileConnectionProvider> provider =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(provider, false);

  nsresult rv = provider->
                  RegisterMobileConnectionMsg(mMobileConnectionListener);
  return NS_SUCCEEDED(rv);
}

bool
BluetoothRilListener::StopMobileConnectionListening()
{
  nsCOMPtr<nsIMobileConnectionProvider> provider =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(provider, false);

  nsresult rv = provider->
                  UnregisterMobileConnectionMsg(mMobileConnectionListener);
  return NS_SUCCEEDED(rv);
}

bool
BluetoothRilListener::StartTelephonyListening()
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, false);

  nsresult rv = provider->RegisterListener(mTelephonyListener);
  return NS_SUCCEEDED(rv);
}

bool
BluetoothRilListener::StopTelephonyListening()
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(TELEPHONY_PROVIDER_CONTRACTID);
  NS_ENSURE_TRUE(provider, false);

  nsresult rv = provider->UnregisterListener(mTelephonyListener);
  return NS_SUCCEEDED(rv);
}
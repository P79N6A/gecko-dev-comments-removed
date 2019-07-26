





#include "BluetoothTelephonyListener.h"

#include "BluetoothHfpManager.h"
#include "nsRadioInterfaceLayer.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"

USING_BLUETOOTH_NAMESPACE

namespace {

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
                                    bool aIsEmergency)
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  hfp->HandleCallStateChanged(aCallIndex, aCallState, EmptyString(), aNumber,
                              aIsOutgoing, true);

  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::EnumerateCallStateComplete()
{
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::EnumerateCallState(uint32_t aCallIndex,
                                      uint16_t aCallState,
                                      const nsAString_internal& aNumber,
                                      bool aIsActive,
                                      bool aIsOutgoing,
                                      bool aIsEmergency,
                                      bool* aResult)
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  hfp->HandleCallStateChanged(aCallIndex, aCallState, EmptyString(), aNumber,
                              aIsOutgoing, false);
  *aResult = true;
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::SupplementaryServiceNotification(int32_t aCallIndex,
                                                    uint16_t aNotification)
{
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
    NS_WARNING("Reset the call state due to call transition ends abnormally");
  }

  NS_WARNING(NS_ConvertUTF16toUTF8(aError).get());
  return NS_OK;
}

} 

BluetoothTelephonyListener::BluetoothTelephonyListener()
{
  mTelephonyListener = new TelephonyListener();
}

bool
BluetoothTelephonyListener::StartListening()
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(provider, false);

  nsresult rv = provider->RegisterTelephonyMsg(mTelephonyListener);
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool
BluetoothTelephonyListener::StopListening()
{
  nsCOMPtr<nsITelephonyProvider> provider =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(provider, false);

  nsresult rv = provider->UnregisterTelephonyMsg(mTelephonyListener);

  return NS_FAILED(rv) ? false : true;
}

nsITelephonyListener*
BluetoothTelephonyListener::GetListener()
{
  return mTelephonyListener;
}

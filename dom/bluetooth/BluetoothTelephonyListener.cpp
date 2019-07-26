





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
                                    bool aIsActive)
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  hfp->HandleCallStateChanged(aCallIndex, aCallState, aNumber, true);

  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::EnumerateCallState(uint32_t aCallIndex,
                                      uint16_t aCallState,
                                      const nsAString_internal& aNumber,
                                      bool aIsActive,
                                      bool* aResult)
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  hfp->HandleCallStateChanged(aCallIndex, aCallState, aNumber, false);
  *aResult = true;
  return NS_OK;
}

NS_IMETHODIMP
TelephonyListener::NotifyError(int32_t aCallIndex,
                               const nsAString& aError)
{
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

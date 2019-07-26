





#include "BluetoothRilListener.h"

#include "BluetoothHfpManager.h"
#include "nsRadioInterfaceLayer.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"

USING_BLUETOOTH_NAMESPACE

class BluetoothRILTelephonyCallback : public nsIRILTelephonyCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRILTELEPHONYCALLBACK

  BluetoothRILTelephonyCallback() { }
};

NS_IMPL_ISUPPORTS1(BluetoothRILTelephonyCallback, nsIRILTelephonyCallback)

NS_IMETHODIMP
BluetoothRILTelephonyCallback::CallStateChanged(uint32_t aCallIndex,
                                                uint16_t aCallState,
                                                const nsAString& aNumber,
                                                bool aIsActive)
{
  BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
  hfp->HandleCallStateChanged(aCallIndex, aCallState, aNumber, true);

  return NS_OK;
}

NS_IMETHODIMP
BluetoothRILTelephonyCallback::EnumerateCallState(uint32_t aCallIndex,
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
BluetoothRILTelephonyCallback::NotifyError(int32_t aCallIndex,
                                           const nsAString& aError)
{
  return NS_OK;
}

BluetoothRilListener::BluetoothRilListener()
{
  mRILTelephonyCallback = new BluetoothRILTelephonyCallback();
}

bool
BluetoothRilListener::StartListening()
{
  nsCOMPtr<nsIRILContentHelper> ril =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(ril, false);

  nsresult rv = ril->RegisterTelephonyCallback(mRILTelephonyCallback);
  NS_ENSURE_SUCCESS(rv, false);
  rv = ril->RegisterTelephonyMsg();
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool
BluetoothRilListener::StopListening()
{
  nsCOMPtr<nsIRILContentHelper> ril =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(ril, false);

  nsresult rv = ril->UnregisterTelephonyCallback(mRILTelephonyCallback);

  return NS_FAILED(rv) ? false : true;
}

nsIRILTelephonyCallback*
BluetoothRilListener::GetCallback()
{
  return mRILTelephonyCallback;
}

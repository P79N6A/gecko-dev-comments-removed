





#include "BluetoothAdapter.h"
#include "nsDOMClassInfo.h"

USING_BLUETOOTH_NAMESPACE

BluetoothAdapter::BluetoothAdapter() : mPower(false)
{
}

NS_INTERFACE_MAP_BEGIN(BluetoothAdapter)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBluetoothAdapter)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BluetoothAdapter)
NS_INTERFACE_MAP_END
  
NS_IMPL_ADDREF(BluetoothAdapter)
NS_IMPL_RELEASE(BluetoothAdapter)

DOMCI_DATA(BluetoothAdapter, BluetoothAdapter)
  
NS_IMETHODIMP
BluetoothAdapter::GetPower(bool* aPower)
{
  *aPower = mPower;
  return NS_OK;
}

NS_IMETHODIMP
BluetoothAdapter::SetPower(bool aPower)
{
  mPower = aPower;
  return NS_OK;
}

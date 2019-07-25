





#include "base/basictypes.h"
#include "BluetoothDeviceEvent.h"
#include "BluetoothTypes.h"
#include "BluetoothDevice.h"
#include "nsIDOMDOMRequest.h"

#include "nsDOMClassInfo.h"

USING_BLUETOOTH_NAMESPACE


already_AddRefed<BluetoothDeviceEvent>
BluetoothDeviceEvent::Create(BluetoothDevice* aDevice)
{
  NS_ASSERTION(aDevice, "Null pointer!");

  nsRefPtr<BluetoothDeviceEvent> event = new BluetoothDeviceEvent();

  event->mDevice = aDevice;

  return event.forget();
}

NS_IMPL_CYCLE_COLLECTION_CLASS(BluetoothDeviceEvent)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(BluetoothDeviceEvent,
                                                  nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDevice)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(BluetoothDeviceEvent,
                                                nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDevice)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BluetoothDeviceEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBluetoothDeviceEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BluetoothDeviceEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(BluetoothDeviceEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(BluetoothDeviceEvent, nsDOMEvent)

DOMCI_DATA(BluetoothDeviceEvent, BluetoothDeviceEvent)

NS_IMETHODIMP
BluetoothDeviceEvent::GetDevice(nsIDOMBluetoothDevice** aDevice)
{
  nsCOMPtr<nsIDOMBluetoothDevice> device = mDevice.get();
  device.forget(aDevice);
  return NS_OK;
}

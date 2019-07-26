





#include "base/basictypes.h"
#include "BluetoothPropertyEvent.h"

#include "nsDOMClassInfo.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"

USING_BLUETOOTH_NAMESPACE


already_AddRefed<BluetoothPropertyEvent>
BluetoothPropertyEvent::Create(const nsAString& aPropertyName)
{
  NS_ASSERTION(!aPropertyName.IsEmpty(), "Empty Property String!");

  nsRefPtr<BluetoothPropertyEvent> event = new BluetoothPropertyEvent();

  event->mPropertyName = aPropertyName;

  return event.forget();
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(BluetoothPropertyEvent,
                                                  nsDOMEvent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(BluetoothPropertyEvent,
                                                nsDOMEvent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BluetoothPropertyEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBluetoothPropertyEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BluetoothPropertyEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(BluetoothPropertyEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(BluetoothPropertyEvent, nsDOMEvent)

DOMCI_DATA(BluetoothPropertyEvent, BluetoothPropertyEvent)

NS_IMETHODIMP
BluetoothPropertyEvent::GetProperty(nsAString& aPropertyName)
{
  aPropertyName = mPropertyName;
  return NS_OK;
}

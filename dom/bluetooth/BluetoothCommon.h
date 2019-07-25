





#ifndef mozilla_dom_bluetooth_bluetoothcommon_h__
#define mozilla_dom_bluetooth_bluetoothcommon_h__

#include "nsString.h"
#include "nsTArray.h"
#include "mozilla/Observer.h"

#define BEGIN_BLUETOOTH_NAMESPACE \
  namespace mozilla { namespace dom { namespace bluetooth {
#define END_BLUETOOTH_NAMESPACE \
  } /* namespace bluetooth */ } /* namespace dom */ } /* namespace mozilla */
#define USING_BLUETOOTH_NAMESPACE \
  using namespace mozilla::dom::bluetooth;

class nsCString;

BEGIN_BLUETOOTH_NAMESPACE










struct BluetoothVariant
{
  uint32_t mUint32;
  nsCString mString;  
};





struct BluetoothNamedVariant
{
  nsCString mName;
  BluetoothVariant mValue;
};





struct BluetoothEvent
{
  nsCString mEventName;
  nsTArray<BluetoothNamedVariant> mValues;
};

typedef mozilla::Observer<BluetoothEvent> BluetoothEventObserver;
typedef mozilla::ObserverList<BluetoothEvent> BluetoothEventObserverList;

END_BLUETOOTH_NAMESPACE

#endif 

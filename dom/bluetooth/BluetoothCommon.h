





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

#define LOCAL_AGENT_PATH  "/B2G/bluetooth/agent"
#define REMOTE_AGENT_PATH "/B2G/bluetooth/remote_device_agent"


#define BLUETOOTH_ADDRESS_LENGTH 17

#define DOM_BLUETOOTH_URL_PREF "dom.mozBluetooth.whitelist"

class nsCString;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSignal;
typedef mozilla::Observer<BluetoothSignal> BluetoothSignalObserver;




enum BluetoothObjectType {
  TYPE_MANAGER = 0,
  TYPE_ADAPTER = 1,
  TYPE_DEVICE = 2 
};

END_BLUETOOTH_NAMESPACE

#endif 

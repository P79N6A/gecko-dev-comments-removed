





#ifndef mozilla_dom_bluetooth_bluetoothfirmware_h__
#define mozilla_dom_bluetooth_bluetoothfirmware_h__

namespace mozilla {
namespace dom {
namespace bluetooth {

bool EnsureBluetoothInit();
int IsBluetoothEnabled();
int EnableBluetooth();
int DisableBluetooth();

}
}
}

#endif

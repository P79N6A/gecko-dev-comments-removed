





#ifndef mozilla_dom_bluetooth_bluetoothuuid_h__
#define mozilla_dom_bluetooth_bluetoothuuid_h__

namespace mozilla {
namespace dom {
namespace bluetooth {

namespace BluetoothServiceUuid {
  static unsigned long long Headset      = 0x0000110800000000;
  static unsigned long long HeadsetAG    = 0x0000111200000000;
  static unsigned long long Handsfree    = 0x0000111E00000000;
  static unsigned long long HandsfreeAG  = 0x0000111F00000000;
  static unsigned long long ObjectPush   = 0x0000110500000000;

  static unsigned long long BaseMSB     = 0x0000000000001000;
  static unsigned long long BaseLSB     = 0x800000805F9B34FB;
}

namespace BluetoothServiceUuidStr {
  static const char* Headset       = "00001108-0000-1000-8000-00805F9B34FB";
  static const char* HeadsetAG     = "00001112-0000-1000-8000-00805F9B34FB";
  static const char* Handsfree     = "0000111E-0000-1000-8000-00805F9B34FB";
  static const char* HandsfreeAG   = "0000111F-0000-1000-8000-00805F9B34FB";
  static const char* ObjectPush    = "00001105-0000-1000-8000-00805F9B34FB";
}

}
}
}

#endif


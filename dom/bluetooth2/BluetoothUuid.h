





#ifndef mozilla_dom_bluetooth_bluetoothuuid_h__
#define mozilla_dom_bluetooth_bluetoothuuid_h__

#include "BluetoothCommon.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothProfileManagerBase;










enum BluetoothServiceClass
{
  A2DP          = 0x110D,
  HANDSFREE     = 0x111E,
  HANDSFREE_AG  = 0x111F,
  HEADSET       = 0x1108,
  HEADSET_AG    = 0x1112,
  HID           = 0x1124,
  OBJECT_PUSH   = 0x1105,
  UNKNOWN       = 0x0000
};

class BluetoothUuidHelper
{
public:
  






  static void
  GetString(BluetoothServiceClass aServiceClassUuid, nsAString& aRetUuidStr);

  





  static BluetoothServiceClass
  GetBluetoothServiceClass(const nsAString& aUuidStr);

  static BluetoothServiceClass
  GetBluetoothServiceClass(uint16_t aServiceUuid);

  static BluetoothProfileManagerBase*
  GetBluetoothProfileManager(uint16_t aServiceUuid);
};






enum BluetoothReservedChannels {
  CHANNEL_DIALUP_NETWORK = 1,
  CHANNEL_HANDSFREE_AG   = 10,
  CHANNEL_HEADSET_AG     = 11,
  CHANNEL_OPUSH          = 12,
  CHANNEL_SIM_ACCESS     = 15,
  CHANNEL_PBAP_PSE       = 19,
  CHANNEL_FTP            = 20,
  CHANNEL_OPUSH_L2CAP    = 5255
};

END_BLUETOOTH_NAMESPACE

#endif

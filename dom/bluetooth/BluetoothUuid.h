





#ifndef mozilla_dom_bluetooth_bluetoothuuid_h__
#define mozilla_dom_bluetooth_bluetoothuuid_h__

#include "BluetoothCommon.h"

BEGIN_BLUETOOTH_NAMESPACE










enum BluetoothServiceClass
{
  HEADSET       = 0x1108,
  HEADSET_AG    = 0x1112,
  HANDSFREE     = 0x111E,
  HANDSFREE_AG  = 0x111F,
  OBJECT_PUSH   = 0x1105
};

class BluetoothUuidHelper
{
public:
  






  static void
  GetString(BluetoothServiceClass aServiceClassUuid, nsAString& aRetUuidStr);
};






enum BluetoothReservedChannels {
  CHANNEL_DIALUP_NETWORK = 1,
  CHANNEL_HANDSFREE_AG = 10,
  CHANNEL_HEADSET_AG = 11,
  CHANNEL_OPUSH = 12,
  CHANNEL_SIM_ACCESS = 15,
  CHANNEL_PBAP_PSE = 19,
  CHANNEL_FTP = 20,
};

END_BLUETOOTH_NAMESPACE

#endif

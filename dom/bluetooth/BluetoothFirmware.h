





#ifndef mozilla_dom_bluetooth_bluetoothfirmware_h__
#define mozilla_dom_bluetooth_bluetoothfirmware_h__

namespace mozilla {
namespace dom {
namespace bluetooth {

static struct BluedroidFunctions {
  bool initialized;
  bool tried_initialization;

  BluedroidFunctions() :
    initialized(false),
    tried_initialization(false)
  {
  }
  
  int (* bt_enable)();
  int (* bt_disable)();
  int (* bt_is_enabled)();
} sBluedroidFunctions;

bool EnsureBluetoothInit();
}
}
}

#endif







#include "BluetoothFirmware.h"

#include "nsDebug.h"
#include "nsError.h"
#include <dlfcn.h>

namespace mozilla {
namespace dom {
namespace bluetooth {

bool EnsureBluetoothInit() {
  if (sBluedroidFunctions.tried_initialization)
  {
    return sBluedroidFunctions.initialized;
  }

  sBluedroidFunctions.initialized = false;
  sBluedroidFunctions.tried_initialization = true;
  
  void* handle = dlopen("libbluedroid.so", RTLD_LAZY);

  if(!handle) {
    NS_ERROR("Failed to open libbluedroid.so, bluetooth cannot run");
    return false;
  }

  sBluedroidFunctions.bt_enable = (int (*)())dlsym(handle, "bt_enable");
  if(sBluedroidFunctions.bt_enable == NULL) {
    NS_ERROR("Failed to attach bt_enable function");
    return false;
  }
  sBluedroidFunctions.bt_disable = (int (*)())dlsym(handle, "bt_disable");
  if(sBluedroidFunctions.bt_disable == NULL) {
    NS_ERROR("Failed to attach bt_disable function");
    return false;
  }
  sBluedroidFunctions.bt_is_enabled = (int (*)())dlsym(handle, "bt_is_enabled");
  if(sBluedroidFunctions.bt_is_enabled == NULL) {
    NS_ERROR("Failed to attach bt_is_enabled function");
    return false;
  }
  sBluedroidFunctions.initialized = true;
  return true;
}

}
}
}

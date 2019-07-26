
















#include "base/basictypes.h"
#include "BluetoothGonkService.h"
#include "BluetoothDBusService.h"

#include "nsDebug.h"
#include "nsError.h"
#include "nsThreadUtils.h"
#include <dlfcn.h>

USING_BLUETOOTH_NAMESPACE

static struct BluedroidFunctions
{
  bool initialized;

  BluedroidFunctions() :
    initialized(false)
  {
  }

  int (* bt_enable)();
  int (* bt_disable)();
  int (* bt_is_enabled)();
} sBluedroidFunctions;

static bool
EnsureBluetoothInit()
{
  if (sBluedroidFunctions.initialized) {
    return true;
  }

  void* handle = dlopen("libbluedroid.so", RTLD_LAZY);

  if (!handle) {
    NS_ERROR("Failed to open libbluedroid.so, bluetooth cannot run");
    return false;
  }

  sBluedroidFunctions.bt_enable = (int (*)())dlsym(handle, "bt_enable");
  if (!sBluedroidFunctions.bt_enable) {
    NS_ERROR("Failed to attach bt_enable function");
    return false;
  }
  sBluedroidFunctions.bt_disable = (int (*)())dlsym(handle, "bt_disable");
  if (!sBluedroidFunctions.bt_disable) {
    NS_ERROR("Failed to attach bt_disable function");
    return false;
  }
  sBluedroidFunctions.bt_is_enabled = (int (*)())dlsym(handle, "bt_is_enabled");
  if (!sBluedroidFunctions.bt_is_enabled) {
    NS_ERROR("Failed to attach bt_is_enabled function");
    return false;
  }

  sBluedroidFunctions.initialized = true;
  return true;
}

static nsresult
StartStopGonkBluetooth(bool aShouldEnable)
{
  bool result;
  
  
  
  
  if (!EnsureBluetoothInit()) {
    NS_ERROR("Failed to load bluedroid library.\n");
    return NS_ERROR_FAILURE;
  }

  
  int isEnabled = sBluedroidFunctions.bt_is_enabled();

  if ((isEnabled == 1 && aShouldEnable) || (isEnabled == 0 && !aShouldEnable)) {
    return NS_OK;
  }
  if (aShouldEnable) {
    result = (sBluedroidFunctions.bt_enable() == 0) ? true : false;
    if (sBluedroidFunctions.bt_is_enabled() < 0) {
      
      
      
      BT_WARNING("Bluetooth firmware up, but cannot connect to HCI socket! "
        "Check bluetoothd and try stopping/starting bluetooth again.");
      
      if (sBluedroidFunctions.bt_disable() != 0) {
        BT_WARNING("Problem shutting down bluetooth after error in bringup!");
      }
      return NS_ERROR_FAILURE;
    }
  } else {
    result = (sBluedroidFunctions.bt_disable() == 0) ? true : false;
  }
  if (!result) {
    BT_WARNING("Could not set gonk bluetooth firmware!");
    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}

nsresult
BluetoothGonkService::StartInternal()
{
  MOZ_ASSERT(!NS_IsMainThread());

  nsresult ret;

  ret = StartStopGonkBluetooth(true);

  if (NS_FAILED(ret)) {
    return ret;
  }

  return BluetoothDBusService::StartInternal();
}

nsresult
BluetoothGonkService::StopInternal()
{
  MOZ_ASSERT(!NS_IsMainThread());

  nsresult ret;

  ret = BluetoothDBusService::StopInternal();

  if (NS_FAILED(ret)) {
    return ret;
  }

  return StartStopGonkBluetooth(false);
}

bool
BluetoothGonkService::IsEnabledInternal()
{
  MOZ_ASSERT(!NS_IsMainThread());

  if (!EnsureBluetoothInit()) {
    NS_ERROR("Failed to load bluedroid library.\n");
    return false;
  }

  return (sBluedroidFunctions.bt_is_enabled() == 1);
}


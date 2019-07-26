





#ifndef mozilla_dom_bluetooth_bluetoothcommon_h__
#define mozilla_dom_bluetooth_bluetoothcommon_h__

#include "mozilla/Observer.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

extern bool gBluetoothDebugFlag;

#define SWITCH_BT_DEBUG(V) (gBluetoothDebugFlag = V)

#undef BT_LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define BT_LOG(args...)                                              \
  do {                                                               \
    if (gBluetoothDebugFlag) {                                       \
      __android_log_print(ANDROID_LOG_INFO, "GeckoBluetooth", args); \
    }                                                                \
  } while(0)
#else
#define BT_LOG(args...)                                              \
  do {                                                               \
    if (gBluetoothDebugFlag) {                                       \
      printf(args);                                                  \
    }                                                                \
  } while(0)
#endif

#define BEGIN_BLUETOOTH_NAMESPACE \
  namespace mozilla { namespace dom { namespace bluetooth {
#define END_BLUETOOTH_NAMESPACE \
  } /* namespace bluetooth */ } /* namespace dom */ } /* namespace mozilla */
#define USING_BLUETOOTH_NAMESPACE \
  using namespace mozilla::dom::bluetooth;

#define LOCAL_AGENT_PATH  "/B2G/bluetooth/agent"
#define REMOTE_AGENT_PATH "/B2G/bluetooth/remote_device_agent"


#define BLUETOOTH_ADDRESS_LENGTH 17

BEGIN_BLUETOOTH_NAMESPACE

enum BluetoothSocketType {
  RFCOMM = 1,
  SCO = 2,
  L2CAP = 3
};

class BluetoothSignal;
typedef mozilla::Observer<BluetoothSignal> BluetoothSignalObserver;




enum BluetoothObjectType {
  TYPE_MANAGER = 0,
  TYPE_ADAPTER = 1,
  TYPE_DEVICE = 2,

  TYPE_INVALID
};

END_BLUETOOTH_NAMESPACE

#endif

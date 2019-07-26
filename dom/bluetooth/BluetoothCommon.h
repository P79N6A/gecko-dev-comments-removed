





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

#define BT_WARNING(args...)                                          \
  __android_log_print(ANDROID_LOG_WARN, "GeckoBluetooth", args)

#else
#define BT_LOG(args, ...)                                            \
  do {                                                               \
    if (gBluetoothDebugFlag) {                                       \
      printf(args, ##__VA_ARGS__);                                   \
    }                                                                \
  } while(0)

#define BT_WARNING(args, ...) printf(args, ##__VA_ARGS__)
#endif

#define BEGIN_BLUETOOTH_NAMESPACE \
  namespace mozilla { namespace dom { namespace bluetooth {
#define END_BLUETOOTH_NAMESPACE \
  } /* namespace bluetooth */ } /* namespace dom */ } /* namespace mozilla */
#define USING_BLUETOOTH_NAMESPACE \
  using namespace mozilla::dom::bluetooth;

#define KEY_LOCAL_AGENT  "/B2G/bluetooth/agent"
#define KEY_REMOTE_AGENT "/B2G/bluetooth/remote_device_agent"
#define KEY_MANAGER      "/B2G/bluetooth/manager"
#define KEY_ADAPTER      "/B2G/bluetooth/adapter"





#define BLUETOOTH_SCO_STATUS_CHANGED "bluetooth-sco-status-changed"
#define BLUETOOTH_A2DP_STATUS_CHANGED "bluetooth-a2dp-status-changed"


#define BLUETOOTH_ADDRESS_LENGTH 17
#define BLUETOOTH_ADDRESS_NONE   "00:00:00:00:00:00"

BEGIN_BLUETOOTH_NAMESPACE

enum BluetoothSocketType {
  RFCOMM = 1,
  SCO    = 2,
  L2CAP  = 3,
  EL2CAP = 4
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

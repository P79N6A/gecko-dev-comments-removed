





#ifndef mozilla_dom_bluetooth_bluetoothcommon_h__
#define mozilla_dom_bluetooth_bluetoothcommon_h__

#include "mozilla/Observer.h"
#include "nsPrintfCString.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

extern bool gBluetoothDebugFlag;

#define SWITCH_BT_DEBUG(V) (gBluetoothDebugFlag = V)

#undef BT_LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>




#define BT_LOGD(args...)                                             \
  do {                                                               \
    if (gBluetoothDebugFlag) {                                       \
      __android_log_print(ANDROID_LOG_INFO, "GeckoBluetooth", args); \
    }                                                                \
  } while(0)




#define BT_LOGR(args...)                                             \
  __android_log_print(ANDROID_LOG_INFO, "GeckoBluetooth", args)      \




#define BT_WARNING(args...)                                          \
  NS_WARNING(nsPrintfCString(args).get())                            \

#else
#define BT_LOGD(args, ...)                                           \
  do {                                                               \
    if (gBluetoothDebugFlag) {                                       \
      printf(args, ##__VA_ARGS__);                                   \
    }                                                                \
  } while(0)

#define BT_LOGR(args, ...) printf(args, ##__VA_ARGS__)
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





#define BLUETOOTH_A2DP_STATUS_CHANGED_ID "bluetooth-a2dp-status-changed"
#define BLUETOOTH_HFP_STATUS_CHANGED_ID  "bluetooth-hfp-status-changed"
#define BLUETOOTH_HID_STATUS_CHANGED_ID  "bluetooth-hid-status-changed"
#define BLUETOOTH_SCO_STATUS_CHANGED_ID  "bluetooth-sco-status-changed"





#define A2DP_STATUS_CHANGED_ID               "a2dpstatuschanged"
#define HFP_STATUS_CHANGED_ID                "hfpstatuschanged"
#define SCO_STATUS_CHANGED_ID                "scostatuschanged"





#define PAIRED_STATUS_CHANGED_ID             "pairedstatuschanged"





#define REQUEST_MEDIA_PLAYSTATUS_ID          "requestmediaplaystatus"


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

enum ControlPlayStatus {
  PLAYSTATUS_STOPPED  = 0x00,
  PLAYSTATUS_PLAYING  = 0x01,
  PLAYSTATUS_PAUSED   = 0x02,
  PLAYSTATUS_FWD_SEEK = 0x03,
  PLAYSTATUS_REV_SEEK = 0x04,
  PLAYSTATUS_UNKNOWN,
  PLAYSTATUS_ERROR    = 0xFF,
};

END_BLUETOOTH_NAMESPACE

#endif

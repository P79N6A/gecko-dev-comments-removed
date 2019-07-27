





#include "BluetoothInterface.h"
#if ANDROID_VERSION >= 17
#include <cutils/properties.h>
#endif
#ifdef MOZ_B2G_BT_BLUEDROID
#include "BluetoothHALInterface.h"
#endif
#ifdef MOZ_B2G_BT_DAEMON
#include "BluetoothDaemonInterface.h"
#endif

BEGIN_BLUETOOTH_NAMESPACE





BluetoothSocketInterface::~BluetoothSocketInterface()
{ }








BluetoothHandsfreeNotificationHandler::
  ~BluetoothHandsfreeNotificationHandler()
{ }




BluetoothHandsfreeInterface::BluetoothHandsfreeInterface()
{ }

BluetoothHandsfreeInterface::~BluetoothHandsfreeInterface()
{ }








BluetoothA2dpNotificationHandler::~BluetoothA2dpNotificationHandler()
{ }




BluetoothA2dpInterface::BluetoothA2dpInterface()
{ }

BluetoothA2dpInterface::~BluetoothA2dpInterface()
{ }








BluetoothAvrcpNotificationHandler::~BluetoothAvrcpNotificationHandler()
{ }




BluetoothAvrcpInterface::BluetoothAvrcpInterface()
{ }

BluetoothAvrcpInterface::~BluetoothAvrcpInterface()
{ }




BluetoothNotificationHandler::~BluetoothNotificationHandler()
{ }




BluetoothInterface*
BluetoothInterface::GetInstance()
{
#if ANDROID_VERSION >= 17
  






  static const char* const sDefaultBackend[] = {
#ifdef MOZ_B2G_BT_DAEMON
    "bluetoothd",
#endif
#ifdef MOZ_B2G_BT_BLUEDROID
    "bluedroid",
#endif
    nullptr 
  };

  const char* defaultBackend;

  for (size_t i = 0; i < MOZ_ARRAY_LENGTH(sDefaultBackend); ++i) {

    
    defaultBackend = sDefaultBackend[i];

    if (defaultBackend) {
      if (!strcmp(defaultBackend, "bluetoothd") &&
          access("/init.bluetooth.rc", F_OK) == -1) {
        continue; 
      }
    }
    break;
  }

  char value[PROPERTY_VALUE_MAX];
  int len;

  len = property_get("ro.moz.bluetooth.backend", value, defaultBackend);
  if (len < 0) {
    BT_WARNING("No Bluetooth backend available.");
    return nullptr;
  }

  const nsDependentCString backend(value, len);

  





#ifdef MOZ_B2G_BT_BLUEDROID
  if (backend.LowerCaseEqualsLiteral("bluedroid")) {
    return BluetoothHALInterface::GetInstance();
  } else
#endif
#ifdef MOZ_B2G_BT_DAEMON
  if (backend.LowerCaseEqualsLiteral("bluetoothd")) {
    return BluetoothDaemonInterface::GetInstance();
  } else
#endif
  {
    BT_WARNING("Bluetooth backend '%s' is unknown or not available.",
               backend.get());
  }
  return nullptr;

#else
  


  BT_WARNING("No Bluetooth backend available for your system.");
  return nullptr;
#endif
}

BluetoothInterface::BluetoothInterface()
{ }

BluetoothInterface::~BluetoothInterface()
{ }

END_BLUETOOTH_NAMESPACE

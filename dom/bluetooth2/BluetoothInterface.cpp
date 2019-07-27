





#include "BluetoothInterface.h"
#ifdef MOZ_B2G_BT_BLUEDROID
#include "BluetoothHALInterface.h"
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
  




#ifdef MOZ_B2G_BT_BLUEDROID
  return BluetoothHALInterface::GetInstance();
#else
  return nullptr;
#endif
}

BluetoothInterface::BluetoothInterface()
{ }

BluetoothInterface::~BluetoothInterface()
{ }

END_BLUETOOTH_NAMESPACE







#include "BluetoothDaemonSetupInterface.h"

BEGIN_BLUETOOTH_NAMESPACE

BluetoothSetupResultHandler::~BluetoothSetupResultHandler()
{ }

void
BluetoothSetupResultHandler::OnError(BluetoothStatus aStatus)
{
  BT_WARNING("Received error code %d", (int)aStatus);
}

void
BluetoothSetupResultHandler::RegisterModule()
{ }

void
BluetoothSetupResultHandler::UnregisterModule()
{ }

void
BluetoothSetupResultHandler::Configuration()
{ }

END_BLUETOOTH_NAMESPACE

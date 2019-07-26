





#ifndef mozilla_dom_bluetooth_BluetoothSocketObserver_h
#define mozilla_dom_bluetooth_BluetoothSocketObserver_h

#include "BluetoothCommon.h"
#include <mozilla/ipc/UnixSocket.h>

using namespace mozilla::ipc;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSocketObserver
{
public:
  virtual void ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage) = 0;
  virtual void OnConnectSuccess() = 0;
  virtual void OnConnectError() = 0;
  virtual void OnDisconnect() = 0;
};

END_BLUETOOTH_NAMESPACE

#endif

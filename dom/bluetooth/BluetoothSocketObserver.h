





#ifndef mozilla_dom_bluetooth_BluetoothSocketObserver_h
#define mozilla_dom_bluetooth_BluetoothSocketObserver_h

#include "BluetoothCommon.h"
#include <mozilla/ipc/UnixSocket.h>

using namespace mozilla::ipc;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSocket;

class BluetoothSocketObserver
{
public:
  virtual void ReceiveSocketData(BluetoothSocket* aSocket,
                                 nsAutoPtr<UnixSocketRawData>& aMessage) = 0;
 
   




  virtual void OnConnectSuccess(BluetoothSocket* aSocket) = 0;
 
   



  virtual void OnConnectError(BluetoothSocket* aSocket) = 0;
 
   




  virtual void OnDisconnect(BluetoothSocket* aSocket) = 0;

};

END_BLUETOOTH_NAMESPACE

#endif

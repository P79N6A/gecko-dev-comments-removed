





#ifndef mozilla_dom_bluetooth_BluetoothSocketObserver_h
#define mozilla_dom_bluetooth_BluetoothSocketObserver_h

#include "BluetoothCommon.h"
#include "mozilla/ipc/SocketBase.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSocket;

class BluetoothSocketObserver
{
public:
  virtual void ReceiveSocketData(
    BluetoothSocket* aSocket,
    nsAutoPtr<mozilla::ipc::UnixSocketBuffer>& aBuffer) = 0;

   




  virtual void OnSocketConnectSuccess(BluetoothSocket* aSocket) = 0;

   



  virtual void OnSocketConnectError(BluetoothSocket* aSocket) = 0;

   




  virtual void OnSocketDisconnect(BluetoothSocket* aSocket) = 0;
};

#define BT_DECL_SOCKET_OBSERVER                                             \
public:                                                                     \
  virtual void ReceiveSocketData(BluetoothSocket* aSocket,                  \
    nsAutoPtr<mozilla::ipc::UnixSocketBuffer>& aMessage) override;          \
  virtual void OnSocketConnectSuccess(BluetoothSocket* aSocket) override;   \
  virtual void OnSocketConnectError(BluetoothSocket* aSocket) override;     \
  virtual void OnSocketDisconnect(BluetoothSocket* aSocket) override;

END_BLUETOOTH_NAMESPACE

#endif

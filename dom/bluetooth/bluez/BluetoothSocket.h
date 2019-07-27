





#ifndef mozilla_dom_bluetooth_BluetoothSocket_h
#define mozilla_dom_bluetooth_BluetoothSocket_h

#include "BluetoothCommon.h"
#include <stdlib.h>
#include "mozilla/ipc/DataSocket.h"
#include "mozilla/ipc/UnixSocketWatcher.h"
#include "mozilla/RefPtr.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsThreadUtils.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSocketObserver;
class BluetoothUnixSocketConnector;

class BluetoothSocket final : public mozilla::ipc::DataSocket
{
public:
  BluetoothSocket(BluetoothSocketObserver* aObserver,
                  BluetoothSocketType aType,
                  bool aAuth,
                  bool aEncrypt);
  ~BluetoothSocket();

  bool Connect(const nsAString& aDeviceAddress,
               const BluetoothUuid& aServiceUuid,
               int aChannel);
  bool Listen(const nsAString& aServiceName,
              const BluetoothUuid& aServiceUuid,
              int aChannel);

  





  void ReceiveSocketData(nsAutoPtr<mozilla::ipc::UnixSocketBuffer>& aBuffer);

  








  bool SendSocketData(const nsACString& aMessage);

  







  nsresult Connect(BluetoothUnixSocketConnector* aConnector,
                   int aDelayMs = 0);

  






  nsresult Listen(BluetoothUnixSocketConnector* aConnector);

  




  void GetAddress(nsAString& aDeviceAddress);

  
  

  void SendSocketData(mozilla::ipc::UnixSocketIOBuffer* aBuffer) override;

  
  

  void Close() override;

  void OnConnectSuccess() override;
  void OnConnectError() override;
  void OnDisconnect() override;

private:
  class BluetoothSocketIO;
  class ConnectTask;
  class DelayedConnectTask;
  class ListenTask;

  BluetoothSocketObserver* mObserver;
  BluetoothSocketType mType;
  bool mAuth;
  bool mEncrypt;
  BluetoothSocketIO* mIO;
};

END_BLUETOOTH_NAMESPACE

#endif

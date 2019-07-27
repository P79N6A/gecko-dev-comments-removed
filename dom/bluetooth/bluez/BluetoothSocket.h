





#ifndef mozilla_dom_bluetooth_BluetoothSocket_h
#define mozilla_dom_bluetooth_BluetoothSocket_h

#include "BluetoothCommon.h"
#include "mozilla/ipc/DataSocket.h"
#include "mozilla/ipc/UnixSocketWatcher.h"
#include "nsAutoPtr.h"
#include "nsString.h"

class MessageLoop;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSocketObserver;
class BluetoothUnixSocketConnector;

class BluetoothSocket final : public mozilla::ipc::DataSocket
{
public:
  BluetoothSocket(BluetoothSocketObserver* aObserver);
  ~BluetoothSocket();

  nsresult Connect(const nsAString& aDeviceAddress,
                   const BluetoothUuid& aServiceUuid,
                   BluetoothSocketType aType,
                   int aChannel,
                   bool aAuth, bool aEncrypt);

  nsresult Listen(const nsAString& aServiceName,
                  const BluetoothUuid& aServiceUuid,
                  BluetoothSocketType aType,
                  int aChannel,
                  bool aAuth, bool aEncrypt);

  





  void ReceiveSocketData(nsAutoPtr<mozilla::ipc::UnixSocketBuffer>& aBuffer);

  








  bool SendSocketData(const nsACString& aMessage);

  









  nsresult Connect(BluetoothUnixSocketConnector* aConnector, int aDelayMs,
                   MessageLoop* aConsumerLoop, MessageLoop* aIOLoop);

  







  nsresult Connect(BluetoothUnixSocketConnector* aConnector,
                   int aDelayMs = 0);

  








  nsresult Listen(BluetoothUnixSocketConnector* aConnector,
                  MessageLoop* aConsumerLoop, MessageLoop* aIOLoop);

  






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
  BluetoothSocketIO* mIO;
};

END_BLUETOOTH_NAMESPACE

#endif







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
  inline void Disconnect()
  {
    CloseSocket();
  }

  virtual void OnConnectSuccess() override;
  virtual void OnConnectError() override;
  virtual void OnDisconnect() override;
  virtual void ReceiveSocketData(
    nsAutoPtr<mozilla::ipc::UnixSocketBuffer>& aBuffer) override;

  inline void GetAddress(nsAString& aDeviceAddress)
  {
    GetSocketAddr(aDeviceAddress);
  }

  





  void SendSocketData(mozilla::ipc::UnixSocketIOBuffer* aBuffer) override;

  








  bool SendSocketData(const nsACString& aMessage);

  









  bool ConnectSocket(BluetoothUnixSocketConnector* aConnector,
                     const char* aAddress,
                     int aDelayMs = 0);

  







  bool ListenSocket(BluetoothUnixSocketConnector* aConnector);

  



  void CloseSocket() override;

  


  void GetSocketAddr(nsAString& aAddrStr);

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

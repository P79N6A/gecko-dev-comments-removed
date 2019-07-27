





#ifndef mozilla_dom_bluetooth_BluetoothSocket_h
#define mozilla_dom_bluetooth_BluetoothSocket_h

#include "BluetoothCommon.h"
#include "mozilla/ipc/UnixSocket.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSocketObserver;

class BluetoothSocket : public mozilla::ipc::UnixSocketConsumer
{
public:
  BluetoothSocket(BluetoothSocketObserver* aObserver,
                  BluetoothSocketType aType,
                  bool aAuth,
                  bool aEncrypt);

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
    nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage) override;

  inline void GetAddress(nsAString& aDeviceAddress)
  {
    GetSocketAddr(aDeviceAddress);
  }

private:
  BluetoothSocketObserver* mObserver;
  BluetoothSocketType mType;
  bool mAuth;
  bool mEncrypt;
};

END_BLUETOOTH_NAMESPACE

#endif

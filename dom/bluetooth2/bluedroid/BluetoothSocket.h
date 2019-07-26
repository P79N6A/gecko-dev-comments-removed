





#ifndef mozilla_dom_bluetooth_BluetoothSocket_h
#define mozilla_dom_bluetooth_BluetoothSocket_h

#include "BluetoothCommon.h"
#include "mozilla/ipc/UnixSocket.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSocketObserver;
class DroidSocketImpl;

class BluetoothSocket : public mozilla::ipc::UnixSocketConsumer
{
public:
  BluetoothSocket(BluetoothSocketObserver* aObserver,
                  BluetoothSocketType aType,
                  bool aAuth,
                  bool aEncrypt);

  








  bool Connect(const nsAString& aDeviceAddress, int aChannel);

  












  bool Listen(int aChannel);

  inline void Disconnect()
  {
    CloseDroidSocket();
  }

  virtual void OnConnectSuccess() MOZ_OVERRIDE;
  virtual void OnConnectError() MOZ_OVERRIDE;
  virtual void OnDisconnect() MOZ_OVERRIDE;
  virtual void ReceiveSocketData(
    nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage) MOZ_OVERRIDE;

  inline void GetAddress(nsAString& aDeviceAddress)
  {
    aDeviceAddress = mDeviceAddress;
  }

  void CloseDroidSocket();
  bool SendDroidSocketData(mozilla::ipc::UnixSocketRawData* aData);

private:
  BluetoothSocketObserver* mObserver;
  DroidSocketImpl* mImpl;
  nsString mDeviceAddress;
  bool mAuth;
  bool mEncrypt;
  bool mIsServer;
  int mReceivedSocketInfoLength;

  bool ReceiveSocketInfo(nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage);
};

END_BLUETOOTH_NAMESPACE

#endif

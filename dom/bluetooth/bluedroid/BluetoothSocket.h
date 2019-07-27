





#ifndef mozilla_dom_bluetooth_BluetoothSocket_h
#define mozilla_dom_bluetooth_BluetoothSocket_h

#include "BluetoothCommon.h"
#include "mozilla/ipc/DataSocket.h"

class MessageLoop;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSocketObserver;
class BluetoothSocketResultHandler;
class DroidSocketImpl;

class BluetoothSocket final : public mozilla::ipc::DataSocket
{
public:
  BluetoothSocket(BluetoothSocketObserver* aObserver);

  nsresult Connect(const nsAString& aDeviceAddress,
                   const BluetoothUuid& aServiceUuid,
                   BluetoothSocketType aType,
                   int aChannel,
                   bool aAuth, bool aEncrypt,
                   MessageLoop* aConsumerLoop,
                   MessageLoop* aIOLoop);

  nsresult Connect(const nsAString& aDeviceAddress,
                   const BluetoothUuid& aServiceUuid,
                   BluetoothSocketType aType,
                   int aChannel,
                   bool aAuth, bool aEncrypt);

  nsresult Listen(const nsAString& aServiceName,
                  const BluetoothUuid& aServiceUuid,
                  BluetoothSocketType aType,
                  int aChannel,
                  bool aAuth, bool aEncrypt,
                  MessageLoop* aConsumerLoop,
                  MessageLoop* aIOLoop);

  nsresult Listen(const nsAString& aServiceName,
                  const BluetoothUuid& aServiceUuid,
                  BluetoothSocketType aType,
                  int aChannel,
                  bool aAuth, bool aEncrypt);

  





  void ReceiveSocketData(nsAutoPtr<mozilla::ipc::UnixSocketBuffer>& aBuffer);

  inline void GetAddress(nsAString& aDeviceAddress)
  {
    aDeviceAddress = mDeviceAddress;
  }

  inline void SetAddress(const nsAString& aDeviceAddress)
  {
    mDeviceAddress = aDeviceAddress;
  }

  inline void SetCurrentResultHandler(BluetoothSocketResultHandler* aRes)
  {
    mCurrentRes = aRes;
  }

  
  

  void SendSocketData(mozilla::ipc::UnixSocketIOBuffer* aBuffer) override;

  
  

  void Close() override;

  void OnConnectSuccess() override;
  void OnConnectError() override;
  void OnDisconnect() override;

private:
  BluetoothSocketObserver* mObserver;
  BluetoothSocketResultHandler* mCurrentRes;
  DroidSocketImpl* mImpl;
  nsString mDeviceAddress;
};

END_BLUETOOTH_NAMESPACE

#endif

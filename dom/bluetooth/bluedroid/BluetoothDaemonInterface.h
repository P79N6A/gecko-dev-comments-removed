





#ifndef mozilla_dom_bluetooth_bluedroid_bluetoothdaemoninterface_h__
#define mozilla_dom_bluetooth_bluedroid_bluetoothdaemoninterface_h__

#include "BluetoothInterface.h"
#include "mozilla/ipc/BluetoothDaemonConnectionConsumer.h"
#include "mozilla/ipc/ListenSocketConsumer.h"

namespace mozilla {
namespace ipc {

class BluetoothDaemonConnection;
class ListenSocket;

}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothDaemonA2dpInterface;
class BluetoothDaemonAvrcpInterface;
class BluetoothDaemonGattInterface;
class BluetoothDaemonHandsfreeInterface;
class BluetoothDaemonProtocol;
class BluetoothDaemonSocketInterface;

class BluetoothDaemonInterface final
  : public BluetoothInterface
  , public mozilla::ipc::DaemonSocketConsumer
  , public mozilla::ipc::ListenSocketConsumer
{
public:
  class CleanupResultHandler;
  class InitResultHandler;
  class StartDaemonTask;

  friend class CleanupResultHandler;
  friend class InitResultHandler;
  friend class StartDaemonTask;

  static BluetoothDaemonInterface* GetInstance();

  void Init(BluetoothNotificationHandler* aNotificationHandler,
            BluetoothResultHandler* aRes);
  void Cleanup(BluetoothResultHandler* aRes);

  void Enable(BluetoothResultHandler* aRes);
  void Disable(BluetoothResultHandler* aRes);

  

  void GetAdapterProperties(BluetoothResultHandler* aRes);
  void GetAdapterProperty(const nsAString& aName,
                          BluetoothResultHandler* aRes);
  void SetAdapterProperty(const BluetoothNamedValue& aProperty,
                          BluetoothResultHandler* aRes);

  

  void GetRemoteDeviceProperties(const nsAString& aRemoteAddr,
                                 BluetoothResultHandler* aRes);
  void GetRemoteDeviceProperty(const nsAString& aRemoteAddr,
                               const nsAString& aName,
                               BluetoothResultHandler* aRes);
  void SetRemoteDeviceProperty(const nsAString& aRemoteAddr,
                               const BluetoothNamedValue& aProperty,
                               BluetoothResultHandler* aRes);

  

  void GetRemoteServiceRecord(const nsAString& aRemoteAddr,
                              const uint8_t aUuid[16],
                              BluetoothResultHandler* aRes);
  void GetRemoteServices(const nsAString& aRemoteAddr,
                         BluetoothResultHandler* aRes);

  

  void StartDiscovery(BluetoothResultHandler* aRes);
  void CancelDiscovery(BluetoothResultHandler* aRes);

  

  void CreateBond(const nsAString& aBdAddr, BluetoothTransport aTransport,
                  BluetoothResultHandler* aRes);
  void RemoveBond(const nsAString& aBdAddr, BluetoothResultHandler* aRes);
  void CancelBond(const nsAString& aBdAddr, BluetoothResultHandler* aRes);

  

  void GetConnectionState(const nsAString& aBdAddr,
                          BluetoothResultHandler* aRes);

  

  void PinReply(const nsAString& aBdAddr, bool aAccept,
                const nsAString& aPinCode,
                BluetoothResultHandler* aRes);

  void SspReply(const nsAString& aBdAddr, BluetoothSspVariant aVariant,
                bool aAccept, uint32_t aPasskey,
                BluetoothResultHandler* aRes);

  

  void DutModeConfigure(bool aEnable, BluetoothResultHandler* aRes);
  void DutModeSend(uint16_t aOpcode, uint8_t* aBuf, uint8_t aLen,
                   BluetoothResultHandler* aRes);

  

  void LeTestMode(uint16_t aOpcode, uint8_t* aBuf, uint8_t aLen,
                  BluetoothResultHandler* aRes);

  

  void ReadEnergyInfo(BluetoothResultHandler* aRes);

  

  BluetoothSocketInterface* GetBluetoothSocketInterface() override;
  BluetoothHandsfreeInterface* GetBluetoothHandsfreeInterface() override;
  BluetoothA2dpInterface* GetBluetoothA2dpInterface() override;
  BluetoothAvrcpInterface* GetBluetoothAvrcpInterface() override;
  BluetoothGattInterface* GetBluetoothGattInterface() override;

protected:
  enum Channel {
    LISTEN_SOCKET,
    CMD_CHANNEL,
    NTF_CHANNEL
  };

  BluetoothDaemonInterface();
  ~BluetoothDaemonInterface();

  nsresult CreateRandomAddressString(const nsACString& aPrefix,
                                     unsigned long aPostfixLength,
                                     nsACString& aAddress);

  
  

  void OnConnectSuccess(int aIndex) override;
  void OnConnectError(int aIndex) override;
  void OnDisconnect(int aIndex) override;

private:
  void DispatchError(BluetoothResultHandler* aRes, BluetoothStatus aStatus);
  void DispatchError(BluetoothResultHandler* aRes, nsresult aRv);

  nsCString mListenSocketName;
  nsRefPtr<mozilla::ipc::ListenSocket> mListenSocket;
  nsRefPtr<mozilla::ipc::BluetoothDaemonConnection> mCmdChannel;
  nsRefPtr<mozilla::ipc::BluetoothDaemonConnection> mNtfChannel;
  nsAutoPtr<BluetoothDaemonProtocol> mProtocol;

  nsTArray<nsRefPtr<BluetoothResultHandler> > mResultHandlerQ;

  nsAutoPtr<BluetoothDaemonSocketInterface> mSocketInterface;
  nsAutoPtr<BluetoothDaemonHandsfreeInterface> mHandsfreeInterface;
  nsAutoPtr<BluetoothDaemonA2dpInterface> mA2dpInterface;
  nsAutoPtr<BluetoothDaemonAvrcpInterface> mAvrcpInterface;
  nsAutoPtr<BluetoothDaemonGattInterface> mGattInterface;
};

END_BLUETOOTH_NAMESPACE

#endif

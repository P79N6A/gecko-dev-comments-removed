





#ifndef mozilla_dom_bluetooth_bluedroid_bluetoothdaemoninterface_h__
#define mozilla_dom_bluetooth_bluedroid_bluetoothdaemoninterface_h__

#include "BluetoothInterface.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothDaemonChannel;
class BluetoothDaemonProtocol;
class BluetoothDaemonSocketInterface;

class BluetoothDaemonInterface MOZ_FINAL : public BluetoothInterface
{
public:
  class CleanupResultHandler;
  class InitResultHandler;

  friend class BluetoothDaemonChannel;
  friend class CleanupResultHandler;
  friend class InitResultHandler;

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

  

  void CreateBond(const nsAString& aBdAddr, BluetoothResultHandler* aRes);
  void RemoveBond(const nsAString& aBdAddr, BluetoothResultHandler* aRes);
  void CancelBond(const nsAString& aBdAddr, BluetoothResultHandler* aRes);

  

  void PinReply(const nsAString& aBdAddr, bool aAccept,
                const nsAString& aPinCode,
                BluetoothResultHandler* aRes);

  void SspReply(const nsAString& aBdAddr, const nsAString& aVariant,
                bool aAccept, uint32_t aPasskey,
                BluetoothResultHandler* aRes);

  

  void DutModeConfigure(bool aEnable, BluetoothResultHandler* aRes);
  void DutModeSend(uint16_t aOpcode, uint8_t* aBuf, uint8_t aLen,
                   BluetoothResultHandler* aRes);

  

  void LeTestMode(uint16_t aOpcode, uint8_t* aBuf, uint8_t aLen,
                  BluetoothResultHandler* aRes);

  

  BluetoothSocketInterface* GetBluetoothSocketInterface() MOZ_OVERRIDE;
  BluetoothHandsfreeInterface* GetBluetoothHandsfreeInterface() MOZ_OVERRIDE;
  BluetoothA2dpInterface* GetBluetoothA2dpInterface() MOZ_OVERRIDE;
  BluetoothAvrcpInterface* GetBluetoothAvrcpInterface() MOZ_OVERRIDE;
  BluetoothGattInterface* GetBluetoothGattInterface() MOZ_OVERRIDE;

protected:
  enum Channel {
    CMD_CHANNEL,
    NTF_CHANNEL
  };

  BluetoothDaemonInterface(BluetoothDaemonChannel* aCmdChannel,
                           BluetoothDaemonChannel* aNtfChannel,
                           BluetoothDaemonProtocol* aProtocol);
  ~BluetoothDaemonInterface();

  void OnConnectSuccess(enum Channel aChannel);
  void OnConnectError(enum Channel aChannel);
  void OnDisconnect(enum Channel aChannel);

private:
  void DispatchError(BluetoothResultHandler* aRes, BluetoothStatus aStatus);

  nsAutoPtr<BluetoothDaemonChannel> mCmdChannel;
  nsAutoPtr<BluetoothDaemonChannel> mNtfChannel;
  nsAutoPtr<BluetoothDaemonProtocol> mProtocol;

  nsTArray<nsRefPtr<BluetoothResultHandler> > mResultHandlerQ;

  nsAutoPtr<BluetoothDaemonSocketInterface> mSocketInterface;
};

END_BLUETOOTH_NAMESPACE

#endif

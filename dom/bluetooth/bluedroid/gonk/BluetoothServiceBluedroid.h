





#ifndef mozilla_dom_bluetooth_bluetoothservicebluedroid_h__
#define mozilla_dom_bluetooth_bluetoothservicebluedroid_h__

#include "BluetoothCommon.h"
#include "BluetoothService.h"

class DBusMessage;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothServiceBluedroid : public BluetoothService
{
public:
  virtual nsresult StartInternal();
  virtual nsresult StopInternal();
  virtual bool IsEnabledInternal();

  virtual nsresult GetDefaultAdapterPathInternal(
                                             BluetoothReplyRunnable* aRunnable);

  virtual nsresult GetConnectedDevicePropertiesInternal(uint16_t aProfileId,
                                             BluetoothReplyRunnable* aRunnable);

  virtual nsresult GetPairedDevicePropertiesInternal(
                                     const nsTArray<nsString>& aDeviceAddress,
                                     BluetoothReplyRunnable* aRunnable);

  virtual nsresult StartDiscoveryInternal(BluetoothReplyRunnable* aRunnable);
  virtual nsresult StopDiscoveryInternal(BluetoothReplyRunnable* aRunnable);

  virtual nsresult
  GetDevicePropertiesInternal(const BluetoothSignal& aSignal);

  virtual nsresult
  SetProperty(BluetoothObjectType aType,
              const BluetoothNamedValue& aValue,
              BluetoothReplyRunnable* aRunnable);

  virtual bool
  GetDevicePath(const nsAString& aAdapterPath,
                const nsAString& aDeviceAddress,
                nsAString& aDevicePath);

  static bool
  AddServiceRecords(const char* serviceName,
                    unsigned long long uuidMsb,
                    unsigned long long uuidLsb,
                    int channel);

  static bool
  RemoveServiceRecords(const char* serviceName,
                       unsigned long long uuidMsb,
                       unsigned long long uuidLsb,
                       int channel);

  static bool
  AddReservedServicesInternal(const nsTArray<uint32_t>& aServices,
                              nsTArray<uint32_t>& aServiceHandlesContainer);

  static bool
  RemoveReservedServicesInternal(const nsTArray<uint32_t>& aServiceHandles);

  virtual nsresult
  GetScoSocket(const nsAString& aObjectPath,
               bool aAuth,
               bool aEncrypt,
               mozilla::ipc::UnixSocketConsumer* aConsumer);

  virtual nsresult
  GetServiceChannel(const nsAString& aDeviceAddress,
                    const nsAString& aServiceUuid,
                    BluetoothProfileManagerBase* aManager);

  virtual bool
  UpdateSdpRecords(const nsAString& aDeviceAddress,
                   BluetoothProfileManagerBase* aManager);

  virtual nsresult
  CreatePairedDeviceInternal(const nsAString& aDeviceAddress,
                             int aTimeout,
                             BluetoothReplyRunnable* aRunnable);

  virtual nsresult
  RemoveDeviceInternal(const nsAString& aDeviceObjectPath,
                       BluetoothReplyRunnable* aRunnable);

  virtual bool
  SetPinCodeInternal(const nsAString& aDeviceAddress, const nsAString& aPinCode,
                     BluetoothReplyRunnable* aRunnable);

  virtual bool
  SetPasskeyInternal(const nsAString& aDeviceAddress, uint32_t aPasskey,
                     BluetoothReplyRunnable* aRunnable);

  virtual bool
  SetPairingConfirmationInternal(const nsAString& aDeviceAddress, bool aConfirm,
                                 BluetoothReplyRunnable* aRunnable);

  virtual bool
  SetAuthorizationInternal(const nsAString& aDeviceAddress, bool aAllow,
                           BluetoothReplyRunnable* aRunnable);

  virtual nsresult
  PrepareAdapterInternal();

  virtual void
  Connect(const nsAString& aDeviceAddress,
          uint32_t aCod,
          uint16_t aServiceUuid,
          BluetoothReplyRunnable* aRunnable);

  virtual bool
  IsConnected(uint16_t aProfileId);

  virtual void
  Disconnect(const nsAString& aDeviceAddress, uint16_t aServiceUuid,
             BluetoothReplyRunnable* aRunnable);

  virtual void
  SendFile(const nsAString& aDeviceAddress,
           BlobParent* aBlobParent,
           BlobChild* aBlobChild,
           BluetoothReplyRunnable* aRunnable);

  virtual void
  StopSendingFile(const nsAString& aDeviceAddress,
                  BluetoothReplyRunnable* aRunnable);

  virtual void
  ConfirmReceivingFile(const nsAString& aDeviceAddress, bool aConfirm,
                       BluetoothReplyRunnable* aRunnable);

  virtual void
  ConnectSco(BluetoothReplyRunnable* aRunnable);

  virtual void
  DisconnectSco(BluetoothReplyRunnable* aRunnable);

  virtual void
  IsScoConnected(BluetoothReplyRunnable* aRunnable);

  virtual void
  AnswerWaitingCall(BluetoothReplyRunnable* aRunnable);

  virtual void
  IgnoreWaitingCall(BluetoothReplyRunnable* aRunnable);

  virtual void
  ToggleCalls(BluetoothReplyRunnable* aRunnable);

  virtual void
  SendMetaData(const nsAString& aTitle,
               const nsAString& aArtist,
               const nsAString& aAlbum,
               int64_t aMediaNumber,
               int64_t aTotalMediaCount,
               int64_t aDuration,
               BluetoothReplyRunnable* aRunnable) MOZ_OVERRIDE;

  virtual void
  SendPlayStatus(int64_t aDuration,
                 int64_t aPosition,
                 const nsAString& aPlayStatus,
                 BluetoothReplyRunnable* aRunnable) MOZ_OVERRIDE;

  virtual void
  UpdatePlayStatus(uint32_t aDuration,
                   uint32_t aPosition,
                   ControlPlayStatus aPlayStatus) MOZ_OVERRIDE;

  virtual nsresult
  SendSinkMessage(const nsAString& aDeviceAddresses,
                  const nsAString& aMessage) MOZ_OVERRIDE;

  virtual nsresult
  SendInputMessage(const nsAString& aDeviceAddresses,
                   const nsAString& aMessage) MOZ_OVERRIDE;
};

END_BLUETOOTH_NAMESPACE

#endif


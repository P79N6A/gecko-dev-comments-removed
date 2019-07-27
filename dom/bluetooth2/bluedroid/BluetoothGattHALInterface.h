





#ifndef mozilla_dom_bluetooth_bluedroid_bluetoothgatthalinterface_h__
#define mozilla_dom_bluetooth_bluedroid_bluetoothgatthalinterface_h__

#include <hardware/bluetooth.h>
#if ANDROID_VERSION >= 19
#include <hardware/bt_gatt.h>
#endif
#include "BluetoothCommon.h"
#include "BluetoothInterface.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothHALInterface;

class BluetoothGattClientHALInterface MOZ_FINAL
  : public BluetoothGattClientInterface
{
public:
  friend class BluetoothGattHALInterface;

  
  void RegisterClient(const BluetoothUuid& aUuid,
                      BluetoothGattClientResultHandler* aRes);
  void UnregisterClient(int aClientIf,
                        BluetoothGattClientResultHandler* aRes);

  
  void Scan(int aClientIf, bool aStart,
            BluetoothGattClientResultHandler* aRes);

  
  void Connect(int aClientIf,
               const nsAString& aBdAddr,
               bool aIsDirect, 
               BluetoothGattClientResultHandler* aRes);
  void Disconnect(int aClientIf,
                  const nsAString& aBdAddr,
                  int aConnId,
                  BluetoothGattClientResultHandler* aRes);

  
  void Listen(int aClientIf,
              bool aIsStart,
              BluetoothGattClientResultHandler* aRes);

  
  void Refresh(int aClientIf,
               const nsAString& aBdAddr,
               BluetoothGattClientResultHandler* aRes);

  
  void SearchService(int aConnId,
                     const BluetoothUuid& aUuid,
                     BluetoothGattClientResultHandler* aRes);
  void GetIncludedService(int aConnId,
                          const BluetoothGattServiceId& aServiceId,
                          const BluetoothGattServiceId& aStartServiceId,
                          BluetoothGattClientResultHandler* aRes);
  void GetCharacteristic(int aConnId,
                         const BluetoothGattServiceId& aServiceId,
                         const BluetoothGattId& aStartCharId,
                         BluetoothGattClientResultHandler* aRes);
  void GetDescriptor(int aConnId,
                     const BluetoothGattServiceId& aServiceId,
                     const BluetoothGattId& aCharId,
                     const BluetoothGattId& aDescriptorId,
                     BluetoothGattClientResultHandler* aRes);

  
  void ReadCharacteristic(int aConnId,
                          const BluetoothGattServiceId& aServiceId,
                          const BluetoothGattId& aCharId,
                          int aAuthReq,
                          BluetoothGattClientResultHandler* aRes);
  void WriteCharacteristic(int aConnId,
                           const BluetoothGattServiceId& aServiceId,
                           const BluetoothGattId& aCharId,
                           int aWriteType,
                           int aLen,
                           int aAuthReq,
                           const ArrayBuffer& aValue,
                           BluetoothGattClientResultHandler* aRes);
  void ReadDescriptor(int aConnId,
                      const BluetoothGattServiceId& aServiceId,
                      const BluetoothGattId& aCharId,
                      const BluetoothGattId& aDescriptorId,
                      int aAuthReq,
                      BluetoothGattClientResultHandler* aRes);
  void WriteDescriptor(int aConnId,
                       const BluetoothGattServiceId& aServiceId,
                       const BluetoothGattId& aCharId,
                       const BluetoothGattId& aDescriptorId,
                       int aWriteType,
                       int aLen,
                       int aAuthReq,
                       const ArrayBuffer& aValue,
                       BluetoothGattClientResultHandler* aRes);

  
  void ExecuteWrite(int aConnId,
                    int aIsExecute,
                    BluetoothGattClientResultHandler* aRes);


  
  void RegisterNotification(int aClientIf,
                            const nsAString& aBdAddr,
                            const BluetoothGattServiceId& aServiceId,
                            const BluetoothGattId& aCharId,
                            BluetoothGattClientResultHandler* aRes);
  void DeregisterNotification(int aClientIf,
                              const nsAString& aBdAddr,
                              const BluetoothGattServiceId& aServiceId,
                              const BluetoothGattId& aCharId,
                              BluetoothGattClientResultHandler* aRes);

  void ReadRemoteRssi(int aClientIf,
                      const nsAString& aBdAddr,
                      BluetoothGattClientResultHandler* aRes);

  void GetDeviceType(const nsAString& aBdAddr,
                     BluetoothGattClientResultHandler* aRes);

  
  void SetAdvData(int aServerIf,
                  bool aIsScanRsp,
                  bool aIsNameIncluded,
                  bool aIsTxPowerIncluded,
                  int aMinInterval,
                  int aMaxInterval,
                  int aApperance,
                  uint8_t aManufacturerLen,
                  const ArrayBuffer& aManufacturerData,
                  BluetoothGattClientResultHandler* aRes);

protected:
  BluetoothGattClientHALInterface(
#if ANDROID_VERSION >= 19
    const btgatt_client_interface_t* aInterface
#endif
    );
  ~BluetoothGattClientHALInterface();

private:
#if ANDROID_VERSION >= 19
  const btgatt_client_interface_t* mInterface;
#endif
};



class BluetoothGattHALInterface MOZ_FINAL
 : public BluetoothGattInterface
{
public:
  friend class BluetoothHALInterface;

  void Init(BluetoothGattNotificationHandler* aNotificationHandler,
            BluetoothGattResultHandler* aRes);
  void Cleanup(BluetoothGattResultHandler* aRes);

  BluetoothGattClientInterface* GetBluetoothGattClientInterface();

protected:
  BluetoothGattHALInterface(
#if ANDROID_VERSION >= 19
    const btgatt_interface_t* aInterface
#endif
    );
  ~BluetoothGattHALInterface();

private:
#if ANDROID_VERSION >= 19
  const btgatt_interface_t* mInterface;
#endif
};

END_BLUETOOTH_NAMESPACE

#endif

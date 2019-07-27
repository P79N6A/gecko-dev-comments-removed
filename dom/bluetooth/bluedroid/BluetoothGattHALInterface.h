





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

class BluetoothGattClientHALInterface final
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
               BluetoothTransport aTransport,
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
                     bool aSearchAll,
                     const BluetoothUuid& aUuid,
                     BluetoothGattClientResultHandler* aRes);
  void GetIncludedService(int aConnId,
                          const BluetoothGattServiceId& aServiceId,
                          bool aFirst,
                          const BluetoothGattServiceId& aStartServiceId,
                          BluetoothGattClientResultHandler* aRes);
  void GetCharacteristic(int aConnId,
                         const BluetoothGattServiceId& aServiceId,
                         bool aFirst,
                         const BluetoothGattId& aStartCharId,
                         BluetoothGattClientResultHandler* aRes);
  void GetDescriptor(int aConnId,
                     const BluetoothGattServiceId& aServiceId,
                     const BluetoothGattId& aCharId,
                     bool aFirst,
                     const BluetoothGattId& aDescriptorId,
                     BluetoothGattClientResultHandler* aRes);

  
  void ReadCharacteristic(int aConnId,
                          const BluetoothGattServiceId& aServiceId,
                          const BluetoothGattId& aCharId,
                          BluetoothGattAuthReq aAuthReq,
                          BluetoothGattClientResultHandler* aRes);
  void WriteCharacteristic(int aConnId,
                           const BluetoothGattServiceId& aServiceId,
                           const BluetoothGattId& aCharId,
                           BluetoothGattWriteType aWriteType,
                           BluetoothGattAuthReq aAuthReq,
                           const nsTArray<uint8_t>& aValue,
                           BluetoothGattClientResultHandler* aRes);
  void ReadDescriptor(int aConnId,
                      const BluetoothGattServiceId& aServiceId,
                      const BluetoothGattId& aCharId,
                      const BluetoothGattId& aDescriptorId,
                      BluetoothGattAuthReq aAuthReq,
                      BluetoothGattClientResultHandler* aRes);
  void WriteDescriptor(int aConnId,
                       const BluetoothGattServiceId& aServiceId,
                       const BluetoothGattId& aCharId,
                       const BluetoothGattId& aDescriptorId,
                       BluetoothGattWriteType aWriteType,
                       BluetoothGattAuthReq aAuthReq,
                       const nsTArray<uint8_t>& aValue,
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
                  uint16_t aManufacturerLen, char* aManufacturerData,
                  uint16_t aServiceDataLen, char* aServiceData,
                  uint16_t aServiceUUIDLen, char* aServiceUUID,
                  BluetoothGattClientResultHandler* aRes);

  void TestCommand(int aCommand, const BluetoothGattTestParam& aTestParam,
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

class BluetoothGattServerHALInterface final
  : public BluetoothGattServerInterface
{
public:
  friend class BluetoothGattHALInterface;

  
  void RegisterServer(const BluetoothUuid& aUuid,
                      BluetoothGattServerResultHandler* aRes);
  void UnregisterServer(int aServerIf,
                        BluetoothGattServerResultHandler* aRes);

  
  void ConnectPeripheral(int aServerIf,
                         const nsAString& aBdAddr,
                         bool aIsDirect, 
                         BluetoothTransport aTransport,
                         BluetoothGattServerResultHandler* aRes);
  void DisconnectPeripheral(int aServerIf,
                            const nsAString& aBdAddr,
                            int aConnId,
                            BluetoothGattServerResultHandler* aRes);

  
  void AddService(int aServerIf,
                  const BluetoothGattServiceId& aServiceId,
                  int aNumHandles,
                  BluetoothGattServerResultHandler* aRes);
  void AddIncludedService(int aServerIf,
                          int aServiceHandle,
                          int aIncludedServiceHandle,
                          BluetoothGattServerResultHandler* aRes);
  void AddCharacteristic(int aServerIf,
                         int aServiceHandle,
                         const BluetoothUuid& aUuid,
                         BluetoothGattCharProp aProperties,
                         BluetoothGattAttrPerm aPermissions,
                         BluetoothGattServerResultHandler* aRes);
  void AddDescriptor(int aServerIf,
                     int aServiceHandle,
                     const BluetoothUuid& aUuid,
                     BluetoothGattAttrPerm aPermissions,
                     BluetoothGattServerResultHandler* aRes);

  
  void StartService(int aServerIf,
                    int aServiceHandle,
                    BluetoothTransport aTransport,
                    BluetoothGattServerResultHandler* aRes);
  void StopService(int aServerIf,
                   int aServiceHandle,
                   BluetoothGattServerResultHandler* aRes);
  void DeleteService(int aServerIf,
                     int aServiceHandle,
                     BluetoothGattServerResultHandler* aRes);

  
  void SendIndication(int aServerIf,
                      int aAttributeHandle,
                      int aConnId,
                      const nsTArray<uint8_t>& aValue,
                      bool aConfirm, 
                      BluetoothGattServerResultHandler* aRes);

  
  void SendResponse(int aConnId,
                    int aTransId,
                    BluetoothGattStatus aStatus,
                    const BluetoothGattResponse& aResponse,
                    BluetoothGattServerResultHandler* aRes);

protected:
  BluetoothGattServerHALInterface(
#if ANDROID_VERSION >= 19
    const btgatt_server_interface_t* aInterface
#endif
    );
  ~BluetoothGattServerHALInterface();

private:
#if ANDROID_VERSION >= 19
  const btgatt_server_interface_t* mInterface;
#endif
};

class BluetoothGattHALInterface final
 : public BluetoothGattInterface
{
public:
  friend class BluetoothHALInterface;

  void Init(BluetoothGattNotificationHandler* aNotificationHandler,
            BluetoothGattResultHandler* aRes);
  void Cleanup(BluetoothGattResultHandler* aRes);

  BluetoothGattClientInterface* GetBluetoothGattClientInterface();
  BluetoothGattServerInterface* GetBluetoothGattServerInterface();

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

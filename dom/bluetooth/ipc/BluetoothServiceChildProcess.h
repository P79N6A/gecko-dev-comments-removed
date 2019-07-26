





#ifndef mozilla_dom_bluetooth_ipc_bluetoothservicechildprocess_h__
#define mozilla_dom_bluetooth_ipc_bluetoothservicechildprocess_h__

#include "BluetoothService.h"

namespace mozilla {
namespace dom {
namespace bluetooth {

class BluetoothChild;

} 
} 
} 


BEGIN_BLUETOOTH_NAMESPACE

class BluetoothServiceChildProcess : public BluetoothService
{
  friend class mozilla::dom::bluetooth::BluetoothChild;

public:
  static BluetoothServiceChildProcess*
  Create();

  virtual void
  RegisterBluetoothSignalHandler(const nsAString& aNodeName,
                                 BluetoothSignalObserver* aMsgHandler)
                                 MOZ_OVERRIDE;

  virtual void
  UnregisterBluetoothSignalHandler(const nsAString& aNodeName,
                                   BluetoothSignalObserver* aMsgHandler)
                                   MOZ_OVERRIDE;

  virtual nsresult
  GetDefaultAdapterPathInternal(BluetoothReplyRunnable* aRunnable) MOZ_OVERRIDE;

  virtual nsresult
  GetPairedDevicePropertiesInternal(const nsTArray<nsString>& aDeviceAddresses,
                                    BluetoothReplyRunnable* aRunnable)
                                    MOZ_OVERRIDE;

  virtual nsresult
  StopDiscoveryInternal(const nsAString& aAdapterPath,
                        BluetoothReplyRunnable* aRunnable) MOZ_OVERRIDE;

  virtual nsresult
  StartDiscoveryInternal(const nsAString& aAdapterPath,
                         BluetoothReplyRunnable* aRunnable) MOZ_OVERRIDE;

  virtual nsresult
  GetProperties(BluetoothObjectType aType,
                const nsAString& aPath,
                BluetoothReplyRunnable* aRunnable) MOZ_OVERRIDE;

  virtual nsresult
  SetProperty(BluetoothObjectType aType,
              const nsAString& aPath,
              const BluetoothNamedValue& aValue,
              BluetoothReplyRunnable* aRunnable) MOZ_OVERRIDE;

  virtual bool
  GetDevicePath(const nsAString& aAdapterPath,
                const nsAString& aDeviceAddress,
                nsAString& aDevicePath) MOZ_OVERRIDE;

  virtual bool
  AddReservedServicesInternal(const nsAString& aAdapterPath,
                              const nsTArray<uint32_t>& aServices,
                              nsTArray<uint32_t>& aServiceHandlesContainer)
                              MOZ_OVERRIDE;

  virtual bool
  RemoveReservedServicesInternal(const nsAString& aAdapterPath,
                                 const nsTArray<uint32_t>& aServiceHandles)
                                 MOZ_OVERRIDE;

  virtual nsresult
  CreatePairedDeviceInternal(const nsAString& aAdapterPath,
                             const nsAString& aAddress,
                             int aTimeout,
                             BluetoothReplyRunnable* aRunnable) MOZ_OVERRIDE;

  virtual nsresult
  RemoveDeviceInternal(const nsAString& aAdapterPath,
                       const nsAString& aObjectPath,
                       BluetoothReplyRunnable* aRunnable) MOZ_OVERRIDE;

  virtual nsresult
  GetSocketViaService(const nsAString& aObjectPath,
                      const nsAString& aService,
                      int aType,
                      bool aAuth,
                      bool aEncrypt,
                      BluetoothReplyRunnable* aRunnable) MOZ_OVERRIDE;

  virtual bool
  CloseSocket(int aFd, BluetoothReplyRunnable* aRunnable) MOZ_OVERRIDE;

  virtual bool
  SetPinCodeInternal(const nsAString& aDeviceAddress,
                     const nsAString& aPinCode) MOZ_OVERRIDE;

  virtual bool
  SetPasskeyInternal(const nsAString& aDeviceAddress,
                     uint32_t aPasskey) MOZ_OVERRIDE;

  virtual bool
  SetPairingConfirmationInternal(const nsAString& aDeviceAddress,
                                 bool aConfirm) MOZ_OVERRIDE;

  virtual bool
  SetAuthorizationInternal(const nsAString& aDeviceAddress,
                           bool aAllow) MOZ_OVERRIDE;

protected:
  BluetoothServiceChildProcess();
  virtual ~BluetoothServiceChildProcess();

  void
  NoteDeadActor();

  void
  NoteShutdownInitiated();

  virtual nsresult
  HandleStartup() MOZ_OVERRIDE;

  virtual nsresult
  HandleShutdown() MOZ_OVERRIDE;

private:
  
  virtual nsresult
  StartInternal() MOZ_OVERRIDE;

  
  virtual nsresult
  StopInternal() MOZ_OVERRIDE;
};

END_BLUETOOTH_NAMESPACE

#endif 

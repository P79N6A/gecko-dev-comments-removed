





#ifndef mozilla_dom_bluetooth_bluetoothhidmanager_h__
#define mozilla_dom_bluetooth_bluetoothhidmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothProfileController.h"
#include "BluetoothProfileManagerBase.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothHidManager : public BluetoothProfileManagerBase
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static BluetoothHidManager* Get();
  ~BluetoothHidManager();

  
  virtual void OnGetServiceChannel(const nsAString& aDeviceAddress,
                                   const nsAString& aServiceUuid,
                                   int aChannel) MOZ_OVERRIDE;
  virtual void OnUpdateSdpRecords(const nsAString& aDeviceAddress) MOZ_OVERRIDE;
  virtual void GetAddress(nsAString& aDeviceAddress) MOZ_OVERRIDE;
  virtual bool IsConnected() MOZ_OVERRIDE;
  virtual void Connect(const nsAString& aDeviceAddress,
                       BluetoothProfileController* aController) MOZ_OVERRIDE;
  virtual void Disconnect(BluetoothProfileController* aController)
                          MOZ_OVERRIDE;
  virtual void OnConnect(const nsAString& aErrorStr) MOZ_OVERRIDE;
  virtual void OnDisconnect(const nsAString& aErrorStr) MOZ_OVERRIDE;

  virtual void GetName(nsACString& aName)
  {
    aName.AssignLiteral("HID");
  }

  
  void HandleInputPropertyChanged(const BluetoothSignal& aSignal);

private:
  BluetoothHidManager();
  bool Init();
  void Cleanup();
  void HandleShutdown();

  void NotifyStatusChanged();

  
  bool mConnected;
  nsString mDeviceAddress;
  nsRefPtr<BluetoothProfileController> mController;
};

END_BLUETOOTH_NAMESPACE

#endif 

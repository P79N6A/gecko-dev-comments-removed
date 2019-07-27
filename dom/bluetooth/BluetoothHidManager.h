





#ifndef mozilla_dom_bluetooth_bluetoothhidmanager_h__
#define mozilla_dom_bluetooth_bluetoothhidmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothProfileController.h"
#include "BluetoothProfileManagerBase.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothHidManager : public BluetoothProfileManagerBase
{
public:
  BT_DECL_PROFILE_MGR_BASE
  virtual void GetName(nsACString& aName)
  {
    aName.AssignLiteral("HID");
  }

  static BluetoothHidManager* Get();

  
  void HandleInputPropertyChanged(const BluetoothSignal& aSignal);

protected:
  virtual ~BluetoothHidManager();

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

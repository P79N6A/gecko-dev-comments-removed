





#ifndef mozilla_dom_bluetooth_bluetoothhfpmanager_h__
#define mozilla_dom_bluetooth_bluetoothhfpmanager_h__

#include "BluetoothHfpManagerBase.h"









BEGIN_BLUETOOTH_NAMESPACE

class BluetoothHfpManager : public BluetoothHfpManagerBase
{
public:
  BT_DECL_HFP_MGR_BASE
  virtual void GetName(nsACString& aName)
  {
    aName.AssignLiteral("Fallback HFP/HSP");
  }

  static BluetoothHfpManager* Get();
  virtual ~BluetoothHfpManager() { }

  bool ConnectSco();
  bool DisconnectSco();

private:
  BluetoothHfpManager() { }
  bool Init();
  void HandleShutdown();
};

END_BLUETOOTH_NAMESPACE

#endif

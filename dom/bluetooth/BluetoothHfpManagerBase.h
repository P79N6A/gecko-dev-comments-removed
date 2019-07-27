





#ifndef mozilla_dom_bluetooth_bluetoothhfpmanagerbase_h__
#define mozilla_dom_bluetooth_bluetoothhfpmanagerbase_h__

#include "BluetoothProfileManagerBase.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothHfpManagerBase : public BluetoothProfileManagerBase
{
public:
  


  virtual bool IsScoConnected() = 0;
};

#define BT_DECL_HFP_MGR_BASE                  \
  BT_DECL_PROFILE_MGR_BASE                    \
  virtual bool IsScoConnected() override;

END_BLUETOOTH_NAMESPACE

#endif  

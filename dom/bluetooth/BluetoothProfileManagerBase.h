





#ifndef mozilla_dom_bluetooth_bluetoothprofilemanagerbase_h__
#define mozilla_dom_bluetooth_bluetoothprofilemanagerbase_h__






#define ERR_ALREADY_CONNECTED           "AlreadyConnectedError"
#define ERR_NO_AVAILABLE_RESOURCE       "NoAvailableResourceError"
#define ERR_REACHED_CONNECTION_LIMIT    "ReachedConnectionLimitError"
#define ERR_SERVICE_CHANNEL_NOT_FOUND   "DeviceChannelRetrievalError"

#include "BluetoothCommon.h"
#include "nsIObserver.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothProfileManagerBase : public nsIObserver
{
public:
  virtual void OnGetServiceChannel(const nsAString& aDeviceAddress,
                                   const nsAString& aServiceUuid,
                                   int aChannel) = 0;
  virtual void OnUpdateSdpRecords(const nsAString& aDeviceAddress) = 0;
  virtual void GetAddress(nsAString& aDeviceAddress) = 0;
};

END_BLUETOOTH_NAMESPACE

#endif  

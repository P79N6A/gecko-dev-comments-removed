





#ifndef mozilla_dom_bluetooth_bluetoothprofilemanagerbase_h__
#define mozilla_dom_bluetooth_bluetoothprofilemanagerbase_h__






#define ERR_ALREADY_CONNECTED           "AlreadyConnectedError"
#define ERR_ALREADY_DISCONNECTED        "AlreadyDisconnectedError"
#define ERR_CONNECTION_FAILED           "ConnectionFailedError"
#define ERR_DISCONNECTION_FAILED        "DisconnectionFailedError"
#define ERR_NO_AVAILABLE_RESOURCE       "NoAvailableResourceError"
#define ERR_REACHED_CONNECTION_LIMIT    "ReachedConnectionLimitError"
#define ERR_SERVICE_CHANNEL_NOT_FOUND   "DeviceChannelRetrievalError"
#define ERR_UNKNOWN_PROFILE             "UnknownProfileError"
#define ERR_OPERATION_TIMEOUT           "OperationTimeout"

#include "BluetoothCommon.h"
#include "nsIObserver.h"

BEGIN_BLUETOOTH_NAMESPACE
class BluetoothProfileController;

class BluetoothProfileResultHandler
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(BluetoothProfileResultHandler);

  virtual ~BluetoothProfileResultHandler() { }

  virtual void OnError(nsresult aResult) { }
  virtual void Init() { }
  virtual void Deinit() { }
};

class BluetoothProfileManagerBase : public nsIObserver
{
public:
  virtual void OnGetServiceChannel(const nsAString& aDeviceAddress,
                                   const nsAString& aServiceUuid,
                                   int aChannel) = 0;
  virtual void OnUpdateSdpRecords(const nsAString& aDeviceAddress) = 0;

  


  virtual void GetAddress(nsAString& aDeviceAddress) = 0;

  


  virtual bool IsConnected() = 0;

  



  virtual void Connect(const nsAString& aDeviceAddress,
                       BluetoothProfileController* aController) = 0;

  


  virtual void Disconnect(BluetoothProfileController* aController) = 0;

  



  virtual void OnConnect(const nsAString& aErrorStr) = 0;
  virtual void OnDisconnect(const nsAString& aErrorStr) = 0;

  


  virtual void Reset() = 0;

  


  virtual void GetName(nsACString& aName) = 0;
};

#define BT_DECL_PROFILE_MGR_BASE                                             \
public:                                                                      \
  NS_DECL_ISUPPORTS                                                          \
  NS_DECL_NSIOBSERVER                                                        \
  virtual void OnGetServiceChannel(const nsAString& aDeviceAddress,          \
                                   const nsAString& aServiceUuid,            \
                                   int aChannel) override;                   \
  virtual void OnUpdateSdpRecords(const nsAString& aDeviceAddress) override; \
  virtual void GetAddress(nsAString& aDeviceAddress) override;               \
  virtual bool IsConnected() override;                                       \
  virtual void Connect(const nsAString& aDeviceAddress,                      \
                       BluetoothProfileController* aController) override;    \
  virtual void Disconnect(BluetoothProfileController* aController) override; \
  virtual void OnConnect(const nsAString& aErrorStr) override;               \
  virtual void OnDisconnect(const nsAString& AErrorStr) override;            \
  virtual void Reset() override;

END_BLUETOOTH_NAMESPACE

#endif  

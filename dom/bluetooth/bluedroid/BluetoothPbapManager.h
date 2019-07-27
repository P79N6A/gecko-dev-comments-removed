





#ifndef mozilla_dom_bluetooth_bluetoothpbapmanager_h__
#define mozilla_dom_bluetooth_bluetoothpbapmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothProfileManagerBase.h"
#include "BluetoothSocketObserver.h"
#include "mozilla/ipc/SocketBase.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSocket;
class ObexHeaderSet;

class BluetoothPbapManager : public BluetoothSocketObserver
                          , public BluetoothProfileManagerBase
{
public:
  BT_DECL_PROFILE_MGR_BASE
  BT_DECL_SOCKET_OBSERVER
  virtual void GetName(nsACString& aName)
  {
    aName.AssignLiteral("PBAP");
  }

  static const int MAX_PACKET_LENGTH = 0xFFFE;

  static BluetoothPbapManager* Get();
  bool Listen();

protected:
  virtual ~BluetoothPbapManager();

private:
  BluetoothPbapManager();
  bool Init();
  void HandleShutdown();

  void ReplyToConnect();
  void ReplyToDisconnectOrAbort();
  void ReplyError(uint8_t aError);
  void SendObexData(uint8_t* aData, uint8_t aOpcode, int aSize);

  bool CompareHeaderTarget(const ObexHeaderSet& aHeader);
  void AfterPbapConnected();
  void AfterPbapDisconnected();

  


  bool mConnected;
  nsString mDeviceAddress;

  
  
  
  nsRefPtr<BluetoothSocket> mSocket;

  
  
  
  nsRefPtr<BluetoothSocket> mServerSocket;
};

END_BLUETOOTH_NAMESPACE

#endif

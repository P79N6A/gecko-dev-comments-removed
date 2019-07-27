





#ifndef mozilla_dom_bluetooth_bluetoothpbapmanager_h__
#define mozilla_dom_bluetooth_bluetoothpbapmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothProfileManagerBase.h"
#include "BluetoothSocketObserver.h"
#include "mozilla/ipc/SocketBase.h"

BEGIN_BLUETOOTH_NAMESPACE




enum AppParameterTag {
  Order                   = 0x01,
  SearchValue             = 0x02,
  SearchProperty          = 0x03,
  MaxListCount            = 0x04,
  ListStartOffset         = 0x05,
  PropertySelector        = 0x06,
  Format                  = 0x07,
  PhonebookSize           = 0x08,
  NewMissedCalls          = 0x09,
  PrimaryVersionCounter   = 0x0A,
  SecondaryVersionCounter = 0x0B,
  vCardSelector           = 0x0C,
  DatabaseIdentifier      = 0x0D,
  vCardSelectorOperator   = 0x0E,
  ResetNewMissedCalls     = 0x0F,
  PbapSupportedFeatures   = 0x10
};

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
  void ReplyToSetPath();
  void ReplyError(uint8_t aError);
  void SendObexData(uint8_t* aData, uint8_t aOpcode, int aSize);

  uint8_t SetPhoneBookPath(uint8_t flags, const ObexHeaderSet& aHeader);
  bool CompareHeaderTarget(const ObexHeaderSet& aHeader);
  bool IsLegalPath(const nsAString& aPath);
  void AfterPbapConnected();
  void AfterPbapDisconnected();

  


  nsString mCurrentPath;

  


  bool mConnected;
  nsString mDeviceAddress;

  
  
  
  nsRefPtr<BluetoothSocket> mSocket;

  
  
  
  nsRefPtr<BluetoothSocket> mServerSocket;
};

END_BLUETOOTH_NAMESPACE

#endif

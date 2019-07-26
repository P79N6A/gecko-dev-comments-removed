





#ifndef mozilla_dom_bluetooth_bluetoothhidmanager_h__
#define mozilla_dom_bluetooth_bluetoothhidmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothProfileManagerBase.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;

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

  bool Connect(const nsAString& aDeviceAddress,
               BluetoothReplyRunnable* aRunnable);
  void Disconnect();
  void HandleInputPropertyChanged(const BluetoothSignal& aSignal);

private:
  BluetoothHidManager();
  bool Init();
  void Cleanup();
  void HandleShutdown();

  void NotifyStatusChanged();

  
  bool mConnected;
  nsString mDeviceAddress;
};

END_BLUETOOTH_NAMESPACE

#endif 

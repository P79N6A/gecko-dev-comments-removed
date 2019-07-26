





#ifndef mozilla_dom_bluetooth_bluetootha2dpmanager_h__
#define mozilla_dom_bluetooth_bluetootha2dpmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothProfileManagerBase.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothA2dpManagerObserver;

class BluetoothA2dpManager : public BluetoothProfileManagerBase
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  enum SinkState {
    SINK_DISCONNECTED = 1,
    SINK_CONNECTING,
    SINK_CONNECTED,
    SINK_PLAYING,
    SINK_DISCONNECTING
  };

  static BluetoothA2dpManager* Get();
  ~BluetoothA2dpManager();
  void ResetA2dp();
  void ResetAvrcp();

  
  virtual void OnGetServiceChannel(const nsAString& aDeviceAddress,
                                   const nsAString& aServiceUuid,
                                   int aChannel) MOZ_OVERRIDE;
  virtual void OnUpdateSdpRecords(const nsAString& aDeviceAddress) MOZ_OVERRIDE;
  virtual void GetAddress(nsAString& aDeviceAddress) MOZ_OVERRIDE;
  virtual bool IsConnected() MOZ_OVERRIDE;

  
  bool Connect(const nsAString& aDeviceAddress);
  void Disconnect();
  void HandleSinkPropertyChanged(const BluetoothSignal& aSignal);

  
  void SetAvrcpConnected(bool aConnected);
  bool IsAvrcpConnected();

private:
  BluetoothA2dpManager();
  bool Init();

  void HandleSinkStateChanged(SinkState aState);
  void HandleShutdown();

  void NotifyStatusChanged();
  void NotifyAudioManager();

  nsString mDeviceAddress;

  
  bool mA2dpConnected;
  bool mPlaying;
  SinkState mSinkState;

  
  bool mAvrcpConnected;
};

END_BLUETOOTH_NAMESPACE

#endif

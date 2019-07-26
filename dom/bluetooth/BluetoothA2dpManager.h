





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
  void UpdateMetaData(const nsAString& aTitle,
                      const nsAString& aArtist,
                      const nsAString& aAlbum,
                      uint32_t aMediaNumber,
                      uint32_t aTotalMediaCount,
                      uint32_t aDuration);
  void UpdatePlayStatus(uint32_t aDuration,
                        uint32_t aPosition,
                        ControlPlayStatus aPlayStatus);
  void GetAlbum(nsAString& aAlbum);
  uint32_t GetDuration();
  ControlPlayStatus GetPlayStatus();
  uint32_t GetPosition();
  uint32_t GetMediaNumber();
  void GetTitle(nsAString& aTitle);

private:
  BluetoothA2dpManager();
  bool Init();

  void HandleSinkStateChanged(SinkState aState);
  void HandleShutdown();

  void NotifyStatusChanged();

  
  bool mA2dpConnected;
  bool mPlaying;
  nsString mDeviceAddress;
  SinkState mSinkState;

  
  bool mAvrcpConnected;
  nsString mAlbum;
  nsString mArtist;
  nsString mTitle;
  uint32_t mDuration;
  uint32_t mMediaNumber;
  uint32_t mTotalMediaCount;
  uint32_t mPosition;
  ControlPlayStatus mPlayStatus;
};

END_BLUETOOTH_NAMESPACE

#endif

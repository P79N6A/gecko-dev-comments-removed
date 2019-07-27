





#ifndef mozilla_dom_bluetooth_bluetootha2dpmanager_h__
#define mozilla_dom_bluetooth_bluetootha2dpmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothInterface.h"
#include "BluetoothProfileController.h"
#include "BluetoothProfileManagerBase.h"

BEGIN_BLUETOOTH_NAMESPACE
class BluetoothA2dpManager : public BluetoothProfileManagerBase
                           , public BluetoothA2dpNotificationHandler
                           , public BluetoothAvrcpNotificationHandler
{
public:
  BT_DECL_PROFILE_MGR_BASE
  virtual void GetName(nsACString& aName)
  {
    aName.AssignLiteral("A2DP");
  }

  enum SinkState {
    SINK_UNKNOWN,
    SINK_DISCONNECTED,
    SINK_CONNECTING,
    SINK_CONNECTED,
    SINK_PLAYING,
  };

  static BluetoothA2dpManager* Get();
  static void InitA2dpInterface(BluetoothProfileResultHandler* aRes);
  static void DeinitA2dpInterface(BluetoothProfileResultHandler* aRes);

  void OnConnectError();
  void OnDisconnectError();

  
  void HandleSinkPropertyChanged(const BluetoothSignal& aSignal);

  
  void SetAvrcpConnected(bool aConnected);
  bool IsAvrcpConnected();
  void UpdateMetaData(const nsAString& aTitle,
                      const nsAString& aArtist,
                      const nsAString& aAlbum,
                      uint64_t aMediaNumber,
                      uint64_t aTotalMediaCount,
                      uint32_t aDuration);
  void UpdatePlayStatus(uint32_t aDuration,
                        uint32_t aPosition,
                        ControlPlayStatus aPlayStatus);
  void UpdateRegisterNotification(BluetoothAvrcpEvent aEvent, uint32_t aParam);
  void GetAlbum(nsAString& aAlbum);
  uint32_t GetDuration();
  ControlPlayStatus GetPlayStatus();
  uint32_t GetPosition();
  uint64_t GetMediaNumber();
  uint64_t GetTotalMediaNumber();
  void GetTitle(nsAString& aTitle);
  void GetArtist(nsAString& aArtist);

protected:
  virtual ~BluetoothA2dpManager();

private:
  class CleanupA2dpResultHandler;
  class CleanupA2dpResultHandlerRunnable;
  class CleanupAvrcpResultHandler;
  class ConnectResultHandler;
  class DisconnectResultHandler;
  class InitA2dpResultHandler;
  class InitAvrcpResultHandler;
  class OnErrorProfileResultHandlerRunnable;

  BluetoothA2dpManager();
  void ResetA2dp();
  void ResetAvrcp();

  void HandleShutdown();
  void NotifyConnectionStatusChanged();

  void ConnectionStateNotification(BluetoothA2dpConnectionState aState,
                                   const nsAString& aBdAddr) override;
  void AudioStateNotification(BluetoothA2dpAudioState aState,
                              const nsAString& aBdAddr) override;

  void GetPlayStatusNotification() override;

  void ListPlayerAppAttrNotification() override;

  void ListPlayerAppValuesNotification(
    BluetoothAvrcpPlayerAttribute aAttrId) override;

  void GetPlayerAppValueNotification(
    uint8_t aNumAttrs,
    const BluetoothAvrcpPlayerAttribute* aAttrs) override;

  void GetPlayerAppAttrsTextNotification(
    uint8_t aNumAttrs,
    const BluetoothAvrcpPlayerAttribute* aAttrs) override;

  void GetPlayerAppValuesTextNotification(
    uint8_t aAttrId, uint8_t aNumVals, const uint8_t* aValues) override;

  void SetPlayerAppValueNotification(
    const BluetoothAvrcpPlayerSettings& aSettings) override;

  void GetElementAttrNotification(
    uint8_t aNumAttrs,
    const BluetoothAvrcpMediaAttribute* aAttrs) override;

  void RegisterNotificationNotification(
    BluetoothAvrcpEvent aEvent, uint32_t aParam) override;

  void RemoteFeatureNotification(
    const nsAString& aBdAddr, unsigned long aFeatures) override;

  void VolumeChangeNotification(uint8_t aVolume, uint8_t aCType) override;

  void PassthroughCmdNotification(int aId, int aKeyState) override;

  nsString mDeviceAddress;
  nsRefPtr<BluetoothProfileController> mController;

  
  bool mA2dpConnected;
  SinkState mSinkState;

  
  bool mAvrcpConnected;
  nsString mAlbum;
  nsString mArtist;
  nsString mTitle;
  uint32_t mDuration;
  uint64_t mMediaNumber;
  uint64_t mTotalMediaCount;
  uint32_t mPosition;
  





  uint32_t mPlaybackInterval;
  ControlPlayStatus mPlayStatus;
  









  BluetoothAvrcpNotification mPlayStatusChangedNotifyType;
  BluetoothAvrcpNotification mTrackChangedNotifyType;
  BluetoothAvrcpNotification mPlayPosChangedNotifyType;
  BluetoothAvrcpNotification mAppSettingsChangedNotifyType;
};

END_BLUETOOTH_NAMESPACE

#endif







#ifndef mozilla_dom_bluetooth_bluetootha2dpmanager_h__
#define mozilla_dom_bluetooth_bluetootha2dpmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothProfileController.h"
#include "BluetoothProfileManagerBase.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothA2dpManager : public BluetoothProfileManagerBase
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
  void GetAlbum(nsAString& aAlbum);
  uint32_t GetDuration();
  ControlPlayStatus GetPlayStatus();
  uint32_t GetPosition();
  uint64_t GetMediaNumber();
  void GetTitle(nsAString& aTitle);

protected:
  virtual ~BluetoothA2dpManager();

private:
  BluetoothA2dpManager();
  bool Init();
  void ResetA2dp();
  void ResetAvrcp();

  void HandleShutdown();
  void NotifyConnectionStatusChanged();

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
  ControlPlayStatus mPlayStatus;
};

END_BLUETOOTH_NAMESPACE

#endif

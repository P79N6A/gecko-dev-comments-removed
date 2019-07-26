





#ifndef mozilla_dom_bluetooth_bluetootha2dpmanager_h__
#define mozilla_dom_bluetooth_bluetootha2dpmanager_h__

#include "BluetoothCommon.h"

BEGIN_BLUETOOTH_NAMESPACE

enum SinkState {
  SINK_DISCONNECTED = 1,
  SINK_CONNECTING,
  SINK_CONNECTED,
  SINK_PLAYING,
  SINK_DISCONNECTING
};

class BluetoothA2dpManagerObserver;
class BluetoothValue;
class BluetoothSocket;

class BluetoothA2dpManager
{
public:
  static BluetoothA2dpManager* Get();
  ~BluetoothA2dpManager();

  bool Connect(const nsAString& aDeviceAddress);
  void Disconnect();
  void HandleSinkPropertyChanged(const BluetoothSignal& aSignal);
  nsresult HandleShutdown();

private:
  BluetoothA2dpManager();
  bool Init();
  void Cleanup();

  void HandleSinkStateChanged(SinkState aState);

  bool mConnected;
  bool mPlaying;
  nsString mDeviceAddress;
  SinkState mSinkState;
};

END_BLUETOOTH_NAMESPACE

#endif

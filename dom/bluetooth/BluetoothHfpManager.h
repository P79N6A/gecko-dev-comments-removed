





#ifndef mozilla_dom_bluetooth_bluetoothhfpmanager_h__
#define mozilla_dom_bluetooth_bluetoothhfpmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothRilListener.h"
#include "mozilla/ipc/UnixSocket.h"
#include "nsIObserver.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;
class BluetoothHfpManagerObserver;

class BluetoothHfpManager : public mozilla::ipc::UnixSocketConsumer
{
public:
  ~BluetoothHfpManager();
  static BluetoothHfpManager* Get();
  void ReceiveSocketData(mozilla::ipc::UnixSocketRawData* aMessage);
  bool Connect(const nsAString& aDeviceObjectPath,
               const bool aIsHandsfree,
               BluetoothReplyRunnable* aRunnable);
  void Disconnect();
  bool SendLine(const char* aMessage);
  void CallStateChanged(int aCallIndex, int aCallState,
                        const char* aNumber, bool aIsActive);
  bool Listen();

private:
  friend class BluetoothHfpManagerObserver;
  BluetoothHfpManager();
  nsresult HandleVolumeChanged(const nsAString& aData);
  nsresult HandleShutdown();
  bool Init();
  void Cleanup();
  void NotifyDialer(const nsAString& aCommand);
  void NotifySettings(const bool aConnected);

  int mCurrentVgs;
  int mCurrentCallIndex;
  int mCurrentCallState;
  nsAutoPtr<BluetoothRilListener> mListener;
  nsString mDevicePath;
};

END_BLUETOOTH_NAMESPACE

#endif

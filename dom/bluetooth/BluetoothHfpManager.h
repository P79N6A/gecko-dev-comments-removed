





#ifndef mozilla_dom_bluetooth_bluetoothhfpmanager_h__
#define mozilla_dom_bluetooth_bluetoothhfpmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothSocketObserver.h"
#include "BluetoothTelephonyListener.h"
#include "mozilla/ipc/UnixSocket.h"
#include "nsIObserver.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothHfpManagerObserver;
class BluetoothReplyRunnable;
class BluetoothSocket;
class Call;





enum BluetoothCmeError {
  AG_FAILURE = 0,
  NO_CONNECTION_TO_PHONE = 1,
  OPERATION_NOT_ALLOWED = 3,
  OPERATION_NOT_SUPPORTED = 4,
  PIN_REQUIRED = 5,
  SIM_NOT_INSERTED = 10,
  SIM_PIN_REQUIRED = 11,
  SIM_PUK_REQUIRED = 12,
  SIM_FAILURE = 13,
  SIM_BUSY = 14,
  INCORRECT_PASSWORD = 16,
  SIM_PIN2_REQUIRED = 17,
  SIM_PUK2_REQUIRED = 18,
  MEMORY_FULL = 20,
  INVALID_INDEX = 21,
  MEMORY_FAILURE = 23,
  TEXT_STRING_TOO_LONG = 24,
  INVALID_CHARACTERS_IN_TEXT_STRING = 25,
  DIAL_STRING_TOO_LONG = 26,
  INVALID_CHARACTERS_IN_DIAL_STRING = 27,
  NO_NETWORK_SERVICE = 30,
  NETWORK_TIMEOUT = 31,
  NETWORK_NOT_ALLOWED = 32
};

class BluetoothHfpManager : public BluetoothSocketObserver
{
public:
  static BluetoothHfpManager* Get();
  ~BluetoothHfpManager();

  virtual void ReceiveSocketData(
    BluetoothSocket* aSocket,
    nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage) MOZ_OVERRIDE;
  virtual void OnConnectSuccess(BluetoothSocket* aSocket) MOZ_OVERRIDE;
  virtual void OnConnectError(BluetoothSocket* aSocket) MOZ_OVERRIDE;
  virtual void OnDisconnect(BluetoothSocket* aSocket) MOZ_OVERRIDE;

  bool Connect(const nsAString& aDeviceObjectPath,
               const bool aIsHandsfree,
               BluetoothReplyRunnable* aRunnable);
  void Disconnect();
  bool Listen();

  


  void HandleCallStateChanged(uint32_t aCallIndex, uint16_t aCallState,
                              const nsAString& aNumber, bool aSend);
  bool IsConnected();

private:
  class GetVolumeTask;
  class SendRingIndicatorTask;

  friend class GetVolumeTask;
  friend class SendRingIndicatorTask;
  friend class BluetoothHfpManagerObserver;

  BluetoothHfpManager();
  nsresult HandleIccInfoChanged();
  nsresult HandleShutdown();
  nsresult HandleVolumeChanged(const nsAString& aData);
  nsresult HandleVoiceConnectionChanged();

  bool Init();
  void Cleanup();
  void Reset();
  void ResetCallArray();

  void NotifyDialer(const nsAString& aCommand);
  void NotifySettings();

  bool SendCommand(const char* aCommand, uint8_t aValue = 0);
  bool SendLine(const char* aMessage);
  void UpdateCIND(uint8_t aType, uint8_t aValue, bool aSend);

  int mCurrentVgs;
  int mCurrentVgm;
  uint32_t mCurrentCallIndex;
  bool mCCWA;
  bool mCLIP;
  bool mCMEE;
  bool mCMER;
  int mNetworkSelectionMode;
  bool mReceiveVgsFlag;
  nsString mDevicePath;
  nsString mMsisdn;
  nsString mOperatorName;
  SocketConnectionStatus mPrevSocketStatus;

  nsTArray<Call> mCurrentCallArray;
  nsAutoPtr<BluetoothTelephonyListener> mListener;
  nsRefPtr<BluetoothReplyRunnable> mRunnable;
  nsRefPtr<BluetoothSocket> mSocket;
};

END_BLUETOOTH_NAMESPACE

#endif







#ifndef mozilla_dom_bluetooth_bluetoothhfpmanager_h__
#define mozilla_dom_bluetooth_bluetoothhfpmanager_h__

#include "BluetoothCommon.h"
#include "BluetoothHfpManagerBase.h"
#ifdef MOZ_B2G_RIL
#include "BluetoothRilListener.h"
#endif
#include "BluetoothSocketObserver.h"
#include "mozilla/ipc/SocketBase.h"
#include "mozilla/Hal.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;
class BluetoothSocket;

#ifdef MOZ_B2G_RIL
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

enum PhoneType {
  NONE, 
  GSM,
  CDMA
};

class Call {
public:
  Call();
  void Reset();
  bool IsActive();

  uint16_t mState;
  bool mDirection; 
  bool mIsConference;
  nsString mNumber;
  int mType;
};
#endif 

class BluetoothHfpManager : public BluetoothSocketObserver
                          , public BluetoothHfpManagerBase
                          , public BatteryObserver
{
public:
  BT_DECL_HFP_MGR_BASE
  virtual void GetName(nsACString& aName)
  {
    aName.AssignLiteral("HFP/HSP");
  }

  static BluetoothHfpManager* Get();

  
  virtual void ReceiveSocketData(
    BluetoothSocket* aSocket,
    nsAutoPtr<mozilla::ipc::UnixSocketBuffer>& aMessage) override;
  virtual void OnSocketConnectSuccess(BluetoothSocket* aSocket) override;
  virtual void OnSocketConnectError(BluetoothSocket* aSocket) override;
  virtual void OnSocketDisconnect(BluetoothSocket* aSocket) override;

  bool Listen();
  











  bool ConnectSco(BluetoothReplyRunnable* aRunnable = nullptr);
  bool DisconnectSco();
  bool ListenSco();

#ifdef MOZ_B2G_RIL
  


  void HandleCallStateChanged(uint32_t aCallIndex, uint16_t aCallState,
                              const nsAString& aError, const nsAString& aNumber,
                              const bool aIsOutgoing, const bool aIsConference,
                              bool aSend);
  void HandleIccInfoChanged(uint32_t aClientId);
  void HandleVoiceConnectionChanged(uint32_t aClientId);

  
  void UpdateSecondNumber(const nsAString& aNumber);
  void AnswerWaitingCall();
  void IgnoreWaitingCall();
  void ToggleCalls();
#endif

protected:
  ~BluetoothHfpManager();

private:
  void ParseAtCommand(const nsACString& aAtCommand, const int aStart,
                      nsTArray<nsCString>& aRetValues);

  class CloseScoTask;
  class GetVolumeTask;
#ifdef MOZ_B2G_RIL
  class RespondToBLDNTask;
  class SendRingIndicatorTask;
#endif

  friend class CloseScoTask;
  friend class GetVolumeTask;
#ifdef MOZ_B2G_RIL
  friend class RespondToBLDNTask;
  friend class SendRingIndicatorTask;
#endif
  friend class BluetoothHfpManagerObserver;

  BluetoothHfpManager();
  void HandleShutdown();
  void HandleVolumeChanged(nsISupports* aSubject);

  bool Init();
  void Notify(const hal::BatteryInformation& aBatteryInfo);
#ifdef MOZ_B2G_RIL
  void ResetCallArray();
  uint32_t FindFirstCall(uint16_t aState);
  uint32_t GetNumberOfCalls(uint16_t aState);
  uint32_t GetNumberOfConCalls();
  uint32_t GetNumberOfConCalls(uint16_t aState);
  PhoneType GetPhoneType(const nsAString& aType);
#endif

  void NotifyConnectionStatusChanged(const nsAString& aType);
  void NotifyDialer(const nsAString& aCommand);

#ifdef MOZ_B2G_RIL
  void SendCCWA(const nsAString& aNumber, int aType);
  bool SendCLCC(const Call& aCall, int aIndex);
#endif
  bool SendCommand(const char* aCommand, uint32_t aValue = 0);
  bool SendLine(const char* aMessage);
#ifdef MOZ_B2G_RIL
  void UpdateCIND(uint8_t aType, uint8_t aValue, bool aSend = true);
#endif
  void OnScoConnectSuccess();
  void OnScoConnectError();
  void OnScoDisconnect();

  int mCurrentVgs;
  int mCurrentVgm;
#ifdef MOZ_B2G_RIL
  bool mBSIR;
  bool mCCWA;
  bool mCLIP;
#endif
  bool mCMEE;
  bool mCMER;
  bool mConnectScoRequest;
  bool mSlcConnected;
  bool mIsHsp;
#ifdef MOZ_B2G_RIL
  bool mFirstCKPD;
  int mNetworkSelectionMode;
  PhoneType mPhoneType;
#endif
  bool mReceiveVgsFlag;
#ifdef MOZ_B2G_RIL
  bool mDialingRequestProcessed;
#endif
  nsString mDeviceAddress;
#ifdef MOZ_B2G_RIL
  nsString mMsisdn;
  nsString mOperatorName;

  nsTArray<Call> mCurrentCallArray;
  nsAutoPtr<BluetoothRilListener> mListener;
#endif
  nsRefPtr<BluetoothProfileController> mController;
  nsRefPtr<BluetoothReplyRunnable> mScoRunnable;

  
  
  
  
  nsRefPtr<BluetoothSocket> mSocket;

  
  
  
  nsRefPtr<BluetoothSocket> mHandsfreeSocket;
  nsRefPtr<BluetoothSocket> mHeadsetSocket;
  nsRefPtr<BluetoothSocket> mScoSocket;
  mozilla::ipc::SocketConnectionStatus mScoSocketStatus;

#ifdef MOZ_B2G_RIL
  
  Call mCdmaSecondCall;
#endif
};

END_BLUETOOTH_NAMESPACE

#endif

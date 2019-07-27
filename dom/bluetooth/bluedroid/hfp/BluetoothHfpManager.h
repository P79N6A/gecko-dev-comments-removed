





#ifndef mozilla_dom_bluetooth_bluetoothhfpmanager_h__
#define mozilla_dom_bluetooth_bluetoothhfpmanager_h__

#include "BluetoothInterface.h"
#include "BluetoothCommon.h"
#include "BluetoothHfpManagerBase.h"
#include "BluetoothRilListener.h"
#include "BluetoothSocketObserver.h"
#include "mozilla/ipc/UnixSocket.h"
#include "mozilla/Hal.h"

BEGIN_BLUETOOTH_NAMESPACE

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

enum PhoneType {
  NONE, 
  GSM,
  CDMA
};

class Call {
public:
  Call();
  void Set(const nsAString& aNumber, const bool aIsOutgoing);
  void Reset();
  bool IsActive();

  uint16_t mState;
  nsString mNumber;
  BluetoothHandsfreeCallDirection mDirection;
  BluetoothHandsfreeCallAddressType mType;
};

class BluetoothHfpManager : public BluetoothHfpManagerBase
                          , public BluetoothHandsfreeNotificationHandler
                          , public BatteryObserver
{
public:
  BT_DECL_HFP_MGR_BASE

  static const int MAX_NUM_CLIENTS;

  void OnConnectError();
  void OnDisconnectError();

  virtual void GetName(nsACString& aName)
  {
    aName.AssignLiteral("HFP/HSP");
  }

  static BluetoothHfpManager* Get();
  static void InitHfpInterface(BluetoothProfileResultHandler* aRes);
  static void DeinitHfpInterface(BluetoothProfileResultHandler* aRes);

  bool ConnectSco();
  bool DisconnectSco();

  


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

  
  
  

  void ConnectionStateNotification(BluetoothHandsfreeConnectionState aState,
                                   const nsAString& aBdAddress) MOZ_OVERRIDE;
  void AudioStateNotification(BluetoothHandsfreeAudioState aState,
                              const nsAString& aBdAddress) MOZ_OVERRIDE;
  void AnswerCallNotification(const nsAString& aBdAddress) MOZ_OVERRIDE;
  void HangupCallNotification(const nsAString& aBdAddress) MOZ_OVERRIDE;
  void VolumeNotification(BluetoothHandsfreeVolumeType aType,
                          int aVolume,
                          const nsAString& aBdAddress) MOZ_OVERRIDE;
  void DtmfNotification(char aDtmf,
                        const nsAString& aBdAddress) MOZ_OVERRIDE;
  void CallHoldNotification(BluetoothHandsfreeCallHoldType aChld,
                            const nsAString& aBdAddress) MOZ_OVERRIDE;
  void DialCallNotification(const nsAString& aNumber,
                            const nsAString& aBdAddress) MOZ_OVERRIDE;
  void CnumNotification(const nsAString& aBdAddress) MOZ_OVERRIDE;
  void CindNotification(const nsAString& aBdAddress) MOZ_OVERRIDE;
  void CopsNotification(const nsAString& aBdAddress) MOZ_OVERRIDE;
  void ClccNotification(const nsAString& aBdAddress) MOZ_OVERRIDE;
  void UnknownAtNotification(const nsACString& aAtString,
                             const nsAString& aBdAddress) MOZ_OVERRIDE;
  void KeyPressedNotification(const nsAString& aBdAddress) MOZ_OVERRIDE;

protected:
  virtual ~BluetoothHfpManager();

private:
  class GetVolumeTask;
  class CloseScoTask;
  class CloseScoRunnable;
  class RespondToBLDNTask;
  class MainThreadTask;

  friend class BluetoothHfpManagerObserver;
  friend class GetVolumeTask;
  friend class CloseScoTask;
  friend class RespondToBLDNTask;
  friend class MainThreadTask;

  BluetoothHfpManager();
  bool Init();
  void Cleanup();

  void HandleShutdown();
  void HandleVolumeChanged(nsISupports* aSubject);
  void Notify(const hal::BatteryInformation& aBatteryInfo);

  void NotifyConnectionStateChanged(const nsAString& aType);
  void NotifyDialer(const nsAString& aCommand);

  PhoneType GetPhoneType(const nsAString& aType);
  void ResetCallArray();
  uint32_t FindFirstCall(uint16_t aState);
  uint32_t GetNumberOfCalls(uint16_t aState);
  uint16_t GetCallSetupState();
  bool IsTransitionState(uint16_t aCallState, bool aIsConference);
  BluetoothHandsfreeCallState
    ConvertToBluetoothHandsfreeCallState(int aCallState) const;

  void UpdatePhoneCIND(uint32_t aCallIndex);
  void UpdateDeviceCIND();
  void SendCLCC(Call& aCall, int aIndex);
  void SendLine(const char* aMessage);
  void SendResponse(BluetoothHandsfreeAtResponse aResponseCode);

  BluetoothHandsfreeConnectionState mConnectionState;
  BluetoothHandsfreeConnectionState mPrevConnectionState;
  BluetoothHandsfreeAudioState mAudioState;
  
  int mBattChg;
  BluetoothHandsfreeNetworkState mService;
  BluetoothHandsfreeServiceType mRoam;
  int mSignal;

  int mCurrentVgs;
  int mCurrentVgm;
  bool mReceiveVgsFlag;
  bool mDialingRequestProcessed;
  PhoneType mPhoneType;
  nsString mDeviceAddress;
  nsString mMsisdn;
  nsString mOperatorName;

  nsTArray<Call> mCurrentCallArray;
  nsAutoPtr<BluetoothRilListener> mListener;
  nsRefPtr<BluetoothProfileController> mController;

  
  Call mCdmaSecondCall;
};

END_BLUETOOTH_NAMESPACE

#endif

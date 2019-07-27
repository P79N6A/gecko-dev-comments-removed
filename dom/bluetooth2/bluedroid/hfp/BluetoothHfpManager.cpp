





#include "base/basictypes.h"

#include "BluetoothHfpManager.h"
#include "BluetoothProfileController.h"
#include "BluetoothUtils.h"

#include "jsapi.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "nsContentUtils.h"
#include "nsIAudioManager.h"
#include "nsIDOMIccInfo.h"
#include "nsIIccProvider.h"
#include "nsIMobileConnectionInfo.h"
#include "nsIMobileConnectionProvider.h"
#include "nsIMobileNetworkInfo.h"
#include "nsIObserverService.h"
#include "nsISettingsService.h"
#include "nsITelephonyService.h"
#include "nsRadioInterfaceLayer.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"

#define MOZSETTINGS_CHANGED_ID               "mozsettings-changed"
#define AUDIO_VOLUME_BT_SCO_ID               "audio.volume.bt_sco"




#define BT_HF_DISPATCH_MAIN(args...) \
  NS_DispatchToMainThread(new MainThreadTask(args))




#define BT_HF_PROCESS_CB(func, args...)          \
  do {                                           \
    NS_ENSURE_TRUE_VOID(sBluetoothHfpManager);   \
    sBluetoothHfpManager->func(args);            \
  } while(0)

using namespace mozilla;
using namespace mozilla::ipc;
USING_BLUETOOTH_NAMESPACE

namespace {
  StaticRefPtr<BluetoothHfpManager> sBluetoothHfpManager;
  static BluetoothHandsfreeInterface* sBluetoothHfpInterface = nullptr;

  bool sInShutdown = false;

  
  
  static int sWaitingForDialingInterval = 2000; 

  
  
  
  
  static int sBusyToneInterval = 3700; 
} 


enum MainThreadTaskCmd {
  NOTIFY_CONN_STATE_CHANGED,
  NOTIFY_DIALER,
  NOTIFY_SCO_VOLUME_CHANGED,
  POST_TASK_RESPOND_TO_BLDN,
  POST_TASK_CLOSE_SCO
};

static void
ConnectionStateCallback(bthf_connection_state_t state, bt_bdaddr_t* bd_addr)
{
  BT_HF_PROCESS_CB(ProcessConnectionState, state, bd_addr);
}

static void
AudioStateCallback(bthf_audio_state_t state, bt_bdaddr_t* bd_addr)
{
  BT_HF_PROCESS_CB(ProcessAudioState, state, bd_addr);
}

static void
VoiceRecognitionCallback(bthf_vr_state_t state)
{
  
}

static void
AnswerCallCallback()
{
  BT_HF_PROCESS_CB(ProcessAnswerCall);
}

static void
HangupCallCallback()
{
  BT_HF_PROCESS_CB(ProcessHangupCall);
}

static void
VolumeControlCallback(bthf_volume_type_t type, int volume)
{
  BT_HF_PROCESS_CB(ProcessVolumeControl, type, volume);
}

static void
DialCallCallback(char *number)
{
  BT_HF_PROCESS_CB(ProcessDialCall, number);
}

static void
DtmfCmdCallback(char dtmf)
{
  BT_HF_PROCESS_CB(ProcessDtmfCmd, dtmf);
}

static void
NoiceReductionCallback(bthf_nrec_t nrec)
{
  
}

static void
AtChldCallback(bthf_chld_type_t chld)
{
  BT_HF_PROCESS_CB(ProcessAtChld, chld);
}

static void
AtCnumCallback()
{
  BT_HF_PROCESS_CB(ProcessAtCnum);
}

static void
AtCindCallback()
{
  BT_HF_PROCESS_CB(ProcessAtCind);
}

static void
AtCopsCallback()
{
  BT_HF_PROCESS_CB(ProcessAtCops);
}

static void
AtClccCallback()
{
  BT_HF_PROCESS_CB(ProcessAtClcc);
}

static void
UnknownAtCallback(char *at_string)
{
  BT_HF_PROCESS_CB(ProcessUnknownAt, at_string);
}

static void
KeyPressedCallback()
{
  BT_HF_PROCESS_CB(ProcessKeyPressed);
}

static bthf_callbacks_t sBluetoothHfpCallbacks = {
  sizeof(sBluetoothHfpCallbacks),
  ConnectionStateCallback,
  AudioStateCallback,
  VoiceRecognitionCallback,
  AnswerCallCallback,
  HangupCallCallback,
  VolumeControlCallback,
  DialCallCallback,
  DtmfCmdCallback,
  NoiceReductionCallback,
  AtChldCallback,
  AtCnumCallback,
  AtCindCallback,
  AtCopsCallback,
  AtClccCallback,
  UnknownAtCallback,
  KeyPressedCallback
};

static bool
IsValidDtmf(const char aChar) {
  
  return (aChar == '*' || aChar == '#') ||
         (aChar >= '0' && aChar <= '9') ||
         (aChar >= 'A' && aChar <= 'D');
}

class BluetoothHfpManager::GetVolumeTask MOZ_FINAL : public nsISettingsServiceCallback
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD
  Handle(const nsAString& aName, JS::Handle<JS::Value> aResult)
  {
    MOZ_ASSERT(NS_IsMainThread());

    JSContext *cx = nsContentUtils::GetCurrentJSContext();
    NS_ENSURE_TRUE(cx, NS_OK);

    if (!aResult.isNumber()) {
      BT_WARNING("'" AUDIO_VOLUME_BT_SCO_ID "' is not a number!");
      return NS_OK;
    }

    BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
    hfp->mCurrentVgs = aResult.toNumber();

    return NS_OK;
  }

  NS_IMETHOD
  HandleError(const nsAString& aName)
  {
    BT_WARNING("Unable to get value for '" AUDIO_VOLUME_BT_SCO_ID "'");
    return NS_OK;
  }
};

class BluetoothHfpManager::CloseScoTask : public Task
{
private:
  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(sBluetoothHfpManager);
    sBluetoothHfpManager->DisconnectSco();
  }
};

class BluetoothHfpManager::RespondToBLDNTask : public Task
{
private:
  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(sBluetoothHfpManager);

    if (!sBluetoothHfpManager->mDialingRequestProcessed) {
      sBluetoothHfpManager->mDialingRequestProcessed = true;
      sBluetoothHfpManager->SendResponse(HFP_AT_RESPONSE_ERROR);
    }
  }
};

class BluetoothHfpManager::MainThreadTask : public nsRunnable
{
public:
  MainThreadTask(const int aCommand,
                 const nsAString& aParameter = EmptyString())
    : mCommand(aCommand), mParameter(aParameter)
  {
  }

  nsresult Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(sBluetoothHfpManager);

    switch (mCommand) {
      case MainThreadTaskCmd::NOTIFY_CONN_STATE_CHANGED:
        sBluetoothHfpManager->NotifyConnectionStateChanged(mParameter);
        break;
      case MainThreadTaskCmd::NOTIFY_DIALER:
        sBluetoothHfpManager->NotifyDialer(mParameter);
        break;
      case MainThreadTaskCmd::NOTIFY_SCO_VOLUME_CHANGED:
        {
          nsCOMPtr<nsIObserverService> os =
            mozilla::services::GetObserverService();
          NS_ENSURE_TRUE(os, NS_OK);

          os->NotifyObservers(nullptr, "bluetooth-volume-change",
                              mParameter.get());
        }
        break;
      case MainThreadTaskCmd::POST_TASK_RESPOND_TO_BLDN:
        MessageLoop::current()->
          PostDelayedTask(FROM_HERE, new RespondToBLDNTask(),
                          sWaitingForDialingInterval);
        break;
      case MainThreadTaskCmd::POST_TASK_CLOSE_SCO:
        MessageLoop::current()->
          PostDelayedTask(FROM_HERE, new CloseScoTask(),
                          sBusyToneInterval);
        break;
      default:
        BT_WARNING("MainThreadTask: Unknown command %d", mCommand);
        break;
    }

    return NS_OK;
  }

private:
  int mCommand;
  nsString mParameter;
};

NS_IMPL_ISUPPORTS(BluetoothHfpManager::GetVolumeTask,
                  nsISettingsServiceCallback);




Call::Call()
{
  Reset();
}

void
Call::Set(const nsAString& aNumber, const bool aIsOutgoing)
{
  mNumber = aNumber;
  mDirection = (aIsOutgoing) ? HFP_CALL_DIRECTION_OUTGOING :
                               HFP_CALL_DIRECTION_INCOMING;
  
  if (aNumber.Length() && aNumber[0] == '+') {
    mType = HFP_CALL_ADDRESS_TYPE_INTERNATIONAL;
  } else {
    mType = HFP_CALL_ADDRESS_TYPE_UNKNOWN;
  }
}

void
Call::Reset()
{
  mState = nsITelephonyService::CALL_STATE_DISCONNECTED;
  mDirection = HFP_CALL_DIRECTION_OUTGOING;
  mNumber.Truncate();
  mType = HFP_CALL_ADDRESS_TYPE_UNKNOWN;
}

bool
Call::IsActive()
{
  return (mState == nsITelephonyService::CALL_STATE_CONNECTED);
}




BluetoothHfpManager::BluetoothHfpManager() : mPhoneType(PhoneType::NONE)
{
  Reset();
}

void
BluetoothHfpManager::ResetCallArray()
{
  mCurrentCallArray.Clear();
  
  
  Call call;
  mCurrentCallArray.AppendElement(call);

  if (mPhoneType == PhoneType::CDMA) {
    mCdmaSecondCall.Reset();
  }
}

void
BluetoothHfpManager::Reset()
{
  mReceiveVgsFlag = false;
  mDialingRequestProcessed = true;

  mConnectionState = BTHF_CONNECTION_STATE_DISCONNECTED;
  mPrevConnectionState = BTHF_CONNECTION_STATE_DISCONNECTED;
  mAudioState = BTHF_AUDIO_STATE_DISCONNECTED;

  
  ResetCallArray();
  mBattChg = 5;
  mService = HFP_NETWORK_STATE_NOT_AVAILABLE;
  mRoam = HFP_SERVICE_TYPE_HOME;
  mSignal = 0;

  mController = nullptr;
}

bool
BluetoothHfpManager::Init()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE(obs, false);

  if (NS_FAILED(obs->AddObserver(this, MOZSETTINGS_CHANGED_ID, false)) ||
      NS_FAILED(obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false))) {
    BT_WARNING("Failed to add observers!");
    return false;
  }

  hal::RegisterBatteryObserver(this);
  
  hal::BatteryInformation batteryInfo;
  hal::GetCurrentBatteryInformation(&batteryInfo);
  Notify(batteryInfo);

  mListener = new BluetoothRilListener();
  NS_ENSURE_TRUE(mListener->Listen(true), false);

  nsCOMPtr<nsISettingsService> settings =
    do_GetService("@mozilla.org/settingsService;1");
  NS_ENSURE_TRUE(settings, false);

  nsCOMPtr<nsISettingsServiceLock> settingsLock;
  nsresult rv = settings->CreateLock(nullptr, getter_AddRefs(settingsLock));
  NS_ENSURE_SUCCESS(rv, false);

  nsRefPtr<GetVolumeTask> callback = new GetVolumeTask();
  rv = settingsLock->Get(AUDIO_VOLUME_BT_SCO_ID, callback);
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

class CleanupInitResultHandler MOZ_FINAL : public BluetoothHandsfreeResultHandler
{
public:
  CleanupInitResultHandler(BluetoothHandsfreeInterface* aInterface,
                           BluetoothProfileResultHandler* aRes)
  : mInterface(aInterface)
  , mRes(aRes)
  {
    MOZ_ASSERT(mInterface);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::Init failed: %d", (int)aStatus);
    if (mRes) {
      mRes->OnError(NS_ERROR_FAILURE);
    }
  }

  void Init() MOZ_OVERRIDE
  {
    sBluetoothHfpInterface = mInterface;
    if (mRes) {
      mRes->Init();
    }
  }

  void Cleanup() MOZ_OVERRIDE
  {
    sBluetoothHfpInterface = nullptr;
    



    RunInit();
  }

  void RunInit()
  {
    mInterface->Init(&sBluetoothHfpCallbacks, this);
  }

private:
  BluetoothHandsfreeInterface* mInterface;
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};

class InitResultHandlerRunnable MOZ_FINAL : public nsRunnable
{
public:
  InitResultHandlerRunnable(CleanupInitResultHandler* aRes)
  : mRes(aRes)
  {
    MOZ_ASSERT(mRes);
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    mRes->RunInit();
    return NS_OK;
  }

private:
  nsRefPtr<CleanupInitResultHandler> mRes;
};


void
BluetoothHfpManager::InitHfpInterface(BluetoothProfileResultHandler* aRes)
{
  BluetoothInterface* btInf = BluetoothInterface::GetInstance();
  if (!btInf) {
    BT_LOGR("Error: Bluetooth interface not available");
    if (aRes) {
      aRes->OnError(NS_ERROR_FAILURE);
    }
    return;
  }

  BluetoothHandsfreeInterface *interface =
    btInf->GetBluetoothHandsfreeInterface();
  if (!interface) {
    BT_LOGR("Error: Bluetooth Handsfree interface not available");
    if (aRes) {
      aRes->OnError(NS_ERROR_FAILURE);
    }
    return;
  }

  nsRefPtr<CleanupInitResultHandler> res =
    new CleanupInitResultHandler(interface, aRes);

  if (sBluetoothHfpInterface) {
    
    sBluetoothHfpInterface->Cleanup(res);
  } else {
    
    
    nsRefPtr<nsRunnable> r = new InitResultHandlerRunnable(res);
    if (NS_FAILED(NS_DispatchToMainThread(r))) {
      BT_LOGR("Failed to dispatch HFP init runnable");
    }
  }
}

BluetoothHfpManager::~BluetoothHfpManager()
{
  if (!mListener->Listen(false)) {
    BT_WARNING("Failed to stop listening RIL");
  }
  mListener = nullptr;

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE_VOID(obs);

  if (NS_FAILED(obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) ||
      NS_FAILED(obs->RemoveObserver(this, MOZSETTINGS_CHANGED_ID))) {
    BT_WARNING("Failed to remove observers!");
  }

  hal::UnregisterBatteryObserver(this);
}

class CleanupResultHandler MOZ_FINAL : public BluetoothHandsfreeResultHandler
{
public:
  CleanupResultHandler(BluetoothProfileResultHandler* aRes)
  : mRes(aRes)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::Cleanup failed: %d", (int)aStatus);
    if (mRes) {
      mRes->OnError(NS_ERROR_FAILURE);
    }
  }

  void Cleanup() MOZ_OVERRIDE
  {
    sBluetoothHfpInterface = nullptr;
    if (mRes) {
      mRes->Deinit();
    }
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};

class DeinitResultHandlerRunnable MOZ_FINAL : public nsRunnable
{
public:
  DeinitResultHandlerRunnable(BluetoothProfileResultHandler* aRes)
  : mRes(aRes)
  {
    MOZ_ASSERT(mRes);
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    mRes->Deinit();
    return NS_OK;
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};


void
BluetoothHfpManager::DeinitHfpInterface(BluetoothProfileResultHandler* aRes)
{
  if (sBluetoothHfpInterface) {
    sBluetoothHfpInterface->Cleanup(new CleanupResultHandler(aRes));
  } else if (aRes) {
    
    
    nsRefPtr<nsRunnable> r = new DeinitResultHandlerRunnable(aRes);
    if (NS_FAILED(NS_DispatchToMainThread(r))) {
      BT_LOGR("Failed to dispatch cleanup-result-handler runnable");
    }
  }
}


BluetoothHfpManager*
BluetoothHfpManager::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (sBluetoothHfpManager) {
    return sBluetoothHfpManager;
  }

  
  NS_ENSURE_FALSE(sInShutdown, nullptr);

  
  BluetoothHfpManager* manager = new BluetoothHfpManager();
  NS_ENSURE_TRUE(manager->Init(), nullptr);

  sBluetoothHfpManager = manager;
  return sBluetoothHfpManager;
}

NS_IMETHODIMP
BluetoothHfpManager::Observe(nsISupports* aSubject,
                             const char* aTopic,
                             const char16_t* aData)
{
  if (!strcmp(aTopic, MOZSETTINGS_CHANGED_ID)) {
    HandleVolumeChanged(nsDependentString(aData));
  } else if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    HandleShutdown();
  } else {
    MOZ_ASSERT(false, "BluetoothHfpManager got unexpected topic!");
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

void
BluetoothHfpManager::Notify(const hal::BatteryInformation& aBatteryInfo)
{
  
  
  mBattChg = (int) round(aBatteryInfo.level() * 5.0);
  UpdateDeviceCIND();
}

void
BluetoothHfpManager::ProcessConnectionState(bthf_connection_state_t aState,
                                            bt_bdaddr_t* aBdAddress)
{
  BT_LOGR("state %d", aState);

  mPrevConnectionState = mConnectionState;
  mConnectionState = aState;

  if (aState == BTHF_CONNECTION_STATE_SLC_CONNECTED) {
    BdAddressTypeToString(aBdAddress, mDeviceAddress);
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_CONN_STATE_CHANGED,
                        NS_LITERAL_STRING(BLUETOOTH_HFP_STATUS_CHANGED_ID));
  } else if (aState == BTHF_CONNECTION_STATE_DISCONNECTED) {
    DisconnectSco();
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_CONN_STATE_CHANGED,
                        NS_LITERAL_STRING(BLUETOOTH_HFP_STATUS_CHANGED_ID));
  }
}

void
BluetoothHfpManager::ProcessAudioState(bthf_audio_state_t aState,
                                       bt_bdaddr_t* aBdAddress)
{
  BT_LOGR("state %d", aState);

  mAudioState = aState;

  if (aState == BTHF_AUDIO_STATE_CONNECTED ||
      aState == BTHF_AUDIO_STATE_DISCONNECTED) {
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_CONN_STATE_CHANGED,
                        NS_LITERAL_STRING(BLUETOOTH_SCO_STATUS_CHANGED_ID));
  }
}

void
BluetoothHfpManager::ProcessAnswerCall()
{
  BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                      NS_LITERAL_STRING("ATA"));
}

void
BluetoothHfpManager::ProcessHangupCall()
{
  BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                      NS_LITERAL_STRING("CHUP"));
}

void
BluetoothHfpManager::ProcessVolumeControl(bthf_volume_type_t aType,
                                          int aVolume)
{
  NS_ENSURE_TRUE_VOID(aVolume >= 0 && aVolume <= 15);

  if (aType == BTHF_VOLUME_TYPE_MIC) {
    mCurrentVgm = aVolume;
  } else if (aType == BTHF_VOLUME_TYPE_SPK) {
    mReceiveVgsFlag = true;

    if (aVolume == mCurrentVgs) {
      
      return;
    }

    nsString data;
    data.AppendInt(aVolume);
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_SCO_VOLUME_CHANGED, data);
  }
}

void
BluetoothHfpManager::ProcessDtmfCmd(char aDtmf)
{
  NS_ENSURE_TRUE_VOID(IsValidDtmf(aDtmf));

  nsAutoCString message("VTS=");
  message += aDtmf;
  BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                      NS_ConvertUTF8toUTF16(message));
}

void
BluetoothHfpManager::ProcessAtChld(bthf_chld_type_t aChld)
{
  nsAutoCString message("CHLD=");
  message.AppendInt((int)aChld);
  BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                      NS_ConvertUTF8toUTF16(message));

  SendResponse(HFP_AT_RESPONSE_OK);
}

void BluetoothHfpManager::ProcessDialCall(char *aNumber)
{
  nsAutoCString message(aNumber);

  
  
  
  
  
  
  
  
  if (message.IsEmpty()) {
    mDialingRequestProcessed = false;
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                        NS_LITERAL_STRING("BLDN"));
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::POST_TASK_RESPOND_TO_BLDN);
  } else if (message[0] == '>') {
    mDialingRequestProcessed = false;
    nsAutoCString newMsg("ATD");
    newMsg += StringHead(message, message.Length() - 1);
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                        NS_ConvertUTF8toUTF16(newMsg));
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::POST_TASK_RESPOND_TO_BLDN);
  } else {
    nsAutoCString newMsg("ATD");
    newMsg += StringHead(message, message.Length() - 1);
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                        NS_ConvertUTF8toUTF16(newMsg));
    SendResponse(HFP_AT_RESPONSE_OK);
  }
}

void
BluetoothHfpManager::ProcessAtCnum()
{
  if (!mMsisdn.IsEmpty()) {
    nsAutoCString message("+CNUM: ,\"");
    message.Append(NS_ConvertUTF16toUTF8(mMsisdn).get());
    message.AppendLiteral("\",");
    message.AppendInt(BTHF_CALL_ADDRTYPE_UNKNOWN);
    message.AppendLiteral(",,4");

    SendLine(message.get());
  }

  SendResponse(HFP_AT_RESPONSE_OK);
}

class CindResponseResultHandler MOZ_FINAL
: public BluetoothHandsfreeResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::CindResponse failed: %d",
               (int)aStatus);
  }
};

void
BluetoothHfpManager::ProcessAtCind()
{
  NS_ENSURE_TRUE_VOID(sBluetoothHfpInterface);

  int numActive = GetNumberOfCalls(nsITelephonyService::CALL_STATE_CONNECTED);
  int numHeld = GetNumberOfCalls(nsITelephonyService::CALL_STATE_HELD);
  BluetoothHandsfreeCallState callState =
    ConvertToBluetoothHandsfreeCallState(GetCallSetupState());

  sBluetoothHfpInterface->CindResponse(mService, numActive, numHeld,
                                       callState, mSignal, mRoam, mBattChg,
                                       new CindResponseResultHandler());
}

class CopsResponseResultHandler MOZ_FINAL
: public BluetoothHandsfreeResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::CopsResponse failed: %d",
               (int)aStatus);
  }
};

void
BluetoothHfpManager::ProcessAtCops()
{
  NS_ENSURE_TRUE_VOID(sBluetoothHfpInterface);

  sBluetoothHfpInterface->CopsResponse(
    NS_ConvertUTF16toUTF8(mOperatorName).get(),
    new CopsResponseResultHandler());
}

void
BluetoothHfpManager::ProcessAtClcc()
{
  uint32_t callNumbers = mCurrentCallArray.Length();
  uint32_t i;
  for (i = 1; i < callNumbers; i++) {
    SendCLCC(mCurrentCallArray[i], i);
  }

  if (!mCdmaSecondCall.mNumber.IsEmpty()) {
    MOZ_ASSERT(mPhoneType == PhoneType::CDMA);
    MOZ_ASSERT(i == 2);

    SendCLCC(mCdmaSecondCall, 2);
  }

  SendResponse(HFP_AT_RESPONSE_OK);
}

class AtResponseResultHandler MOZ_FINAL
: public BluetoothHandsfreeResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::AtResponse failed: %d",
               (int)aStatus);
  }
};

void
BluetoothHfpManager::ProcessUnknownAt(char *aAtString)
{
  BT_LOGR("[%s]", aAtString);

  NS_ENSURE_TRUE_VOID(sBluetoothHfpInterface);

  sBluetoothHfpInterface->AtResponse(HFP_AT_RESPONSE_ERROR, 0,
                                     new AtResponseResultHandler());
}

void
BluetoothHfpManager::ProcessKeyPressed()
{
  bool hasActiveCall =
    (FindFirstCall(nsITelephonyService::CALL_STATE_CONNECTED) > 0);

  
  if (FindFirstCall(nsITelephonyService::CALL_STATE_INCOMING)
      && !hasActiveCall) {
    





    ProcessAnswerCall();
  } else if (hasActiveCall) {
    if (!IsScoConnected()) {
      



      ConnectSco();
    } else {
      





      ProcessHangupCall();
    }
  } else {
    
    mDialingRequestProcessed = false;
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                        NS_LITERAL_STRING("BLDN"));
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::POST_TASK_RESPOND_TO_BLDN);
  }
}

void
BluetoothHfpManager::NotifyConnectionStateChanged(const nsAString& aType)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  nsCOMPtr<nsIObserverService> obs =
    do_GetService("@mozilla.org/observer-service;1");
  NS_ENSURE_TRUE_VOID(obs);

  if (NS_FAILED(obs->NotifyObservers(this, NS_ConvertUTF16toUTF8(aType).get(),
                                     mDeviceAddress.get()))) {
    BT_WARNING("Failed to notify observsers!");
  }

  
  bool status;
  nsAutoString eventName;
  if (aType.EqualsLiteral(BLUETOOTH_HFP_STATUS_CHANGED_ID)) {
    status = IsConnected();
    eventName.AssignLiteral(HFP_STATUS_CHANGED_ID);
  } else if (aType.EqualsLiteral(BLUETOOTH_SCO_STATUS_CHANGED_ID)) {
    status = IsScoConnected();
    eventName.AssignLiteral(SCO_STATUS_CHANGED_ID);
  } else {
    MOZ_ASSERT(false);
    return;
  }

  DispatchStatusChangedEvent(eventName, mDeviceAddress, status);

  
  if (aType.EqualsLiteral(BLUETOOTH_HFP_STATUS_CHANGED_ID)) {
    if (IsConnected()) {
      MOZ_ASSERT(mListener);

      
      mListener->EnumerateCalls();

      OnConnect(EmptyString());
    } else if (mConnectionState == BTHF_CONNECTION_STATE_DISCONNECTED) {
      mDeviceAddress.AssignLiteral(BLUETOOTH_ADDRESS_NONE);
      if (mPrevConnectionState == BTHF_CONNECTION_STATE_DISCONNECTED) {
        
        
        
        
        OnConnect(NS_LITERAL_STRING(ERR_CONNECTION_FAILED));
      } else {
        OnDisconnect(EmptyString());
      }
      Reset();
    }
  }
}

void
BluetoothHfpManager::NotifyDialer(const nsAString& aCommand)
{
  NS_NAMED_LITERAL_STRING(type, "bluetooth-dialer-command");
  InfallibleTArray<BluetoothNamedValue> parameters;

  BT_APPEND_NAMED_VALUE(parameters, "command", nsString(aCommand));

  BT_ENSURE_TRUE_VOID_BROADCAST_SYSMSG(type, parameters);
}

class VolumeControlResultHandler MOZ_FINAL
: public BluetoothHandsfreeResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::VolumeControl failed: %d",
               (int)aStatus);
  }
};

void
BluetoothHfpManager::HandleVolumeChanged(const nsAString& aData)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  JSContext* cx = nsContentUtils::GetSafeJSContext();
  NS_ENSURE_TRUE_VOID(cx);

  JS::Rooted<JS::Value> val(cx);
  NS_ENSURE_TRUE_VOID(JS_ParseJSON(cx, aData.BeginReading(), aData.Length(), &val));
  NS_ENSURE_TRUE_VOID(val.isObject());

  JS::Rooted<JSObject*> obj(cx, &val.toObject());
  JS::Rooted<JS::Value> key(cx);
  if (!JS_GetProperty(cx, obj, "key", &key) || !key.isString()) {
    return;
  }

  bool match;
  if (!JS_StringEqualsAscii(cx, key.toString(), AUDIO_VOLUME_BT_SCO_ID, &match) ||
      !match) {
    return;
  }

  JS::Rooted<JS::Value> value(cx);
  if (!JS_GetProperty(cx, obj, "value", &value) ||
      !value.isNumber()) {
    return;
  }

  mCurrentVgs = value.toNumber();

  
  if (mReceiveVgsFlag) {
    mReceiveVgsFlag = false;
    return;
  }

  
  if (IsConnected()) {
    NS_ENSURE_TRUE_VOID(sBluetoothHfpInterface);
    sBluetoothHfpInterface->VolumeControl(HFP_VOLUME_TYPE_SPEAKER, mCurrentVgs,
                                          new VolumeControlResultHandler());
  }
}

void
BluetoothHfpManager::HandleVoiceConnectionChanged(uint32_t aClientId)
{
  nsCOMPtr<nsIMobileConnectionProvider> connection =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE_VOID(connection);

  nsCOMPtr<nsIMobileConnectionInfo> voiceInfo;
  connection->GetVoiceConnectionInfo(aClientId, getter_AddRefs(voiceInfo));
  NS_ENSURE_TRUE_VOID(voiceInfo);

  nsString type;
  voiceInfo->GetType(type);
  mPhoneType = GetPhoneType(type);

  
  bool roaming;
  voiceInfo->GetRoaming(&roaming);
  mRoam = (roaming) ? HFP_SERVICE_TYPE_ROAMING : HFP_SERVICE_TYPE_HOME;

  
  nsString regState;
  voiceInfo->GetState(regState);

  BluetoothHandsfreeNetworkState service =
    (regState.EqualsLiteral("registered")) ? HFP_NETWORK_STATE_AVAILABLE :
                                             HFP_NETWORK_STATE_NOT_AVAILABLE;
  if (service != mService) {
    
    mListener->ServiceChanged(aClientId, service);
  }
  mService = service;

  
  JSContext* cx = nsContentUtils::GetSafeJSContext();
  NS_ENSURE_TRUE_VOID(cx);
  JS::Rooted<JS::Value> value(cx);
  voiceInfo->GetRelSignalStrength(&value);
  NS_ENSURE_TRUE_VOID(value.isNumber());
  mSignal = (int)ceil(value.toNumber() / 20.0);

  UpdateDeviceCIND();

  
  nsCOMPtr<nsIMobileNetworkInfo> network;
  voiceInfo->GetNetwork(getter_AddRefs(network));
  NS_ENSURE_TRUE_VOID(network);
  network->GetLongName(mOperatorName);

  
  
  
  
  
  
  
  
  if (mOperatorName.Length() > 16) {
    BT_WARNING("The operator name was longer than 16 characters. We cut it.");
    mOperatorName.Left(mOperatorName, 16);
  }
}

void
BluetoothHfpManager::HandleIccInfoChanged(uint32_t aClientId)
{
  nsCOMPtr<nsIIccProvider> icc =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE_VOID(icc);

  nsCOMPtr<nsIDOMMozIccInfo> iccInfo;
  icc->GetIccInfo(aClientId, getter_AddRefs(iccInfo));
  NS_ENSURE_TRUE_VOID(iccInfo);

  nsCOMPtr<nsIDOMMozGsmIccInfo> gsmIccInfo = do_QueryInterface(iccInfo);
  NS_ENSURE_TRUE_VOID(gsmIccInfo);
  gsmIccInfo->GetMsisdn(mMsisdn);
}

void
BluetoothHfpManager::HandleShutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  sInShutdown = true;
  Disconnect(nullptr);
  DisconnectSco();
  sBluetoothHfpManager = nullptr;
}

class ClccResponseResultHandler MOZ_FINAL
: public BluetoothHandsfreeResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::ClccResponse failed: %d",
               (int)aStatus);
  }
};

void
BluetoothHfpManager::SendCLCC(Call& aCall, int aIndex)
{
  NS_ENSURE_TRUE_VOID(aCall.mState !=
                        nsITelephonyService::CALL_STATE_DISCONNECTED);
  NS_ENSURE_TRUE_VOID(sBluetoothHfpInterface);

  BluetoothHandsfreeCallState callState =
    ConvertToBluetoothHandsfreeCallState(aCall.mState);

  if (mPhoneType == PhoneType::CDMA && aIndex == 1 && aCall.IsActive()) {
    callState = (mCdmaSecondCall.IsActive()) ? HFP_CALL_STATE_HELD :
                                               HFP_CALL_STATE_ACTIVE;
  }

  if (callState == HFP_CALL_STATE_INCOMING &&
      FindFirstCall(nsITelephonyService::CALL_STATE_CONNECTED)) {
    callState = HFP_CALL_STATE_WAITING;
  }

  sBluetoothHfpInterface->ClccResponse(
    aIndex, aCall.mDirection, callState, HFP_CALL_MODE_VOICE,
    HFP_CALL_MPTY_TYPE_SINGLE, aCall.mNumber,
    aCall.mType, new ClccResponseResultHandler());
}

class FormattedAtResponseResultHandler MOZ_FINAL
: public BluetoothHandsfreeResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::FormattedAtResponse failed: %d",
               (int)aStatus);
  }
};

void
BluetoothHfpManager::SendLine(const char* aMessage)
{
  NS_ENSURE_TRUE_VOID(sBluetoothHfpInterface);

  sBluetoothHfpInterface->FormattedAtResponse(
    aMessage, new FormattedAtResponseResultHandler());
}

void
BluetoothHfpManager::SendResponse(BluetoothHandsfreeAtResponse aResponseCode)
{
  NS_ENSURE_TRUE_VOID(sBluetoothHfpInterface);

  sBluetoothHfpInterface->AtResponse(
    aResponseCode, 0, new AtResponseResultHandler());
}

class PhoneStateChangeResultHandler MOZ_FINAL
: public BluetoothHandsfreeResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::PhoneStateChange failed: %d",
               (int)aStatus);
  }
};

void
BluetoothHfpManager::UpdatePhoneCIND(uint32_t aCallIndex)
{
  NS_ENSURE_TRUE_VOID(sBluetoothHfpInterface);

  int numActive = GetNumberOfCalls(nsITelephonyService::CALL_STATE_CONNECTED);
  int numHeld = GetNumberOfCalls(nsITelephonyService::CALL_STATE_HELD);
  BluetoothHandsfreeCallState callSetupState =
    ConvertToBluetoothHandsfreeCallState(GetCallSetupState());
  BluetoothHandsfreeCallAddressType type = mCurrentCallArray[aCallIndex].mType;

  BT_LOGR("[%d] state %d => BTHF: active[%d] held[%d] setupstate[%d]",
          aCallIndex, mCurrentCallArray[aCallIndex].mState,
          numActive, numHeld, callSetupState);

  sBluetoothHfpInterface->PhoneStateChange(
    numActive, numHeld, callSetupState,
    mCurrentCallArray[aCallIndex].mNumber, type,
    new PhoneStateChangeResultHandler());
}

class DeviceStatusNotificationResultHandler MOZ_FINAL
: public BluetoothHandsfreeResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING(
      "BluetoothHandsfreeInterface::DeviceStatusNotification failed: %d",
      (int)aStatus);
  }
};

void
BluetoothHfpManager::UpdateDeviceCIND()
{
  if (sBluetoothHfpInterface) {
    sBluetoothHfpInterface->DeviceStatusNotification(
      mService,
      mRoam,
      mSignal,
      mBattChg, new DeviceStatusNotificationResultHandler());
  }
}

uint32_t
BluetoothHfpManager::FindFirstCall(uint16_t aState)
{
  uint32_t callLength = mCurrentCallArray.Length();

  for (uint32_t i = 1; i < callLength; ++i) {
    if (mCurrentCallArray[i].mState == aState) {
      return i;
    }
  }

  return 0;
}

uint32_t
BluetoothHfpManager::GetNumberOfCalls(uint16_t aState)
{
  uint32_t num = 0;
  uint32_t callLength = mCurrentCallArray.Length();

  for (uint32_t i = 1; i < callLength; ++i) {
    if (mCurrentCallArray[i].mState == aState) {
      ++num;
    }
  }

  return num;
}

uint16_t
BluetoothHfpManager::GetCallSetupState()
{
  uint32_t callLength = mCurrentCallArray.Length();

  for (uint32_t i = 1; i < callLength; ++i) {
    switch (mCurrentCallArray[i].mState) {
      case nsITelephonyService::CALL_STATE_INCOMING:
      case nsITelephonyService::CALL_STATE_DIALING:
      case nsITelephonyService::CALL_STATE_ALERTING:
        return mCurrentCallArray[i].mState;
      default:
        break;
    }
  }

  return nsITelephonyService::CALL_STATE_DISCONNECTED;
}

BluetoothHandsfreeCallState
BluetoothHfpManager::ConvertToBluetoothHandsfreeCallState(int aCallState) const
{
  BluetoothHandsfreeCallState state;

  
  if (aCallState == nsITelephonyService::CALL_STATE_INCOMING) {
    state = HFP_CALL_STATE_INCOMING;
  } else if (aCallState == nsITelephonyService::CALL_STATE_DIALING) {
    state = HFP_CALL_STATE_DIALING;
  } else if (aCallState == nsITelephonyService::CALL_STATE_ALERTING) {
    state = HFP_CALL_STATE_ALERTING;
  } else if (aCallState == nsITelephonyService::CALL_STATE_CONNECTED) {
    state = HFP_CALL_STATE_ACTIVE;
  } else if (aCallState == nsITelephonyService::CALL_STATE_HELD) {
    state = HFP_CALL_STATE_HELD;
  } else { 
    state = HFP_CALL_STATE_IDLE;
  }

  return state;
}

bool
BluetoothHfpManager::IsTransitionState(uint16_t aCallState, bool aIsConference)
{
  








  if (!aIsConference) {
    switch (aCallState) {
      case nsITelephonyService::CALL_STATE_CONNECTED:
        return (GetNumberOfCalls(aCallState) > 1);
      case nsITelephonyService::CALL_STATE_HELD:
        return (GetNumberOfCalls(aCallState) > 1 ||
                FindFirstCall(nsITelephonyService::CALL_STATE_INCOMING));
      default:
        break;
    }
  }

  return false;
}

void
BluetoothHfpManager::HandleCallStateChanged(uint32_t aCallIndex,
                                            uint16_t aCallState,
                                            const nsAString& aError,
                                            const nsAString& aNumber,
                                            const bool aIsOutgoing,
                                            const bool aIsConference,
                                            bool aSend)
{
  
  
  
  if (aCallIndex == UINT32_MAX) {
    return;
  }

  
  while (aCallIndex >= mCurrentCallArray.Length()) {
    Call call;
    mCurrentCallArray.AppendElement(call);
  }
  mCurrentCallArray[aCallIndex].mState = aCallState;

  
  if (!IsConnected()) {
    return;
  }

  
  mCurrentCallArray[aCallIndex].Set(aNumber, aIsOutgoing);

  
  
  if (!IsTransitionState(aCallState, aIsConference)) {
    UpdatePhoneCIND(aCallIndex);
  }

  switch (aCallState) {
    case nsITelephonyService::CALL_STATE_DIALING:
      
      if (!mDialingRequestProcessed) {
        SendResponse(HFP_AT_RESPONSE_OK);
        mDialingRequestProcessed = true;
      }
      break;
    case nsITelephonyService::CALL_STATE_DISCONNECTED:
      
      if (mCurrentCallArray.Length() - 1 ==
          GetNumberOfCalls(nsITelephonyService::CALL_STATE_DISCONNECTED)) {
        
        
        if (aError.Equals(NS_LITERAL_STRING("BusyError"))) {
          
          
          BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::POST_TASK_CLOSE_SCO);
        }

        ResetCallArray();
      }
      break;
    default:
      break;
  }
}

PhoneType
BluetoothHfpManager::GetPhoneType(const nsAString& aType)
{
  
  if (aType.EqualsLiteral("gsm") || aType.EqualsLiteral("gprs") ||
      aType.EqualsLiteral("edge") || aType.EqualsLiteral("umts") ||
      aType.EqualsLiteral("hspa") || aType.EqualsLiteral("hsdpa") ||
      aType.EqualsLiteral("hsupa") || aType.EqualsLiteral("hspa+")) {
    return PhoneType::GSM;
  } else if (aType.EqualsLiteral("is95a") || aType.EqualsLiteral("is95b") ||
             aType.EqualsLiteral("1xrtt") || aType.EqualsLiteral("evdo0") ||
             aType.EqualsLiteral("evdoa") || aType.EqualsLiteral("evdob")) {
    return PhoneType::CDMA;
  }

  return PhoneType::NONE;
}

void
BluetoothHfpManager::UpdateSecondNumber(const nsAString& aNumber)
{
  MOZ_ASSERT(mPhoneType == PhoneType::CDMA);

  
  
  mCdmaSecondCall.Set(aNumber, false);

  
  
}

void
BluetoothHfpManager::AnswerWaitingCall()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mPhoneType == PhoneType::CDMA);

  
  mCdmaSecondCall.mState = nsITelephonyService::CALL_STATE_CONNECTED;
  
  

  
  
}

void
BluetoothHfpManager::IgnoreWaitingCall()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mPhoneType == PhoneType::CDMA);

  mCdmaSecondCall.Reset();
  
  
}

void
BluetoothHfpManager::ToggleCalls()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mPhoneType == PhoneType::CDMA);

  
  mCdmaSecondCall.mState = (mCdmaSecondCall.IsActive()) ?
                             nsITelephonyService::CALL_STATE_HELD :
                             nsITelephonyService::CALL_STATE_CONNECTED;
}

class ConnectAudioResultHandler MOZ_FINAL
: public BluetoothHandsfreeResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::ConnectAudio failed: %d",
               (int)aStatus);
  }
};

bool
BluetoothHfpManager::ConnectSco()
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE(!sInShutdown, false);
  NS_ENSURE_TRUE(IsConnected() && !IsScoConnected(), false);
  NS_ENSURE_TRUE(sBluetoothHfpInterface, false);

  sBluetoothHfpInterface->ConnectAudio(mDeviceAddress,
                                       new ConnectAudioResultHandler());

  return true;
}

class DisconnectAudioResultHandler MOZ_FINAL
: public BluetoothHandsfreeResultHandler
{
public:
  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::DisconnectAudio failed: %d",
               (int)aStatus);
  }
};

bool
BluetoothHfpManager::DisconnectSco()
{
  NS_ENSURE_TRUE(IsScoConnected(), false);
  NS_ENSURE_TRUE(sBluetoothHfpInterface, false);

  sBluetoothHfpInterface->DisconnectAudio(mDeviceAddress,
                                          new DisconnectAudioResultHandler());

  return true;
}

bool
BluetoothHfpManager::IsScoConnected()
{
  return (mAudioState == BTHF_AUDIO_STATE_CONNECTED);
}

bool
BluetoothHfpManager::IsConnected()
{
  return (mConnectionState == BTHF_CONNECTION_STATE_SLC_CONNECTED);
}

void
BluetoothHfpManager::OnConnectError()
{
  MOZ_ASSERT(NS_IsMainThread());

  mController->NotifyCompletion(NS_LITERAL_STRING(ERR_CONNECTION_FAILED));

  mController = nullptr;
  mDeviceAddress.Truncate();
}

class ConnectResultHandler MOZ_FINAL : public BluetoothHandsfreeResultHandler
{
public:
  ConnectResultHandler(BluetoothHfpManager* aHfpManager)
  : mHfpManager(aHfpManager)
  {
    MOZ_ASSERT(mHfpManager);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::Connect failed: %d",
               (int)aStatus);
    mHfpManager->OnConnectError();
  }

private:
  BluetoothHfpManager* mHfpManager;
};

void
BluetoothHfpManager::Connect(const nsAString& aDeviceAddress,
                             BluetoothProfileController* aController)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aController && !mController);

  if (sInShutdown) {
    aController->NotifyCompletion(NS_LITERAL_STRING(ERR_NO_AVAILABLE_RESOURCE));
    return;
  }

  if (!sBluetoothHfpInterface) {
    BT_LOGR("sBluetoothHfpInterface is null");
    aController->NotifyCompletion(NS_LITERAL_STRING(ERR_NO_AVAILABLE_RESOURCE));
    return;
  }

  mDeviceAddress = aDeviceAddress;
  mController = aController;

  sBluetoothHfpInterface->Connect(mDeviceAddress,
                                  new ConnectResultHandler(this));
}

void
BluetoothHfpManager::OnDisconnectError()
{
  MOZ_ASSERT(NS_IsMainThread());

  mController->NotifyCompletion(NS_LITERAL_STRING(ERR_CONNECTION_FAILED));
}

class DisconnectResultHandler MOZ_FINAL : public BluetoothHandsfreeResultHandler
{
public:
  DisconnectResultHandler(BluetoothHfpManager* aHfpManager)
  : mHfpManager(aHfpManager)
  {
    MOZ_ASSERT(mHfpManager);
  }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothHandsfreeInterface::Disconnect failed: %d",
               (int)aStatus);
    mHfpManager->OnDisconnectError();
  }

private:
  BluetoothHfpManager* mHfpManager;
};


void
BluetoothHfpManager::Disconnect(BluetoothProfileController* aController)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mController);

  if (!sBluetoothHfpInterface) {
    BT_LOGR("sBluetoothHfpInterface is null");
    aController->NotifyCompletion(NS_LITERAL_STRING(ERR_NO_AVAILABLE_RESOURCE));
    return;
  }

  mController = aController;

  sBluetoothHfpInterface->Disconnect(mDeviceAddress,
                                     new DisconnectResultHandler(this));
}

void
BluetoothHfpManager::OnConnect(const nsAString& aErrorStr)
{
  MOZ_ASSERT(NS_IsMainThread());

  



  NS_ENSURE_TRUE_VOID(mController);

  mController->NotifyCompletion(aErrorStr);
  mController = nullptr;
}

void
BluetoothHfpManager::OnDisconnect(const nsAString& aErrorStr)
{
  MOZ_ASSERT(NS_IsMainThread());

  



  NS_ENSURE_TRUE_VOID(mController);

  mController->NotifyCompletion(aErrorStr);
  mController = nullptr;
}

void
BluetoothHfpManager::OnUpdateSdpRecords(const nsAString& aDeviceAddress)
{
  
  MOZ_ASSERT(false);
}

void
BluetoothHfpManager::OnGetServiceChannel(const nsAString& aDeviceAddress,
                                         const nsAString& aServiceUuid,
                                         int aChannel)
{
  
  MOZ_ASSERT(false);
}

void
BluetoothHfpManager::GetAddress(nsAString& aDeviceAddress)
{
  aDeviceAddress = mDeviceAddress;
}





void
BluetoothHfpManager::ConnectionStateNotification(
  BluetoothHandsfreeConnectionState aState, const nsAString& aBdAddress)
{
  MOZ_ASSERT(NS_IsMainThread());

  BT_LOGR("state %d", aState);

  mPrevConnectionState = mConnectionState;
  mConnectionState = aState;

  if (aState == HFP_CONNECTION_STATE_SLC_CONNECTED) {
    mDeviceAddress = aBdAddress;
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_CONN_STATE_CHANGED,
                        NS_LITERAL_STRING(BLUETOOTH_HFP_STATUS_CHANGED_ID));
  } else if (aState == HFP_CONNECTION_STATE_DISCONNECTED) {
    DisconnectSco();
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_CONN_STATE_CHANGED,
                        NS_LITERAL_STRING(BLUETOOTH_HFP_STATUS_CHANGED_ID));
  }
}

void
BluetoothHfpManager::AudioStateNotification(
  BluetoothHandsfreeAudioState aState, const nsAString& aBdAddress)
{
  MOZ_ASSERT(NS_IsMainThread());

  BT_LOGR("state %d", aState);

  mAudioState = aState;

  if (aState == HFP_AUDIO_STATE_CONNECTED ||
      aState == HFP_AUDIO_STATE_DISCONNECTED) {
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_CONN_STATE_CHANGED,
                        NS_LITERAL_STRING(BLUETOOTH_SCO_STATUS_CHANGED_ID));
  }
}

void
BluetoothHfpManager::AnswerCallNotification()
{
  MOZ_ASSERT(NS_IsMainThread());

  BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                      NS_LITERAL_STRING("ATA"));
}

void
BluetoothHfpManager::HangupCallNotification()
{
  MOZ_ASSERT(NS_IsMainThread());

  BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                      NS_LITERAL_STRING("CHUP"));
}

void
BluetoothHfpManager::VolumeNotification(
  BluetoothHandsfreeVolumeType aType, int aVolume)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE_VOID(aVolume >= 0 && aVolume <= 15);

  if (aType == HFP_VOLUME_TYPE_MICROPHONE) {
    mCurrentVgm = aVolume;
  } else if (aType == HFP_VOLUME_TYPE_SPEAKER) {
    mReceiveVgsFlag = true;

    if (aVolume == mCurrentVgs) {
      
      return;
    }

    nsString data;
    data.AppendInt(aVolume);
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_SCO_VOLUME_CHANGED, data);
  }
}

void
BluetoothHfpManager::DtmfNotification(char aDtmf)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE_VOID(IsValidDtmf(aDtmf));

  nsAutoCString message("VTS=");
  message += aDtmf;
  BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                      NS_ConvertUTF8toUTF16(message));
}

void
BluetoothHfpManager::CallHoldNotification(BluetoothHandsfreeCallHoldType aChld)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoCString message("CHLD=");
  message.AppendInt((int)aChld);
  BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                      NS_ConvertUTF8toUTF16(message));

  SendResponse(HFP_AT_RESPONSE_OK);
}

void BluetoothHfpManager::DialCallNotification(const nsAString& aNumber)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCString message = NS_ConvertUTF16toUTF8(aNumber);

  
  
  
  
  
  
  
  
  if (message.IsEmpty()) {
    mDialingRequestProcessed = false;
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                        NS_LITERAL_STRING("BLDN"));
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::POST_TASK_RESPOND_TO_BLDN);
  } else if (message[0] == '>') {
    mDialingRequestProcessed = false;
    nsAutoCString newMsg("ATD");
    newMsg += StringHead(message, message.Length() - 1);
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                        NS_ConvertUTF8toUTF16(newMsg));
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::POST_TASK_RESPOND_TO_BLDN);
  } else {
    nsAutoCString newMsg("ATD");
    newMsg += StringHead(message, message.Length() - 1);
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                        NS_ConvertUTF8toUTF16(newMsg));
    SendResponse(HFP_AT_RESPONSE_OK);
  }
}

void
BluetoothHfpManager::CnumNotification()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mMsisdn.IsEmpty()) {
    nsAutoCString message("+CNUM: ,\"");
    message.Append(NS_ConvertUTF16toUTF8(mMsisdn).get());
    message.AppendLiteral("\",");
    message.AppendInt(BTHF_CALL_ADDRTYPE_UNKNOWN);
    message.AppendLiteral(",,4");

    SendLine(message.get());
  }

  SendResponse(HFP_AT_RESPONSE_OK);
}

void
BluetoothHfpManager::CindNotification()
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE_VOID(sBluetoothHfpInterface);

  int numActive = GetNumberOfCalls(nsITelephonyService::CALL_STATE_CONNECTED);
  int numHeld = GetNumberOfCalls(nsITelephonyService::CALL_STATE_HELD);
  BluetoothHandsfreeCallState callState =
    ConvertToBluetoothHandsfreeCallState(GetCallSetupState());

  sBluetoothHfpInterface->CindResponse(mService, numActive, numHeld,
                                       callState, mSignal, mRoam, mBattChg,
                                       new CindResponseResultHandler());
}

void
BluetoothHfpManager::CopsNotification()
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE_VOID(sBluetoothHfpInterface);

  sBluetoothHfpInterface->CopsResponse(
    NS_ConvertUTF16toUTF8(mOperatorName).get(),
    new CopsResponseResultHandler());
}

void
BluetoothHfpManager::ClccNotification()
{
  MOZ_ASSERT(NS_IsMainThread());

  uint32_t callNumbers = mCurrentCallArray.Length();
  uint32_t i;
  for (i = 1; i < callNumbers; i++) {
    SendCLCC(mCurrentCallArray[i], i);
  }

  if (!mCdmaSecondCall.mNumber.IsEmpty()) {
    MOZ_ASSERT(mPhoneType == PhoneType::CDMA);
    MOZ_ASSERT(i == 2);

    SendCLCC(mCdmaSecondCall, 2);
  }

  SendResponse(HFP_AT_RESPONSE_OK);
}

void
BluetoothHfpManager::UnknownAtNotification(const nsACString& aAtString)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCString atString(aAtString);

  BT_LOGR("[%s]", atString.get());

  NS_ENSURE_TRUE_VOID(sBluetoothHfpInterface);

  sBluetoothHfpInterface->AtResponse(HFP_AT_RESPONSE_ERROR, 0,
                                     new AtResponseResultHandler());
}

void
BluetoothHfpManager::KeyPressedNotification()
{
  MOZ_ASSERT(NS_IsMainThread());

  bool hasActiveCall =
    (FindFirstCall(nsITelephonyService::CALL_STATE_CONNECTED) > 0);

  
  if (FindFirstCall(nsITelephonyService::CALL_STATE_INCOMING)
      && !hasActiveCall) {
    





    ProcessAnswerCall();
  } else if (hasActiveCall) {
    if (!IsScoConnected()) {
      



      ConnectSco();
    } else {
      





      ProcessHangupCall();
    }
  } else {
    
    mDialingRequestProcessed = false;
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::NOTIFY_DIALER,
                        NS_LITERAL_STRING("BLDN"));
    BT_HF_DISPATCH_MAIN(MainThreadTaskCmd::POST_TASK_RESPOND_TO_BLDN);
  }
}

NS_IMPL_ISUPPORTS(BluetoothHfpManager, nsIObserver)

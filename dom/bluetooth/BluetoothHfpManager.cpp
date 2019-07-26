





#include "base/basictypes.h"

#include "BluetoothHfpManager.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothScoManager.h"
#include "BluetoothService.h"
#include "BluetoothUtils.h"
#include "BluetoothUuid.h"

#include "MobileConnection.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "nsContentUtils.h"
#include "nsIAudioManager.h"
#include "nsIObserverService.h"
#include "nsISettingsService.h"
#include "nsIRadioInterfaceLayer.h"
#include "nsRadioInterfaceLayer.h"

#define AUDIO_VOLUME_BT_SCO "audio.volume.bt_sco"
#define MOZSETTINGS_CHANGED_ID "mozsettings-changed"
#define MOBILE_CONNECTION_ICCINFO_CHANGED "mobile-connection-iccinfo-changed"
#define MOBILE_CONNECTION_VOICE_CHANGED "mobile-connection-voice-changed"

#define TOA_UNKNOWN 0x81
#define TOA_INTERNATIONAL 0x91






#define TOA_UNKNOWN 0x81
#define TOA_INTERNATIONAL 0x91

using namespace mozilla;
using namespace mozilla::ipc;
USING_BLUETOOTH_NAMESPACE

namespace {
  StaticRefPtr<BluetoothHfpManager> gBluetoothHfpManager;
  StaticRefPtr<BluetoothHfpManagerObserver> sHfpObserver;
  bool gInShutdown = false;
  static bool sStopSendingRingFlag = true;

  static int sRingInterval = 3000; 
} 





enum CallState {
  NO_CALL,
  IN_PROGRESS
};







enum CallSetupState {
  NO_CALLSETUP,
  INCOMING,
  OUTGOING,
  OUTGOING_ALERTING
};






enum CallHeldState {
  NO_CALLHELD,
  ONHOLD_ACTIVE,
  ONHOLD_NOACTIVE
};

typedef struct {
  const char* name;
  const char* range;
  int value;
} CINDItem;

enum CINDType {
  BATTCHG = 1,
  CALL,
  CALLHELD,
  CALLSETUP,
  SERVICE,
  SIGNAL,
  ROAM
};

static CINDItem sCINDItems[] = {
  {},
  {"battchg", "0-5", 5},
  {"call", "0,1", CallState::NO_CALL},
  {"callheld", "0-2", CallHeldState::NO_CALLHELD},
  {"callsetup", "0-3", CallSetupState::NO_CALLSETUP},
  {"service", "0,1", 0},
  {"signal", "0-5", 0},
  {"roam", "0,1", 0}
};

class mozilla::dom::bluetooth::BluetoothHfpManagerObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  BluetoothHfpManagerObserver()
  {
  }

  bool Init()
  {
    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    MOZ_ASSERT(obs);
    if (NS_FAILED(obs->AddObserver(this, MOZSETTINGS_CHANGED_ID, false))) {
      NS_WARNING("Failed to add settings change observer!");
      return false;
    }

    if (NS_FAILED(obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false))) {
      NS_WARNING("Failed to add shutdown observer!");
      return false;
    }

    if (NS_FAILED(obs->AddObserver(this, MOBILE_CONNECTION_ICCINFO_CHANGED, false))) {
      NS_WARNING("Failed to add mobile connection iccinfo change observer!");
      return false;
    }

    if (NS_FAILED(obs->AddObserver(this, MOBILE_CONNECTION_VOICE_CHANGED, false))) {
      NS_WARNING("Failed to add mobile connection voice change observer!");
      return false;
    }

    return true;
  }

  bool Shutdown()
  {
    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    if (!obs ||
        NS_FAILED(obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) ||
        NS_FAILED(obs->RemoveObserver(this, MOZSETTINGS_CHANGED_ID)) ||
        NS_FAILED(obs->RemoveObserver(this, MOBILE_CONNECTION_ICCINFO_CHANGED)) ||
        NS_FAILED(obs->RemoveObserver(this, MOBILE_CONNECTION_VOICE_CHANGED))) {
      NS_WARNING("Can't unregister observers, or already unregistered!");
      return false;
    }
    return true;
  }

  ~BluetoothHfpManagerObserver()
  {
    Shutdown();
  }
};

class GetVolumeTask : public nsISettingsServiceCallback
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD
  Handle(const nsAString& aName, const jsval& aResult)
  {
    MOZ_ASSERT(NS_IsMainThread());

    JSContext *cx = nsContentUtils::GetCurrentJSContext();
    NS_ENSURE_TRUE(cx, NS_OK);

    if (!aResult.isNumber()) {
      NS_WARNING("'" AUDIO_VOLUME_BT_SCO "' is not a number!");
      return NS_OK;
    }

    BluetoothHfpManager* hfp = BluetoothHfpManager::Get();
    hfp->SetVolume(aResult.toNumber());

    return NS_OK;
  }

  NS_IMETHOD
  HandleError(const nsAString& aName)
  {
    NS_WARNING("Unable to get value for '" AUDIO_VOLUME_BT_SCO "'");
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(GetVolumeTask, nsISettingsServiceCallback);

NS_IMETHODIMP
BluetoothHfpManagerObserver::Observe(nsISupports* aSubject,
                                     const char* aTopic,
                                     const PRUnichar* aData)
{
  MOZ_ASSERT(gBluetoothHfpManager);

  if (!strcmp(aTopic, MOZSETTINGS_CHANGED_ID)) {
    return gBluetoothHfpManager->HandleVolumeChanged(nsDependentString(aData));
  } else if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    return gBluetoothHfpManager->HandleShutdown();
  } else if (!strcmp(aTopic, MOBILE_CONNECTION_ICCINFO_CHANGED)) {
    return gBluetoothHfpManager->HandleIccInfoChanged();
  } else if (!strcmp(aTopic, MOBILE_CONNECTION_VOICE_CHANGED)) {
    return gBluetoothHfpManager->HandleVoiceConnectionChanged();
  }

  MOZ_ASSERT(false, "BluetoothHfpManager got unexpected topic!");
  return NS_ERROR_UNEXPECTED;
}

NS_IMPL_ISUPPORTS1(BluetoothHfpManagerObserver, nsIObserver)

class SendRingIndicatorTask : public Task
{
public:
  SendRingIndicatorTask(const char* aNumber, int aType = TOA_UNKNOWN)
    : mNumber(aNumber)
    , mType(aType)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    NS_ENSURE_FALSE_VOID(sStopSendingRingFlag);

    if (!gBluetoothHfpManager) {
      NS_WARNING("BluetoothHfpManager no longer exists, cannot send ring!");
      return;
    }

    gBluetoothHfpManager->SendLine("RING");

    if (!mNumber.IsEmpty()) {
      nsAutoCString resultCode("+CLIP: \"");
      resultCode += mNumber;
      resultCode += "\",";
      resultCode.AppendInt(mType);

      gBluetoothHfpManager->SendLine(resultCode.get());
    }

    MessageLoop::current()->
      PostDelayedTask(FROM_HERE,
                      new SendRingIndicatorTask(mNumber.get(), mType),
                      sRingInterval);
  }

private:
  nsCString mNumber;
  int mType;
};

void
OpenScoSocket(const nsAString& aDeviceAddress)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothScoManager* sco = BluetoothScoManager::Get();
  if (!sco) {
    NS_WARNING("BluetoothScoManager is not available!");
    return;
  }

  if (!sco->Connect(aDeviceAddress)) {
    NS_WARNING("Failed to create a sco socket!");
  }
}

void
CloseScoSocket()
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothScoManager* sco = BluetoothScoManager::Get();
  if (!sco) {
    NS_WARNING("BluetoothScoManager is not available!");
    return;
  }
  sco->Disconnect();
}

bool
IsValidDtmf(const char aChar) {
  
  if (aChar == '*' || aChar == '#') {
    return true;
  } else if (aChar >= '0' && aChar <= '9') {
    return true;
  } else if (aChar >= 'A' && aChar <= 'D') {
    return true;
  }
  return false;
}

BluetoothHfpManager::BluetoothHfpManager()
  : mCurrentCallIndex(0)
  , mReceiveVgsFlag(false)
{
  sCINDItems[CINDType::CALL].value = CallState::NO_CALL;
  sCINDItems[CINDType::CALLSETUP].value = CallSetupState::NO_CALLSETUP;
  sCINDItems[CINDType::CALLHELD].value = CallHeldState::NO_CALLHELD;

  mCurrentCallStateArray.AppendElement((int)nsIRadioInterfaceLayer::CALL_STATE_DISCONNECTED);
}

bool
BluetoothHfpManager::Init()
{
  MOZ_ASSERT(NS_IsMainThread());

  mSocketStatus = GetConnectionStatus();

  sHfpObserver = new BluetoothHfpManagerObserver();
  if (!sHfpObserver->Init()) {
    NS_WARNING("Cannot set up Hfp Observers!");
  }

  mListener = new BluetoothRilListener();
  if (!mListener->StartListening()) {
    NS_WARNING("Failed to start listening RIL");
    return false;
  }

  nsCOMPtr<nsISettingsService> settings =
    do_GetService("@mozilla.org/settingsService;1");
  NS_ENSURE_TRUE(settings, false);

  nsCOMPtr<nsISettingsServiceLock> settingsLock;
  nsresult rv = settings->CreateLock(getter_AddRefs(settingsLock));
  NS_ENSURE_SUCCESS(rv, false);

  nsRefPtr<GetVolumeTask> callback = new GetVolumeTask();
  rv = settingsLock->Get(AUDIO_VOLUME_BT_SCO, callback);
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

BluetoothHfpManager::~BluetoothHfpManager()
{
  Cleanup();
}

void
BluetoothHfpManager::Cleanup()
{
  if (!mListener->StopListening()) {
    NS_WARNING("Failed to stop listening RIL");
  }
  mListener = nullptr;

  sHfpObserver->Shutdown();
  sHfpObserver = nullptr;
}


BluetoothHfpManager*
BluetoothHfpManager::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (gBluetoothHfpManager) {
    return gBluetoothHfpManager;
  }

  
  if (gInShutdown) {
    NS_WARNING("BluetoothHfpManager can't be created during shutdown");
    return nullptr;
  }

  
  nsRefPtr<BluetoothHfpManager> manager = new BluetoothHfpManager();
  NS_ENSURE_TRUE(manager, nullptr);

  if (!manager->Init()) {
    return nullptr;
  }

  gBluetoothHfpManager = manager;
  return gBluetoothHfpManager;
}

void
BluetoothHfpManager::SetVolume(const int aVolume)
{
  mCurrentVgs = aVolume;
}

void
BluetoothHfpManager::NotifySettings()
{
  nsString type, name;
  BluetoothValue v;
  InfallibleTArray<BluetoothNamedValue> parameters;
  type.AssignLiteral("bluetooth-hfp-status-changed");

  name.AssignLiteral("connected");
  v = (GetConnectionStatus() == SocketConnectionStatus::SOCKET_CONNECTED)
    ? true : false ;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("address");
  v = mDevicePath;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  if (!BroadcastSystemMessage(type, parameters)) {
    NS_WARNING("Failed to broadcast system message to settings");
    return;
  }
}

void
BluetoothHfpManager::NotifyDialer(const nsAString& aCommand)
{
  nsString type, name;
  BluetoothValue v;
  InfallibleTArray<BluetoothNamedValue> parameters;
  type.AssignLiteral("bluetooth-dialer-command");

  name.AssignLiteral("command");
  v = nsString(aCommand);
  parameters.AppendElement(BluetoothNamedValue(name, v));

  if (!BroadcastSystemMessage(type, parameters)) {
    NS_WARNING("Failed to broadcast system message to dialer");
    return;
  }
}

nsresult
BluetoothHfpManager::HandleVolumeChanged(const nsAString& aData)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  

  JSContext* cx = nsContentUtils::GetSafeJSContext();
  if (!cx) {
    NS_WARNING("Failed to get JSContext");
    return NS_OK;
  }

  JS::Value val;
  if (!JS_ParseJSON(cx, aData.BeginReading(), aData.Length(), &val)) {
    return JS_ReportPendingException(cx) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }

  if (!val.isObject()) {
    return NS_OK;
  }

  JSObject& obj(val.toObject());

  JS::Value key;
  if (!JS_GetProperty(cx, &obj, "key", &key)) {
    MOZ_ASSERT(!JS_IsExceptionPending(cx));
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!key.isString()) {
    return NS_OK;
  }

  JSBool match;
  if (!JS_StringEqualsAscii(cx, key.toString(), AUDIO_VOLUME_BT_SCO, &match)) {
    MOZ_ASSERT(!JS_IsExceptionPending(cx));
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!match) {
    return NS_OK;
  }

  JS::Value value;
  if (!JS_GetProperty(cx, &obj, "value", &value)) {
    MOZ_ASSERT(!JS_IsExceptionPending(cx));
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!value.isNumber()) {
    return NS_ERROR_UNEXPECTED;
  }

  mCurrentVgs = value.toNumber();

  
  if (mReceiveVgsFlag) {
    mReceiveVgsFlag = false;
    return NS_OK;
  }

  
  if (GetConnectionStatus() == SocketConnectionStatus::SOCKET_CONNECTED) {
    SendCommand("+VGS: ", mCurrentVgs);
  }

  return NS_OK;
}

nsresult
BluetoothHfpManager::HandleVoiceConnectionChanged()
{
  nsCOMPtr<nsIMobileConnectionProvider> connection =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(connection, NS_ERROR_FAILURE);

  nsIDOMMozMobileConnectionInfo* voiceInfo;
  connection->GetVoiceConnectionInfo(&voiceInfo);
  NS_ENSURE_TRUE(voiceInfo, NS_ERROR_FAILURE);

  bool roaming;
  voiceInfo->GetRoaming(&roaming);
  if (roaming != sCINDItems[CINDType::ROAM].value) {
    sCINDItems[CINDType::ROAM].value = roaming;
    SendCommand("+CIEV: ", CINDType::ROAM);
  }

  bool service = false;
  nsString regState;
  voiceInfo->GetState(regState);
  if (regState.EqualsLiteral("registered")) {
    service = true;
  }
  if (service != sCINDItems[CINDType::SERVICE].value) {
    sCINDItems[CINDType::SERVICE].value = service;
    SendCommand("+CIEV: ", CINDType::SERVICE);
  }

  uint8_t signal;
  JS::Value value;
  voiceInfo->GetRelSignalStrength(&value);
  if (!value.isNumber()) {
    NS_WARNING("Failed to get relSignalStrength in BluetoothHfpManager");
    return NS_ERROR_FAILURE;
  }
  signal = ceil(value.toNumber() / 20.0);

  if (signal != sCINDItems[CINDType::SIGNAL].value) {
    sCINDItems[CINDType::SIGNAL].value = signal;
    SendCommand("+CIEV: ", CINDType::SIGNAL);
  }

  return NS_OK;
}

nsresult
BluetoothHfpManager::HandleIccInfoChanged()
{
  nsCOMPtr<nsIMobileConnectionProvider> connection =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE(connection, NS_ERROR_FAILURE);

  nsIDOMMozMobileICCInfo* iccInfo;
  connection->GetIccInfo(&iccInfo);
  NS_ENSURE_TRUE(iccInfo, NS_ERROR_FAILURE);

  nsString msisdn;
  iccInfo->GetMsisdn(msisdn);

  if (!msisdn.Equals(mMsisdn)) {
    mMsisdn = msisdn;
  }

  return NS_OK;
}

nsresult
BluetoothHfpManager::HandleShutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  gInShutdown = true;
  CloseSocket();
  gBluetoothHfpManager = nullptr;
  return NS_OK;
}


void
BluetoothHfpManager::ReceiveSocketData(UnixSocketRawData* aMessage)
{
  MOZ_ASSERT(NS_IsMainThread());

  int currentCallState = mCurrentCallStateArray[mCurrentCallIndex];

  nsAutoCString msg((const char*)aMessage->mData.get());
  msg.StripWhitespace();

  nsTArray<nsCString> atCommandValues;

  
  
  if (msg.Find("AT+BRSF=") != -1) {
    SendCommand("+BRSF: ", 33);
  } else if (msg.Find("AT+CIND=?") != -1) {
    
    SendCommand("+CIND: ", 0);
  } else if (msg.Find("AT+CIND?") != -1) {
    
    SendCommand("+CIND: ", 1);
  } else if (msg.Find("AT+CMER=") != -1) {
    



    ParseAtCommand(msg, 8, atCommandValues);

    if (atCommandValues.Length() < 4) {
      NS_WARNING("Could't get the value of command [AT+CMER=]");
      goto respond_with_ok;
    }

    if (!atCommandValues[0].EqualsLiteral("3") ||
        !atCommandValues[1].EqualsLiteral("0") ||
        !atCommandValues[2].EqualsLiteral("0")) {
      NS_WARNING("Wrong value of CMER");
      goto respond_with_ok;
    }

    mCMER = atCommandValues[3].EqualsLiteral("1");
  } else if (msg.Find("AT+CMEE=") != -1) {
    ParseAtCommand(msg, 8, atCommandValues);

    if (atCommandValues.IsEmpty()) {
      NS_WARNING("Could't get the value of command [AT+CMEE=]");
      goto respond_with_ok;
    }

    
    
    
    mCMEE = !atCommandValues[0].EqualsLiteral("0");
  } else if (msg.Find("AT+VTS=") != -1) {
    ParseAtCommand(msg, 7, atCommandValues);
    if (atCommandValues.Length() != 1) {
      NS_WARNING("Couldn't get the value of command [AT+VTS=]");
      goto respond_with_ok;
    }

    if (IsValidDtmf(atCommandValues[0].get()[0])) {
      nsAutoCString message("VTS=");
      message += atCommandValues[0].get()[0];
      NotifyDialer(NS_ConvertUTF8toUTF16(message));
    }
  } else if (msg.Find("AT+VGM=") != -1) {
    ParseAtCommand(msg, 7, atCommandValues);

    if (atCommandValues.IsEmpty()) {
      NS_WARNING("Couldn't get the value of command [AT+VGM]");
      goto respond_with_ok;
    }

    nsresult rv;
    int vgm = atCommandValues[0].ToInteger(&rv);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to extract microphone volume from bluetooth headset!");
      goto respond_with_ok;
    }

    NS_ASSERTION(vgm >= 0 && vgm <= 15, "Received invalid VGM value");
    mCurrentVgm = vgm;
  } else if (msg.Find("AT+CHLD=?") != -1) {
    SendLine("+CHLD: (1,2)");
  } else if (msg.Find("AT+CHLD=") != -1) {
    ParseAtCommand(msg, 8, atCommandValues);

    if (atCommandValues.IsEmpty()) {
      NS_WARNING("Could't get the value of command [AT+VGS=]");
      goto respond_with_ok;
    }

    nsresult rv;
    int chld = atCommandValues[0].ToInteger(&rv);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to extract volume value from bluetooth headset!");
      goto respond_with_ok;
    }

    switch(chld) {
      case 1:
        
        NotifyDialer(NS_LITERAL_STRING("CHUP+ATA"));
        break;
      case 2:
        
        NotifyDialer(NS_LITERAL_STRING("CHLD+ATA"));
        break;
      default:
#ifdef DEBUG
        NS_WARNING("Not handling chld value");
#endif
        break;
    }
  } else if (msg.Find("AT+VGS=") != -1) {
    
    mReceiveVgsFlag = true;
    ParseAtCommand(msg, 7, atCommandValues);

    if (atCommandValues.IsEmpty()) {
      NS_WARNING("Could't get the value of command [AT+VGS=]");
      goto respond_with_ok;
    }

    nsresult rv;
    int newVgs = atCommandValues[0].ToInteger(&rv);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to extract volume value from bluetooth headset!");
      goto respond_with_ok;
    }

    if (newVgs == mCurrentVgs) {
      goto respond_with_ok;
    }

#ifdef DEBUG
    NS_ASSERTION(newVgs >= 0 && newVgs <= 15, "Received invalid VGS value");
#endif

    nsString data;
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    data.AppendInt(newVgs);
    os->NotifyObservers(nullptr, "bluetooth-volume-change", data.get());
  } else if (msg.Find("AT+BLDN") != -1) {
    NotifyDialer(NS_LITERAL_STRING("BLDN"));
  } else if (msg.Find("ATA") != -1) {
    NotifyDialer(NS_LITERAL_STRING("ATA"));
  } else if (msg.Find("AT+CHUP") != -1) {
    NotifyDialer(NS_LITERAL_STRING("CHUP"));
  } else if (msg.Find("ATD>") != -1) {
    
    SendLine("ERROR");
    return;
  } else if (msg.Find("ATD") != -1) {
    nsAutoCString message(msg), newMsg;
    int end = message.FindChar(';');
    if (end < 0) {
      NS_WARNING("Could't get the value of command [ATD]");
      goto respond_with_ok;
    }

    newMsg += nsDependentCSubstring(message, 0, end);
    NotifyDialer(NS_ConvertUTF8toUTF16(newMsg));
  } else if (msg.Find("AT+CLIP=") != -1) {
    ParseAtCommand(msg, 8, atCommandValues);

    if (atCommandValues.IsEmpty()) {
      NS_WARNING("Could't get the value of command [AT+CLIP=]");
      goto respond_with_ok;
    }

    mCLIP = (atCommandValues[0].EqualsLiteral("1"));
  } else if (msg.Find("AT+CKPD") != -1) {
    
    switch (currentCallState) {
      case nsIRadioInterfaceLayer::CALL_STATE_INCOMING:
        NotifyDialer(NS_LITERAL_STRING("ATA"));
        break;
      case nsIRadioInterfaceLayer::CALL_STATE_CONNECTED:
      case nsIRadioInterfaceLayer::CALL_STATE_DIALING:
      case nsIRadioInterfaceLayer::CALL_STATE_ALERTING:
        NotifyDialer(NS_LITERAL_STRING("CHUP"));
        break;
      case nsIRadioInterfaceLayer::CALL_STATE_DISCONNECTED:
        NotifyDialer(NS_LITERAL_STRING("BLDN"));
        break;
      default:
#ifdef DEBUG
        NS_WARNING("Not handling state changed");
#endif
        break;
    }
  } else if (msg.Find("AT+CNUM") != -1) {
    if (!mMsisdn.IsEmpty()) {
      nsAutoCString message("+CNUM: ,\"");
      message += NS_ConvertUTF16toUTF8(mMsisdn).get();
      message += "\",";
      message.AppendInt(TOA_UNKNOWN);
      message += ",,4";
      SendLine(message.get());
    }
  } else {
#ifdef DEBUG
    nsCString warningMsg;
    warningMsg.AssignLiteral("Not handling HFP message, reply ok: ");
    warningMsg.Append(msg);
    NS_WARNING(warningMsg.get());
#endif
  }

respond_with_ok:
  
  SendLine("OK");
}

bool
BluetoothHfpManager::Connect(const nsAString& aDevicePath,
                             const bool aIsHandsfree,
                             BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (gInShutdown) {
    NS_WARNING("Connect called while in shutdown!");
    return false;
  }

  if (GetConnectionStatus() == SocketConnectionStatus::SOCKET_CONNECTED ||
      GetConnectionStatus() == SocketConnectionStatus::SOCKET_CONNECTING) {
    NS_WARNING("BluetoothHfpManager has connected/is connecting to a headset!");
    return false;
  }

  CloseSocket();

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE(bs, false);

  nsString uuid;
  if (aIsHandsfree) {
    BluetoothUuidHelper::GetString(BluetoothServiceClass::HANDSFREE, uuid);
  } else {
    BluetoothUuidHelper::GetString(BluetoothServiceClass::HEADSET, uuid);
  }

  mRunnable = aRunnable;

  nsresult rv = bs->GetSocketViaService(aDevicePath,
                                        uuid,
                                        BluetoothSocketType::RFCOMM,
                                        true,
                                        true,
                                        this,
                                        mRunnable);

  return NS_FAILED(rv) ? false : true;
}

bool
BluetoothHfpManager::Listen()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (gInShutdown) {
    MOZ_ASSERT(false, "Listen called while in shutdown!");
    return false;
  }

  if (GetConnectionStatus() == SocketConnectionStatus::SOCKET_LISTENING) {
    NS_WARNING("BluetoothHfpManager has been already listening");
    return true;
  }

  CloseSocket();

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE(bs, false);

  nsresult rv =
    bs->ListenSocketViaService(BluetoothReservedChannels::CHANNEL_HANDSFREE_AG,
                               BluetoothSocketType::RFCOMM,
                               true,
                               true,
                               this);

  mSocketStatus = GetConnectionStatus();

  return NS_FAILED(rv) ? false : true;
}

void
BluetoothHfpManager::Disconnect()
{
  if (GetConnectionStatus() == SocketConnectionStatus::SOCKET_DISCONNECTED) {
    NS_WARNING("BluetoothHfpManager has been disconnected!");
    return;
  }

  CloseScoSocket();
  CloseSocket();
}

bool
BluetoothHfpManager::SendLine(const char* aMessage)
{
  const char* kHfpCrlf = "\xd\xa";
  nsAutoCString msg;

  msg += kHfpCrlf;
  msg += aMessage;
  msg += kHfpCrlf;

  return SendSocketData(msg);
}

bool
BluetoothHfpManager::SendCommand(const char* aCommand, const int aValue)
{
  if (mSocketStatus != SocketConnectionStatus::SOCKET_CONNECTED) {
    return false;
  }

  nsAutoCString message;
  int value = aValue;
  message += aCommand;

  if (!strcmp(aCommand, "+CIEV: ")) {
    if (!mCMER) {
      
      return true;
    }

    if ((aValue < 1) || (aValue > ArrayLength(sCINDItems) - 1)) {
      NS_WARNING("unexpected CINDType for CIEV command");
      return false;
    }

    message.AppendInt(aValue);
    message += ",";
    message.AppendInt(sCINDItems[aValue].value);
  } else if (!strcmp(aCommand, "+CIND: ")) {
    if (!aValue) {
      for (uint8_t i = 1; i < ArrayLength(sCINDItems); i++) {
        message += "(\"";
        message += sCINDItems[i].name;
        message += "\",(";
        message += sCINDItems[i].range;
        message += ")";
        if (i == (ArrayLength(sCINDItems) - 1)) {
          message +=")";
          break;
        }
        message += "),";
      }
    } else {
      for (uint8_t i = 1; i < ArrayLength(sCINDItems); i++) {
        message.AppendInt(sCINDItems[i].value);
        if (i == (ArrayLength(sCINDItems) - 1)) {
          break;
        }
        message += ",";
      }
    }
  } else {
    message.AppendInt(value);
  }

  return SendLine(message.get());
}

void
BluetoothHfpManager::SetupCIND(int aCallIndex, int aCallState,
                               const char* aNumber, bool aInitial)
{
  nsRefPtr<nsRunnable> sendRingTask;
  nsString address;

  while (aCallIndex >= (int)mCurrentCallStateArray.Length()) {
    mCurrentCallStateArray.AppendElement((int)nsIRadioInterfaceLayer::CALL_STATE_DISCONNECTED);
  }

  int currentCallState = mCurrentCallStateArray[aCallIndex];

  switch (aCallState) {
    case nsIRadioInterfaceLayer::CALL_STATE_INCOMING:
      sCINDItems[CINDType::CALLSETUP].value = CallSetupState::INCOMING;
      if (!aInitial) {
        SendCommand("+CIEV: ", CINDType::CALLSETUP);
      }

      if (!mCurrentCallIndex) {
        
        sStopSendingRingFlag = false;

        if (!mCLIP) {
          MessageLoop::current()->
            PostDelayedTask(FROM_HERE,
                            new SendRingIndicatorTask(""),
                            sRingInterval);
        } else {
          
          int type = TOA_UNKNOWN;

          if (aNumber && strlen(aNumber) > 0 && aNumber[0] == '+') {
            type = TOA_INTERNATIONAL;
          }

          MessageLoop::current()->
            PostDelayedTask(FROM_HERE,
                            new SendRingIndicatorTask(aNumber, type),
                            sRingInterval);
        }
      }
      break;
    case nsIRadioInterfaceLayer::CALL_STATE_DIALING:
      sCINDItems[CINDType::CALLSETUP].value = CallSetupState::OUTGOING;
      if (!aInitial) {
        SendCommand("+CIEV: ", CINDType::CALLSETUP);

        GetSocketAddr(address);
        OpenScoSocket(address);
      }
      break;
    case nsIRadioInterfaceLayer::CALL_STATE_ALERTING:
      sCINDItems[CINDType::CALLSETUP].value = CallSetupState::OUTGOING_ALERTING;
      if (!aInitial) {
        SendCommand("+CIEV: ", CINDType::CALLSETUP);
      }
      break;
    case nsIRadioInterfaceLayer::CALL_STATE_CONNECTED:
      mCurrentCallIndex = aCallIndex;
      if (aInitial) {
        sCINDItems[CINDType::CALL].value = CallState::IN_PROGRESS;
        sCINDItems[CINDType::CALLSETUP].value = CallSetupState::NO_CALLSETUP;
      } else {
        switch (currentCallState) {
          case nsIRadioInterfaceLayer::CALL_STATE_INCOMING:
            
            sStopSendingRingFlag = true;

            GetSocketAddr(address);
            OpenScoSocket(address);
          case nsIRadioInterfaceLayer::CALL_STATE_DISCONNECTED:
          case nsIRadioInterfaceLayer::CALL_STATE_ALERTING:
            
            sCINDItems[CINDType::CALL].value = CallState::IN_PROGRESS;
            SendCommand("+CIEV: ", CINDType::CALL);
            sCINDItems[CINDType::CALLSETUP].value = CallSetupState::NO_CALLSETUP;
            SendCommand("+CIEV: ", CINDType::CALLSETUP);
            break;
          default:
#ifdef DEBUG
            NS_WARNING("Not handling state changed");
#endif
            break;
        }
      }
      break;
    case nsIRadioInterfaceLayer::CALL_STATE_DISCONNECTED:
      if (!aInitial) {
        switch (currentCallState) {
          case nsIRadioInterfaceLayer::CALL_STATE_INCOMING:
          case nsIRadioInterfaceLayer::CALL_STATE_BUSY:
            
            sStopSendingRingFlag = true;
          case nsIRadioInterfaceLayer::CALL_STATE_DIALING:
          case nsIRadioInterfaceLayer::CALL_STATE_ALERTING:
            
            sCINDItems[CINDType::CALLSETUP].value = CallSetupState::NO_CALLSETUP;
            SendCommand("+CIEV: ", CINDType::CALLSETUP);
            break;
          case nsIRadioInterfaceLayer::CALL_STATE_CONNECTED:
            sCINDItems[CINDType::CALL].value = CallState::NO_CALL;
            SendCommand("+CIEV: ", CINDType::CALL);
            break;
          case nsIRadioInterfaceLayer::CALL_STATE_HELD:
            sCINDItems[CINDType::CALLHELD].value = NO_CALLHELD;
            SendCommand("+CIEV: ", CINDType::CALLHELD);
          default:
#ifdef DEBUG
            NS_WARNING("Not handling state changed");
#endif
            break;
        }

        if (aCallIndex == mCurrentCallIndex) {
#ifdef DEBUG
          NS_ASSERTION(mCurrentCallStateArray.Length() > aCallIndex,
            "Call index out of bounds!");
#endif
          mCurrentCallStateArray[aCallIndex] = aCallState;

          
          
          int c;
          for (c = 1; c < (int)mCurrentCallStateArray.Length(); c++) {
            if (mCurrentCallStateArray[c] != nsIRadioInterfaceLayer::CALL_STATE_DISCONNECTED) {
              mCurrentCallIndex = c;
              break;
            }
          }

          
          if (c == (int)mCurrentCallStateArray.Length()) {
            mCurrentCallIndex = 0;
            CloseScoSocket();
          }
        }
      }
      break;
    case nsIRadioInterfaceLayer::CALL_STATE_HELD:
      sCINDItems[CINDType::CALLHELD].value = CallHeldState::ONHOLD_ACTIVE;

      if (!aInitial) {
        SendCommand("+CIEV: ", CINDType::CALLHELD);
      }

      break;
    default:
#ifdef DEBUG
      NS_WARNING("Not handling state changed");
      sCINDItems[CINDType::CALL].value = CallState::NO_CALL;
      sCINDItems[CINDType::CALLSETUP].value = CallSetupState::NO_CALLSETUP;
      sCINDItems[CINDType::CALLHELD].value = CallHeldState::NO_CALLHELD;
#endif
      break;
  }

  mCurrentCallStateArray[aCallIndex] = aCallState;
}




void
BluetoothHfpManager::EnumerateCallState(int aCallIndex, int aCallState,
                                        const char* aNumber, bool aIsActive)
{
  SetupCIND(aCallIndex, aCallState, aNumber, true);

  if (sCINDItems[CINDType::CALL].value == CallState::IN_PROGRESS ||
      sCINDItems[CINDType::CALLSETUP].value == CallSetupState::OUTGOING ||
      sCINDItems[CINDType::CALLSETUP].value == CallSetupState::OUTGOING_ALERTING) {
    nsString address;
    GetSocketAddr(address);
    OpenScoSocket(address);
  }
}






void
BluetoothHfpManager::CallStateChanged(int aCallIndex, int aCallState,
                                      const char* aNumber, bool aIsActive)
{
  if (GetConnectionStatus() != SocketConnectionStatus::SOCKET_CONNECTED) {
    return;
  }

  SetupCIND(aCallIndex, aCallState, aNumber, false);
}

void
BluetoothHfpManager::OnConnectSuccess()
{
  
  if (mRunnable) {
    BluetoothValue v = true;
    nsString errorStr;
    DispatchBluetoothReply(mRunnable, v, errorStr);

    mRunnable.forget();
  }

  
  
  GetSocketAddr(mDevicePath);
  mSocketStatus = GetConnectionStatus();

  nsCOMPtr<nsIRILContentHelper> ril =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_TRUE_VOID(ril);
  ril->EnumerateCalls(mListener->GetCallback());

  NotifySettings();
}

void
BluetoothHfpManager::OnConnectError()
{
  
  if (mRunnable) {
    BluetoothValue v;
    nsString errorStr;
    errorStr.AssignLiteral("Failed to connect with a bluetooth headset!");
    DispatchBluetoothReply(mRunnable, v, errorStr);

    mRunnable.forget();
  }

  
  CloseSocket();
  mSocketStatus = GetConnectionStatus();
  Listen();
}

void
BluetoothHfpManager::OnDisconnect()
{
  
  
  if (mSocketStatus == SocketConnectionStatus::SOCKET_CONNECTED) {
    Listen();
    NotifySettings();
  } else if (mSocketStatus == SocketConnectionStatus::SOCKET_CONNECTING) {
    NS_WARNING("BluetoothHfpManager got unexpected socket status!");
  }

  sStopSendingRingFlag = true;
  sCINDItems[CINDType::CALL].value = CallState::NO_CALL;
  sCINDItems[CINDType::CALLSETUP].value = CallSetupState::NO_CALLSETUP;
  sCINDItems[CINDType::CALLHELD].value = CallHeldState::NO_CALLHELD;
  mCLIP = false;
  mCMEE = false;
  mCMER = false;
}

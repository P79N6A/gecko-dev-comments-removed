





#include "base/basictypes.h" 

#include "BluetoothHfpManager.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothScoManager.h"
#include "BluetoothService.h"
#include "BluetoothServiceUuid.h"
#include "BluetoothUtils.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "nsContentUtils.h"
#include "nsIAudioManager.h"
#include "nsIObserverService.h"
#include "nsIRadioInterfaceLayer.h"

#include <unistd.h> 

#define MOZSETTINGS_CHANGED_ID "mozsettings-changed"
#define AUDIO_VOLUME_MASTER "audio.volume.bt_sco"
#define HANDSFREE_UUID mozilla::dom::bluetooth::BluetoothServiceUuidStr::Handsfree
#define HEADSET_UUID mozilla::dom::bluetooth::BluetoothServiceUuidStr::Headset

using namespace mozilla;
using namespace mozilla::ipc;
USING_BLUETOOTH_NAMESPACE





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
  SIGNAL,
  SERVICE,
  CALL,
  CALLSETUP,
  CALLHELD,
  ROAM,
};

static CINDItem sCINDItems[] = {
  {},
  {"battchg", "0-5", 5},
  {"signal", "0-5", 5},
  {"service", "0,1", 1},
  {"call", "0,1", CallState::NO_CALL},
  {"callsetup", "0-3", CallSetupState::NO_CALLSETUP},
  {"callheld", "0-2", CallHeldState::NO_CALLHELD},
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

    return true;
  }

  bool Shutdown()
  {
    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    if (!obs ||
        (NS_FAILED(obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) ||
         NS_FAILED(obs->RemoveObserver(this, MOZSETTINGS_CHANGED_ID)))) {
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

namespace {
  StaticRefPtr<BluetoothHfpManager> gBluetoothHfpManager;
  StaticRefPtr<BluetoothHfpManagerObserver> sHfpObserver;
  bool gInShutdown = false;
  static bool sStopSendingRingFlag = true;

  static int sRingInterval = 3000; 
} 

NS_IMPL_ISUPPORTS1(BluetoothHfpManagerObserver, nsIObserver)

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
  }

  MOZ_ASSERT(false, "BluetoothHfpManager got unexpected topic!");
  return NS_ERROR_UNEXPECTED;
}

class SendRingIndicatorTask : public Task
{
public:
  SendRingIndicatorTask()
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  void Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (sStopSendingRingFlag) {
      return;
    }

    if (!gBluetoothHfpManager) {
      NS_WARNING("BluetoothHfpManager no longer exists, cannot send ring!");
      return;
    }

    gBluetoothHfpManager->SendLine("RING");

    MessageLoop::current()->
      PostDelayedTask(FROM_HERE,
                      new SendRingIndicatorTask(),
                      sRingInterval);

    return;
  }
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

  float volume;
  nsCOMPtr<nsIAudioManager> am = do_GetService("@mozilla.org/telephony/audiomanager;1");
  if (!am) {
    NS_WARNING("Failed to get AudioManager Service!");
    return false;
  }
  am->GetMasterVolume(&volume);

  
  
  mCurrentVgs = floor(volume * 15);

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
BluetoothHfpManager::NotifySettings()
{
  nsString type, name;
  BluetoothValue v;
  InfallibleTArray<BluetoothNamedValue> parameters;
  type.AssignLiteral("bluetooth-hfp-status-changed");

  name.AssignLiteral("connected");
  if (GetConnectionStatus() == SocketConnectionStatus::SOCKET_CONNECTED) {
    v = true;
  } else {
    v = false;
  }
  parameters.AppendElement(BluetoothNamedValue(name, v));

  name.AssignLiteral("address");
  v = mDevicePath;
  parameters.AppendElement(BluetoothNamedValue(name, v));

  if (!BroadcastSystemMessage(type, parameters)) {
    NS_WARNING("Failed to broadcast system message to dialer");
    return;
  }
}

void
BluetoothHfpManager::NotifyDialer(const nsAString& aCommand)
{
  nsString type, name, command;
  command = aCommand;
  InfallibleTArray<BluetoothNamedValue> parameters;
  type.AssignLiteral("bluetooth-dialer-command");

  BluetoothValue v(command);
  parameters.AppendElement(BluetoothNamedValue(type, v));

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
  if (!JS_StringEqualsAscii(cx, key.toString(), AUDIO_VOLUME_MASTER, &match)) {
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

  
  
  float volume = value.toNumber();
  mCurrentVgs = floor(volume * 15);

  if (mReceiveVgsFlag) {
    mReceiveVgsFlag = false;
    return NS_OK;
  }

  if (GetConnectionStatus() != SocketConnectionStatus::SOCKET_CONNECTED) {
    return NS_OK;
  }

  SendCommand("+VGS: ", mCurrentVgs);

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

  const char* msg = (const char*)aMessage->mData;
  int currentCallState = mCurrentCallStateArray[mCurrentCallIndex];

  
  
  if (!strncmp(msg, "AT+BRSF=", 8)) {
    SendCommand("+BRSF: ", 23);
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CIND=?", 9)) {
    
    SendCommand("+CIND: ", 0);
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CIND?", 8)) {
    
    SendCommand("+CIND: ", 1);
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CMER=", 8)) {
    
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CHLD=?", 9)) {
    SendLine("+CHLD: (1,2)");
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CHLD=", 8)) {
    int length = strlen(msg) - 9;
    nsAutoCString chldString(nsDependentCSubstring(msg+8, length));

    nsresult rv;
    int chld = chldString.ToInteger(&rv);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to extract volume value from bluetooth headset!");
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
    SendLine("OK");
  } else if (!strncmp(msg, "AT+VGS=", 7)) {
    mReceiveVgsFlag = true;

    int length = strlen(msg) - 8;
    nsAutoCString vgsString(nsDependentCSubstring(msg+7, length));

    nsresult rv;
    int newVgs = vgsString.ToInteger(&rv);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to extract volume value from bluetooth headset!");
    }

    if (newVgs == mCurrentVgs) {
      SendLine("OK");
      return;
    }

#ifdef DEBUG
    NS_ASSERTION(newVgs >= 0 && newVgs <= 15, "Received invalid VGS value");
#endif

    
    
    nsString data;
    int volume;
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    volume = ceil((float)newVgs / 15.0 * 10.0);
    data.AppendInt(volume);
    os->NotifyObservers(nullptr, "bluetooth-volume-change", data.get());

    SendLine("OK");
  } else if (!strncmp(msg, "AT+BLDN", 7)) {
    NotifyDialer(NS_LITERAL_STRING("BLDN"));
    SendLine("OK");
  } else if (!strncmp(msg, "ATA", 3)) {
    NotifyDialer(NS_LITERAL_STRING("ATA"));
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CHUP", 7)) {
    NotifyDialer(NS_LITERAL_STRING("CHUP"));
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CKPD", 7)) {
    
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
    SendLine("OK");
  } else {
#ifdef DEBUG
    nsCString warningMsg;
    warningMsg.AssignLiteral("Not handling HFP message, reply ok: ");
    warningMsg.Append(msg);
    NS_WARNING(warningMsg.get());
#endif
    SendLine("OK");
  }
}

bool
BluetoothHfpManager::Connect(const nsAString& aDevicePath,
                             const bool aIsHandsfree,
                             BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (gInShutdown) {
    MOZ_ASSERT(false, "Connect called while in shutdown!");
    return false;
  }

  if (GetConnectionStatus() == SocketConnectionStatus::SOCKET_CONNECTED) {
    NS_WARNING("BluetoothHfpManager has connected to a headset/handsfree!");
    return false;
  }

  CloseSocket();

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return false;
  }

  nsString serviceUuidStr;
  if (aIsHandsfree) {
    serviceUuidStr = NS_ConvertUTF8toUTF16(HANDSFREE_UUID);
  } else {
    serviceUuidStr = NS_ConvertUTF8toUTF16(HEADSET_UUID);
  }

  mRunnable = aRunnable;

  nsresult rv = bs->GetSocketViaService(aDevicePath,
                                        serviceUuidStr,
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

  CloseSocket();

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return false;
  }

  nsresult rv = bs->ListenSocketViaService(BluetoothReservedChannels::HANDSFREE_AG,
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
    NS_WARNING("BluetoothHfpManager has disconnected!");
    return;
  }

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
  nsAutoCString message;
  int value = aValue;
  message += aCommand;

  if (!strcmp(aCommand, "+CIEV: ")) {
    message.AppendInt(aValue);
    message += ",";
    switch (aValue) {
      case CINDType::CALL:
        message.AppendInt(sCINDItems[CINDType::CALL].value);
        break;
      case CINDType::CALLSETUP:
        message.AppendInt(sCINDItems[CINDType::CALLSETUP].value);
        break;
      case CINDType::CALLHELD:
        message.AppendInt(sCINDItems[CINDType::CALLHELD].value);
        break;
      default:
#ifdef DEBUG
        NS_WARNING("unexpected CINDType for CIEV command");
#endif
        return false;
    }
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
BluetoothHfpManager::SetupCIND(int aCallIndex, int aCallState, bool aInitial)
{
  nsRefPtr<nsRunnable> sendRingTask;
  nsString address;

  while (aCallIndex >= mCurrentCallStateArray.Length()) {
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
        MessageLoop::current()->PostTask(FROM_HERE,
                                         new SendRingIndicatorTask());
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
          for (c = 1; c < mCurrentCallStateArray.Length(); c++) {
            if (mCurrentCallStateArray[c] != nsIRadioInterfaceLayer::CALL_STATE_DISCONNECTED) {
              mCurrentCallIndex = c;
              break;
            }
          }

          
          if (c == mCurrentCallStateArray.Length()) {
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
  SetupCIND(aCallIndex, aCallState, true);

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

  SetupCIND(aCallIndex, aCallState, false);
}

void
BluetoothHfpManager::OnConnectSuccess()
{
  if (mRunnable) {
    BluetoothReply* reply = new BluetoothReply(BluetoothReplySuccess(true));
    mRunnable->SetReply(reply);
    if (NS_FAILED(NS_DispatchToMainThread(mRunnable))) {
      NS_WARNING("Failed to dispatch to main thread!");
    }
    mRunnable.forget();
  }
  
  
  GetSocketAddr(mDevicePath);
  mSocketStatus = GetConnectionStatus();

  nsCOMPtr<nsIRILContentHelper> ril =
    do_GetService("@mozilla.org/ril/content-helper;1");
  if (!ril) {
    MOZ_ASSERT("Failed to get RIL Content Helper");
  }
  ril->EnumerateCalls(mListener->GetCallback());

  NotifySettings();
}

void
BluetoothHfpManager::OnConnectError()
{
  if (mRunnable) {
    nsString errorStr;
    errorStr.AssignLiteral("Failed to connect with a bluetooth headset!");
    BluetoothReply* reply = new BluetoothReply(BluetoothReplyError(errorStr));
    mRunnable->SetReply(reply);
    if (NS_FAILED(NS_DispatchToMainThread(mRunnable))) {
      NS_WARNING("Failed to dispatch to main thread!");
    }
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
  }

  sCINDItems[CINDType::CALL].value = CallState::NO_CALL;
  sCINDItems[CINDType::CALLSETUP].value = CallSetupState::NO_CALLSETUP;
  sCINDItems[CINDType::CALLHELD].value = CallHeldState::NO_CALLHELD;
}

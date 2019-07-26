





#include "base/basictypes.h"
#include "BluetoothHfpManager.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothServiceUuid.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/Services.h"
#include "nsContentUtils.h"
#include "nsIObserverService.h"
#include "nsIRadioInterfaceLayer.h"
#include "nsISystemMessagesInternal.h"
#include "nsVariant.h"

#include <unistd.h> 

#define MOZSETTINGS_CHANGED_ID "mozsettings-changed"
#define AUDIO_VOLUME_MASTER "audio.volume.master"

USING_BLUETOOTH_NAMESPACE
using namespace mozilla::ipc;

static nsRefPtr<BluetoothHfpManager> sInstance = nullptr;
static nsCOMPtr<nsIThread> sHfpCommandThread;
static bool sStopSendingRingFlag = true;

static int kRingInterval = 3000000;  

NS_IMPL_ISUPPORTS1(BluetoothHfpManager, nsIObserver)

class SendRingIndicatorTask : public nsRunnable
{
public:
  SendRingIndicatorTask()
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    while (!sStopSendingRingFlag) {
      sInstance->SendLine("RING");

      usleep(kRingInterval);
    }

    return NS_OK;
  }
};

BluetoothHfpManager::BluetoothHfpManager()
  : mCurrentVgs(-1)
  , mCurrentCallIndex(0)
  , mCurrentCallState(nsIRadioInterfaceLayer::CALL_STATE_DISCONNECTED)
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();

  if (obs && NS_FAILED(obs->AddObserver(sInstance, MOZSETTINGS_CHANGED_ID, false))) {
    NS_WARNING("Failed to add settings change observer!");
  }

  mListener = new BluetoothRilListener();
  if (!mListener->StartListening()) {
    NS_WARNING("Failed to start listening RIL");
  }

  if (!sHfpCommandThread) {
    if (NS_FAILED(NS_NewThread(getter_AddRefs(sHfpCommandThread)))) {
      NS_ERROR("Failed to new thread for sHfpCommandThread");
    }
  }
}

BluetoothHfpManager::~BluetoothHfpManager()
{
  if (!mListener->StopListening()) {
    NS_WARNING("Failed to stop listening RIL");
  }
  mListener = nullptr;

  
  if (sHfpCommandThread) {
    nsCOMPtr<nsIThread> thread;
    sHfpCommandThread.swap(thread);
    if (NS_FAILED(thread->Shutdown())) {
      NS_WARNING("Failed to shut down the bluetooth hfpmanager command thread!");
    }
  }
}


BluetoothHfpManager*
BluetoothHfpManager::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (sInstance == nullptr) {
    sInstance = new BluetoothHfpManager();
  }

  return sInstance;
}

bool
BluetoothHfpManager::BroadcastSystemMessage(const char* aCommand,
                                            const int aCommandLength)
{
  nsString type;
  type.AssignLiteral("bluetooth-dialer-command");

  JSContext* cx = nsContentUtils::GetSafeJSContext();
  NS_ASSERTION(!::JS_IsExceptionPending(cx),
               "Shouldn't get here when an exception is pending!");

  JSAutoRequest jsar(cx);
  JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
  if (!obj) {
    NS_WARNING("Failed to new JSObject for system message!");
    return false;
  }

  JSString* JsData = JS_NewStringCopyN(cx, aCommand, aCommandLength);
  if (!JsData) {
    NS_WARNING("JS_NewStringCopyN is out of memory");
    return false;
  }

  jsval v = STRING_TO_JSVAL(JsData);
  if (!JS_SetProperty(cx, obj, "command", &v)) {
    NS_WARNING("Failed to set properties of system message!");
    return false;
  }

  nsCOMPtr<nsISystemMessagesInternal> systemMessenger =
    do_GetService("@mozilla.org/system-message-internal;1");

  if (!systemMessenger) {
    NS_WARNING("Failed to get SystemMessenger service!");
    return false;
  }

  systemMessenger->BroadcastMessage(type, OBJECT_TO_JSVAL(obj));

  return true;
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

  
  mCurrentVgs = ceil(volume * 15);

  nsDiscriminatedUnion du;
  du.mType = 0;
  du.u.mInt8Value = mCurrentVgs;

  nsCString vgs;
  if (NS_FAILED(nsVariant::ConvertToACString(du, vgs))) {
    NS_WARNING("Failed to convert volume to string");
    return NS_ERROR_FAILURE;
  }

  nsAutoCString newVgs;
  newVgs += "+VGS: ";
  newVgs += vgs;

  SendLine(newVgs.get());

  return NS_OK;
}

nsresult
BluetoothHfpManager::Observe(nsISupports* aSubject,
                             const char* aTopic,
                             const PRUnichar* aData)
{
  if (!strcmp(aTopic, MOZSETTINGS_CHANGED_ID)) {
    return HandleVolumeChanged(nsDependentString(aData));
  } else {
    MOZ_ASSERT(false, "BluetoothHfpManager got unexpected topic!");
  }
  return NS_ERROR_UNEXPECTED;
}


void
BluetoothHfpManager::ReceiveSocketData(UnixSocketRawData* aMessage)
{
  MOZ_ASSERT(NS_IsMainThread());

  const char* msg = (const char*)aMessage->mData;

  
  
  if (!strncmp(msg, "AT+BRSF=", 8)) {
    SendLine("+BRSF: 23");
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CIND=?", 9)) {
    nsAutoCString cindRange;

    cindRange += "+CIND: ";
    cindRange += "(\"battchg\",(0-5)),";
    cindRange += "(\"signal\",(0-5)),";
    cindRange += "(\"service\",(0,1)),";
    cindRange += "(\"call\",(0,1)),";
    cindRange += "(\"callsetup\",(0-3)),";
    cindRange += "(\"callheld\",(0-2)),";
    cindRange += "(\"roam\",(0,1))";

    SendLine(cindRange.get());
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CIND", 7)) {
    
    
    
    
    
    SendLine("+CIND: 5,5,1,0,0,0,0");
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CMER=", 8)) {
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CHLD=?", 9)) {
    SendLine("+CHLD: (0,1,2,3)");
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CHLD=", 8)) {
    SendLine("OK");
  } else if (!strncmp(msg, "AT+VGS=", 7)) {
    
    int newVgs = msg[7] - '0';

    if (strlen(msg) > 8) {
      newVgs *= 10;
      newVgs += (msg[8] - '0');
    }

#ifdef DEBUG
    NS_ASSERTION(newVgs >= 0 && newVgs <= 15, "Received invalid VGS value");
#endif

    
    
    
    
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (newVgs > mCurrentVgs) {
      os->NotifyObservers(nullptr, "bluetooth-volume-change", NS_LITERAL_STRING("up").get());
    } else if (newVgs < mCurrentVgs) {
      os->NotifyObservers(nullptr, "bluetooth-volume-change", NS_LITERAL_STRING("down").get());
    }

    mCurrentVgs = newVgs;

    SendLine("OK");
  } else if (!strncmp(msg, "AT+BLDN", 7)) {
    if (!BroadcastSystemMessage("BLDN", 4)) {
      NS_WARNING("Failed to broadcast system message to dialer");
      return;
    }
    SendLine("OK");
  } else if (!strncmp(msg, "ATA", 3)) {
    if (!BroadcastSystemMessage("ATA", 3)) {
      NS_WARNING("Failed to broadcast system message to dialer");
      return;
    }
    SendLine("OK");
  } else if (!strncmp(msg, "AT+CHUP", 7)) {
    if (!BroadcastSystemMessage("CHUP", 4)) {
      NS_WARNING("Failed to broadcast system message to dialer");
      return;
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
BluetoothHfpManager::Connect(const nsAString& aDeviceObjectPath,
                             BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return false;
  }

  nsString serviceUuidStr =
    NS_ConvertUTF8toUTF16(mozilla::dom::bluetooth::BluetoothServiceUuidStr::Handsfree);

  nsRefPtr<BluetoothReplyRunnable> runnable = aRunnable;

  nsresult rv = bs->GetSocketViaService(aDeviceObjectPath,
                                        serviceUuidStr,
                                        BluetoothSocketType::RFCOMM,
                                        true,
                                        false,
                                        this,
                                        runnable);

  runnable.forget();
  return NS_FAILED(rv) ? false : true;
}

void
BluetoothHfpManager::Disconnect()
{
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






void
BluetoothHfpManager::CallStateChanged(int aCallIndex, int aCallState,
                                      const char* aNumber, bool aIsActive)
{
  nsRefPtr<nsRunnable> sendRingTask;

  switch (aCallState) {
    case nsIRadioInterfaceLayer::CALL_STATE_INCOMING:
      
      SendLine("+CIEV: 5,1");

      
      sStopSendingRingFlag = false;
      sendRingTask = new SendRingIndicatorTask();

      if (NS_FAILED(sHfpCommandThread->Dispatch(sendRingTask, NS_DISPATCH_NORMAL))) {
        NS_WARNING("Cannot dispatch ring task!");
        return;
      };
      break;
    case nsIRadioInterfaceLayer::CALL_STATE_DIALING:
      
      SendLine("+CIEV: 5,2");
      break;
    case nsIRadioInterfaceLayer::CALL_STATE_ALERTING:
      
      if (mCurrentCallIndex == nsIRadioInterfaceLayer::CALL_STATE_DIALING) {
        SendLine("+CIEV: 5,3");
      } else {
#ifdef DEBUG
        NS_WARNING("Not handling state changed");
#endif
      }
      break;
    case nsIRadioInterfaceLayer::CALL_STATE_CONNECTED:
      switch (mCurrentCallState) {
        case nsIRadioInterfaceLayer::CALL_STATE_INCOMING:
          sStopSendingRingFlag = true;
          
        case nsIRadioInterfaceLayer::CALL_STATE_DIALING:
          
          SendLine("+CIEV: 4,1");
          SendLine("+CIEV: 5,0");
          break;
        default:
#ifdef DEBUG
          NS_WARNING("Not handling state changed");
#endif
          break;
      }

      break;
    case nsIRadioInterfaceLayer::CALL_STATE_DISCONNECTED:
      switch (mCurrentCallState) {
        case nsIRadioInterfaceLayer::CALL_STATE_INCOMING:
          sStopSendingRingFlag = true;
          
        case nsIRadioInterfaceLayer::CALL_STATE_DIALING:
        case nsIRadioInterfaceLayer::CALL_STATE_ALERTING:
          
          SendLine("+CIEV: 5,0");
          break;
        case nsIRadioInterfaceLayer::CALL_STATE_CONNECTED:
          
          SendLine("+CIEV: 4,0");
          break;
        default:
#ifdef DEBUG
          NS_WARNING("Not handling state changed");
#endif
          break;
      }
      break;

    default:
#ifdef DEBUG
      NS_WARNING("Not handling state changed");
#endif
      break;
  }

  mCurrentCallIndex = aCallIndex;
  mCurrentCallState = aCallState;
}

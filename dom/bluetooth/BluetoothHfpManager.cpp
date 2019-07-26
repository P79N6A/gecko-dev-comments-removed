





#include "BluetoothHfpManager.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothServiceUuid.h"

USING_BLUETOOTH_NAMESPACE
using namespace mozilla::ipc;

static nsRefPtr<BluetoothHfpManager> sInstance = nullptr;

BluetoothHfpManager::BluetoothHfpManager()
{
}

BluetoothHfpManager::~BluetoothHfpManager()
{
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


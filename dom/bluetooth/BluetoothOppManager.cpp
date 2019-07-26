





#include "base/basictypes.h"
#include "BluetoothOppManager.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothServiceUuid.h"
#include "ObexBase.h"

#include "mozilla/dom/ipc/Blob.h"
#include "mozilla/RefPtr.h"

USING_BLUETOOTH_NAMESPACE
using namespace mozilla::ipc;

static mozilla::RefPtr<BluetoothOppManager> sInstance;

BluetoothOppManager::BluetoothOppManager() : mConnected(false)
                                           , mConnectionId(1)
                                           , mLastCommand(0)
{
}

BluetoothOppManager::~BluetoothOppManager()
{
}


BluetoothOppManager*
BluetoothOppManager::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (sInstance == nullptr) {
    sInstance = new BluetoothOppManager();
  }

  return sInstance;
}

bool
BluetoothOppManager::Connect(const nsAString& aDeviceObjectPath,
                             BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return false;
  }

  nsString serviceUuidStr =
    NS_ConvertUTF8toUTF16(mozilla::dom::bluetooth::BluetoothServiceUuidStr::ObjectPush);

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
BluetoothOppManager::Disconnect()
{
  CloseSocket();
}

bool
BluetoothOppManager::SendFile(BlobParent* aParent,
                              BluetoothReplyRunnable* aRunnable)
{
  
  return true;
}

bool
BluetoothOppManager::StopSendingFile(BluetoothReplyRunnable* aRunnable)
{
  
  return true;
}


void
BluetoothOppManager::ReceiveSocketData(UnixSocketRawData* aMessage)
{
  uint8_t responseCode = aMessage->mData[0];

  if (mLastCommand == ObexRequestCode::Connect) {
    if (responseCode == ObexResponseCode::Success) {
      mConnected = true;
    }
  } else if (mLastCommand == ObexRequestCode::Disconnect) {
    if (responseCode == ObexResponseCode::Success) {
      mConnected = false;
    }
  }
}

void
BluetoothOppManager::SendConnectReqeust()
{
  
  
  uint8_t req[255];
  int index = 7;

  req[3] = 0x10; 
  req[4] = 0x00; 
  req[5] = BluetoothOppManager::MAX_PACKET_LENGTH >> 8;
  req[6] = BluetoothOppManager::MAX_PACKET_LENGTH;

  index += AppendHeaderConnectionId(&req[index], mConnectionId++);
  SetObexPacketInfo(req, ObexRequestCode::Connect, index);
  mLastCommand = ObexRequestCode::Connect;

  UnixSocketRawData* s = new UnixSocketRawData(index);
  memcpy(s->mData, req, s->mSize);
  SendSocketData(s);
}


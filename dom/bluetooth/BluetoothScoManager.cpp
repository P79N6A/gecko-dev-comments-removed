





#include "base/basictypes.h"

#include "BluetoothScoManager.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothServiceUuid.h"

#include "mozilla/Services.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "nsContentUtils.h"
#include "nsIDOMDOMRequest.h"
#include "nsIObserverService.h"
#include "nsISystemMessagesInternal.h"
#include "nsVariant.h"

USING_BLUETOOTH_NAMESPACE
using namespace mozilla::ipc;

static nsRefPtr<BluetoothScoManager> sInstance;
static nsCOMPtr<nsIThread> sScoCommandThread;

BluetoothScoManager::BluetoothScoManager()
{
  if (!sScoCommandThread) {
    if (NS_FAILED(NS_NewThread(getter_AddRefs(sScoCommandThread)))) {
      NS_ERROR("Failed to new thread for sScoCommandThread");
    }
  }
  mConnected = false;
}

BluetoothScoManager::~BluetoothScoManager()
{
  
  if (sScoCommandThread) {
    nsCOMPtr<nsIThread> thread;
    sScoCommandThread.swap(thread);
    if (NS_FAILED(thread->Shutdown())) {
      NS_WARNING("Failed to shut down the bluetooth hfpmanager command thread!");
    }
  }
}


BluetoothScoManager*
BluetoothScoManager::Get()
{
  if (sInstance == nullptr) {
    sInstance = new BluetoothScoManager();
  }

  
  return sInstance;
}


void
BluetoothScoManager::ReceiveSocketData(mozilla::ipc::UnixSocketRawData* aMessage)
{
  
  MOZ_NOT_REACHED("This should never be called!");
}

bool
BluetoothScoManager::Connect(const nsAString& aDeviceObjectPath)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mConnected) {
    NS_WARNING("Sco socket has been ready");
    return true;
  }

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return false;
  }

  nsresult rv = bs->GetScoSocket(aDeviceObjectPath,
                                 true,
                                 false,
                                 this);

  return NS_FAILED(rv) ? false : true;
}

void
BluetoothScoManager::Disconnect()
{
  CloseSocket();
  mConnected = false;
}


bool
BluetoothScoManager::GetConnected()
{
  return mConnected;
}

void
BluetoothScoManager::SetConnected(bool aConnected)
{
  mConnected = aConnected;
}
